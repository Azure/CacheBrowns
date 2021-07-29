// Copyright (c) Microsoft Corporation. All rights reserved.

#ifndef CACHEBROWNS_POLLINGCACHEHYDRATOR_H
#define CACHEBROWNS_POLLINGCACHEHYDRATOR_H

#include "../DataSource/ICacheDataSource.h"
#include "../Store/ICacheStoreStrategy.h"
#include "../Store/IHydratable.h"
#include "ICacheHydrationStrategy.h"

#include <chrono>
#include <memory>
#include <mutex>
#include <set>
#include <shared_mutex>

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
        using CacheStorePtr = std::shared_ptr<IHydratable<Key, Value>>;
        using CacheDataRetrieverPtr = std::unique_ptr<IRetrievable<Key, Value>>;

        CacheStorePtr cacheDataStore;
        mutable std::shared_mutex dataSourceMutex;
        CacheDataRetrieverPtr cacheDataRetriever;

        // Used to fast-kill the polling thread
        std::condition_variable cv;
        bool activelyPolling = true;

        std::thread pollingThread;
        std::set<Key> invalidEntries;
        std::chrono::milliseconds pollRate;
        std::function<void(CacheLookupResult)> pollResultInstrumentationCallback;

        // TODO: Using iterators this isn't necessary to track, but implementing the generically isn't necessary for PoC
        std::set<Key> keys;

    public:
        PollingCacheHydrator(
                CacheStorePtr dataStore,
                CacheDataRetrieverPtr& dataRetriever,
                std::chrono::milliseconds pollEvery,
                std::function<void(CacheLookupResult)> pollResultInstrumentation = [](CacheLookupResult) {});

        auto Get(const Key& key) -> std::tuple<CacheLookupResult, Value> override;

        void HandleInvalidate(const Key& key) override;

        void HandleDelete(const Key& key) override;

        void HandleFlush() override;

        /// Polling thread will block object destruct, sends kill and sleep interrupt signals.
        /// NOTE: This will not immediately result in thread destruct if a poll operation is currently in progress.
        /// Consider the upper bound of the injected retrieve operation to be the approximate upper bound for this
        /// operation.
        ~PollingCacheHydrator();

    private:
        auto TryHydrate(const Key& key) -> std::tuple<bool, Value>;
        void Hydrate(const Key& key, Value& retrievedValue);
        void Poll();
        std::set<Key> GetCopyOfKeys() const;
        void TryRefresh(const Key& key);
    };

    template<typename Key, typename Value, InvalidCacheEntryBehavior whenInvalid>
    auto PollingCacheHydrator<Key, Value, whenInvalid>::Get(const Key& key) -> std::tuple<CacheLookupResult, Value>
    {
        bool found, wasHydrated = false, valid = true;
        Value datum;

        // Don't hold lock longer than necessary
        {
            std::shared_lock readLock(dataSourceMutex);

            std::tie(found, datum) = cacheDataStore->Get(key);

            if (found)
            {
                valid = invalidEntries.find(key) != invalidEntries.end();
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
            std::unique_lock writeLock(dataSourceMutex);
            Hydrate(key, retrievedValue);
        }

        return {wasRetrieved, retrievedValue};
    }

    template<typename Key, typename Value, InvalidCacheEntryBehavior whenInvalid>
    void PollingCacheHydrator<Key, Value, whenInvalid>::Hydrate(const Key& key, Value& retrievedValue)
    {
        keys.insert(key);
        cacheDataStore->Set(key, retrievedValue);
        invalidEntries.erase(key);
    }

    template<typename Key, typename Value, InvalidCacheEntryBehavior whenInvalid>
    void PollingCacheHydrator<Key, Value, whenInvalid>::HandleInvalidate(const Key& key)
    {
        std::unique_lock writeLock(dataSourceMutex);
        invalidEntries.insert(key);
    }

    template<typename Key, typename Value, InvalidCacheEntryBehavior whenInvalid>
    void PollingCacheHydrator<Key, Value, whenInvalid>::HandleDelete(const Key& key)
    {
        std::unique_lock writeLock(dataSourceMutex);
        keys.erase(key);
        invalidEntries.erase(key);
    }

    template<typename Key, typename Value, InvalidCacheEntryBehavior whenInvalid>
    void PollingCacheHydrator<Key, Value, whenInvalid>::HandleFlush()
    {
        std::unique_lock writeLock(dataSourceMutex);
        keys.clear();
        invalidEntries.clear();
    }

    template<typename Key, typename Value, InvalidCacheEntryBehavior whenInvalid>
    PollingCacheHydrator<Key, Value, whenInvalid>::PollingCacheHydrator(
            PollingCacheHydrator::CacheStorePtr dataStore,
            PollingCacheHydrator::CacheDataRetrieverPtr& dataRetriever,
            std::chrono::milliseconds pollEvery,
            std::function<void(CacheLookupResult)> pollResultInstrumentation) :
        cacheDataStore(std::move(dataStore)),
        cacheDataRetriever(std::move(dataRetriever)), pollRate(pollEvery),
        pollResultInstrumentationCallback(std::move(pollResultInstrumentation))
    {
        pollingThread = std::thread(&PollingCacheHydrator::Poll, this);
    }

    template<typename Key, typename Value, InvalidCacheEntryBehavior whenInvalid>
    PollingCacheHydrator<Key, Value, whenInvalid>::~PollingCacheHydrator()
    {
        activelyPolling = false;
        cv.notify_all();
    }

    template<typename Key, typename Value, InvalidCacheEntryBehavior whenInvalid>
    void PollingCacheHydrator<Key, Value, whenInvalid>::Poll()
    {
        std::mutex mutex;
        std::unique_lock<std::mutex> lock(mutex);

        // This is an example of bulk polling. You could have implementations that maintain freshness on a per entry
        // basis or bucket basis if desired. Because the locks are only held for the final writes, you could also
        // upgrade this or any other implementation to accept async data sources.
        while (activelyPolling)
        {
            auto keysCopy = GetCopyOfKeys();

            for (const auto& key : keysCopy)
            {
                if (!activelyPolling)
                {
                    break;
                }

                TryRefresh(key);

                // TODO: Allow callback for instrumentation of polling thread.
            }

            // Ignore spurious wake-ups. Used instead of sleep to allow for fast-kill
            cv.wait_for(lock, pollRate, [&] { return !activelyPolling; });
        }
    }

    template<typename Key, typename Value, InvalidCacheEntryBehavior whenInvalid>
    void PollingCacheHydrator<Key, Value, whenInvalid>::TryRefresh(const Key& key)
    {
        bool wasHydrated = false, valid = false;
        std::shared_lock readLock(dataSourceMutex);

        bool found = keys.find(key) != keys.end();

        // If value is already deleted, don't issue a superfluous retrieve
        if (found)
        {
            auto previousValue = std::get<1>(cacheDataStore->Get(key));
            auto wasRetrieved = cacheDataRetriever->Retrieve(key, previousValue);

            readLock.unlock();

            std::unique_lock writeLock(dataSourceMutex);

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
                invalidEntries.insert(key);
            }
        }

        pollResultInstrumentationCallback(
                CacheBrowns::FindCacheLookupResultWithSemantics<whenInvalid>(found, valid, wasHydrated));
    }

    template<typename Key, typename Value, InvalidCacheEntryBehavior whenInvalid>
    std::set<Key> PollingCacheHydrator<Key, Value, whenInvalid>::GetCopyOfKeys() const
    {
        std::shared_lock readLock(dataSourceMutex);
        return keys;
    }

}// namespace Microsoft::Azure::CacheBrowns::Hydration

#endif//CACHEBROWNS_POLLINGCACHEHYDRATOR_H
