// Copyright (c) Microsoft Corporation. All rights reserved.

#ifndef CACHEBROWNS_POLLINGCACHEHYDRATOR_H
#define CACHEBROWNS_POLLINGCACHEHYDRATOR_H

#include "DataSource/ICacheDataSource.h"
#include "Store/ICacheStoreStrategy.h"
#include "Store/IHydratable.h"
#include "ICacheHydrationStrategy.h"
#include "Lib/PollingTask.h"
#include "Store/StoreDataKeyTracker.h"

#include <chrono>
#include <memory>
#include <mutex>
#include <set>

namespace Microsoft::Azure::CacheBrowns::Hydration
{
    using namespace Microsoft::Azure::CacheBrowns::DataSource;
    using namespace Microsoft::Azure::CacheBrowns::Store;

    template<
            typename Key,
            typename Value,
            InvalidCacheEntryBehavior whenInvalid = InvalidCacheEntryBehavior::ReturnNotValid>
    class PollingCacheHydrator final : public ICacheHydrationStrategy<Key, Value, whenInvalid>
    {
        using CacheStorePtr = std::shared_ptr<StoreDataKeyTracker<Key, Value>>;
        using CacheDataRetrieverPtr = std::unique_ptr<IRetrievable<Key, Value>>;

        CacheStorePtr cacheDataStore;
        CacheDataRetrieverPtr cacheDataRetriever;

        Microsoft::Azure::Host::Common::PollingTask<std::chrono::milliseconds> pollingTask;

        std::function<void(CacheLookupResult)> pollResultInstrumentationCallback;

    public:
        PollingCacheHydrator(
                CacheStorePtr dataStore,
                CacheDataRetrieverPtr& dataRetriever,
                std::chrono::milliseconds pollEvery,
                std::function<void(CacheLookupResult)> pollResultInstrumentation = [](CacheLookupResult) {});

        auto Get(const Key& key) -> std::tuple<CacheLookupResult, Value> override;

        void SetPollingRate(std::chrono::milliseconds pollRate);

        void Invalidate(const Key& key) override;

        /// NOTE: This will not immediately result in thread destruct if a poll operation is currently in progress.
        /// Consider the upper bound of the injected retrieve operation to be the approximate upper bound for this
        /// operation.
        ~PollingCacheHydrator() = default;

    private:
        auto TryHydrate(const Key& key) -> std::tuple<bool, Value>;
        void Hydrate(const Key& key, Value& retrievedValue);
        void TryRefresh(const Key& key);

        const std::function<void(const std::atomic<bool>&)> Poll = [this](const std::atomic<bool>& activelyPolling)
        {
            auto keysCopy = cacheDataStore->GetKeys();

            for (const auto& key : keysCopy)
            {
                if (!activelyPolling)
                {
                    break;
                }

                TryRefresh(key);
            }
        };
    };

    template<typename Key, typename Value, InvalidCacheEntryBehavior whenInvalid>
    auto PollingCacheHydrator<Key, Value, whenInvalid>::Get(const Key& key) -> std::tuple<CacheLookupResult, Value>
    {
        bool found, wasHydrated = false, valid = true;
        Value datum;

        // Don't hold lock longer than necessary
        {
            std::shared_lock readLock(dataStoreMutex);

            std::tie(found, datum) = cacheDataStore->Get(key);

            if (found)
            {
                valid = cacheDataStore->IsMarkedInvalid(key);
            }
            // Value isn't registered for polling, fetch (which implicitly registers)
            else
            {
                // Unlock, we're done reading at this point & don't want to block the write operation
                readLock.unlock();
                std::tie(wasHydrated, datum) = TryHydrate(key);
            }
        }

        return CacheBrowns::CreateCacheLookupResult<whenInvalid, Value>(found, valid, wasHydrated, datum);
    }

    template<typename Key, typename Value, InvalidCacheEntryBehavior whenInvalid>
    auto PollingCacheHydrator<Key, Value, whenInvalid>::TryHydrate(const Key& key) -> std::tuple<bool, Value>
    {
        auto [wasRetrieved, retrievedValue] = cacheDataRetriever->Retrieve(key);

        if (wasRetrieved)
        {
            std::unique_lock writeLock(dataStoreMutex);
            Hydrate(key, retrievedValue);
        }

        return {wasRetrieved, retrievedValue};
    }

    template<typename Key, typename Value, InvalidCacheEntryBehavior whenInvalid>
    void PollingCacheHydrator<Key, Value, whenInvalid>::Hydrate(const Key& key, Value& retrievedValue)
    {
        keys.insert(key);
        cacheDataStore->Set(key, retrievedValue);
    }

    template<typename Key, typename Value, InvalidCacheEntryBehavior whenInvalid>
    void PollingCacheHydrator<Key, Value, whenInvalid>::Invalidate(const Key& key)
    {
        std::unique_lock writeLock(dataStoreMutex);
        cacheDataStore->MarkInvalid();
    }

    template<typename Key, typename Value, InvalidCacheEntryBehavior whenInvalid>
    PollingCacheHydrator<Key, Value, whenInvalid>::PollingCacheHydrator(
            PollingCacheHydrator::CacheStorePtr dataStore,
            PollingCacheHydrator::CacheDataRetrieverPtr& dataRetriever,
            std::chrono::milliseconds pollEvery,
            std::function<void(CacheLookupResult)> pollResultInstrumentation) :
        cacheDataStore(std::move(dataStore)),
        cacheDataRetriever(std::move(dataRetriever)),
        pollResultInstrumentationCallback(std::move(pollResultInstrumentation)),
        pollingTask(pollEvery, Poll)
    {
    }

    template<typename Key, typename Value, InvalidCacheEntryBehavior whenInvalid>
    void PollingCacheHydrator<Key, Value, whenInvalid>::TryRefresh(const Key& key)
    {
        bool wasHydrated = false, valid = false;
        std::shared_lock readLock(dataStoreMutex);

        bool found = keys.find(key) != keys.end();

        // If value is already deleted, don't issue a superfluous retrieve
        if (found)
        {
            auto previousValue = std::get<1>(cacheDataStore->Get(key));
            auto [wasRetrieved, newValue] = cacheDataRetriever->Retrieve(key, previousValue);

            readLock.unlock();

            std::unique_lock writeLock(dataStoreMutex);

            // Value could have been deleted during retrieval, verify update should still occur
            valid = keys.find(key) != keys.end();

            if (wasRetrieved && valid)
            {
                wasHydrated = true;
                Hydrate(key, previousValue);
            }
            else
            {
                // Value in cache, but polling operation failed meaning the value is now stale
                cacheDataStore->MarkInvalid(key);
            }
        }

        pollResultInstrumentationCallback(
                CacheBrowns::FindCacheLookupResultWithSemantics<whenInvalid>(found, valid, wasHydrated));
    }

    template<typename Key, typename Value, InvalidCacheEntryBehavior whenInvalid>
    void PollingCacheHydrator<Key, Value, whenInvalid>::SetPollingRate(std::chrono::milliseconds pollRate)
    {
        pollingTask.SetPollingRate(pollRate);
    }

}// namespace Microsoft::Azure::CacheBrowns::Hydration

#endif//CACHEBROWNS_POLLINGCACHEHYDRATOR_H
