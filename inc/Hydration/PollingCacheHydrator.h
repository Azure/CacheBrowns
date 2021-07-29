// Copyright (c) Microsoft Corporation. All rights reserved.

#ifndef CACHEBROWNS_POLLINGCACHEHYDRATOR_H
#define CACHEBROWNS_POLLINGCACHEHYDRATOR_H

#include "../DataSource/ICacheDataSource.h"
#include "../Store/ICacheStoreStrategy.h"
#include "ICacheHydrationStrategy.h"

#include <chrono>
#include <memory>
#include <mutex>
#include <shared_mutex>

namespace Microsoft::Azure::CacheBrowns::Hydration
{
    using namespace Microsoft::Azure::CacheBrowns::DataSource;

    template<
            typename Key,
            typename Value,
            InvalidCacheEntryBehavior whenInvalid = InvalidCacheEntryBehavior::ReturnNotValid>
    class PollingCacheHydrator final : public ICacheHydrationStrategy<Key, Value, whenInvalid>
    {
        using CacheStorePtr = std::unique_ptr<ICacheStoreStrategy<Key, Value>>;
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

        // TODO: Using iterators this isn't necessary to track, but implementing the generically isn't necessary for PoC
        std::set<Key> keys;

    public:
        PollingCacheHydrator(
                CacheStorePtr& dataStore,
                CacheDataRetrieverPtr& dataRetriever,
                std::chrono::milliseconds pollEvery);

        auto Get(const Key& key) -> std::tuple<CacheLookupResult, Value> override;

        void Invalidate(const Key& key) override;

        void Delete(const Key& key) override;

        void Flush() override;

        /// Polling thread will block object destruct, sends kill and sleep interrupt signals.
        /// NOTE: This will not immediately result in thread destruct if a poll operation is currently in progress.
        /// Consider the upper bound of the injected retrieve operation to be the approximate upper bound for this
        /// operation.
        ~PollingCacheHydrator();

    private:
        auto TryHydrateForPoll(const Key& key, Value previousValue) -> std::tuple<bool, Value>;
        auto TryHydrate(const Key& key) -> std::tuple<bool, Value>;
        void Hydrate(const Key& key, Value& retrievedValue);
        void Poll();
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

    // Intended to be used from polling thread. Since that thread already acquires a lock, this function is lockless
    template<typename Key, typename Value, InvalidCacheEntryBehavior whenInvalid>
    auto PollingCacheHydrator<Key, Value, whenInvalid>::TryHydrateForPoll(const Key& key, Value previousValue)
            -> std::tuple<bool, Value>
    {
        auto wasRetrieved = cacheDataRetriever->Retrieve(key, previousValue);

        if (wasRetrieved)
        {
            Hydrate(key, previousValue);
        }
        else
        {
            // Value in cache, but polling operation failed meaning the value is now stale
            invalidEntries.insert(key);
        }

        return {wasRetrieved, previousValue};
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
    void PollingCacheHydrator<Key, Value, whenInvalid>::Invalidate(const Key& key)
    {
        std::unique_lock writeLock(dataSourceMutex);
        invalidEntries.insert(key);
    }

    template<typename Key, typename Value, InvalidCacheEntryBehavior whenInvalid>
    void PollingCacheHydrator<Key, Value, whenInvalid>::Delete(const Key& key)
    {
        std::unique_lock writeLock(dataSourceMutex);
        keys.erase(key);
        cacheDataStore->Delete(key);
        invalidEntries.erase(key);
    }

    template<typename Key, typename Value, InvalidCacheEntryBehavior whenInvalid>
    void PollingCacheHydrator<Key, Value, whenInvalid>::Flush()
    {
        std::unique_lock writeLock(dataSourceMutex);
        keys.clear();
        cacheDataStore->Flush();
        invalidEntries.clear();
    }

    template<typename Key, typename Value, InvalidCacheEntryBehavior whenInvalid>
    PollingCacheHydrator<Key, Value, whenInvalid>::PollingCacheHydrator(
            PollingCacheHydrator::CacheStorePtr& dataStore,
            PollingCacheHydrator::CacheDataRetrieverPtr& dataRetriever,
            std::chrono::milliseconds pollEvery) :
        cacheDataStore(std::move(dataStore)),
        cacheDataRetriever(std::move(dataRetriever)), pollRate(pollEvery)
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
            {
                // TODO: This is a really expensive lock that can cause lag spikes. Evaluate other options
                // e.g.: Copy keys under lock. Retrieve value not under lock. Acquire lock, update if still exists
                // this creates an implicit queue with fast inserts, because if data races did occur (remember, they
                // can only be deletes in this paradigm as inserts don't need to be immediately polled) we respect the
                // prior event. If the event comes mid update, it will be delayed acquiring it's lock and immediately
                // delete whatever data we just wrote. In this case we maintain coherency while limiting the max block
                // delay. We don't gain the ability to interrupt retrieves but that cost is small. If we needed, we
                // could also check if a delete occurred before performing a read to reduce unnecessary retrieves.
                std::unique_lock writeLock(dataSourceMutex);

                for (const auto& key : keys)
                {
                    if (!activelyPolling)
                    {
                        break;
                    }

                    TryHydrateForPoll(key, std::get<1>(cacheDataStore->Get(key)));
                }
            }

            // Ignore spurious wake-ups. Used instead of sleep to allow for fast-kill
            cv.wait_for(lock, pollRate, [&] { return !activelyPolling; });
        }
    }

}// namespace Microsoft::Azure::CacheBrowns::Hydration

#endif//CACHEBROWNS_POLLINGCACHEHYDRATOR_H
