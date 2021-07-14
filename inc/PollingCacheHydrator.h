// Copyright (c) Microsoft Corporation. All rights reserved.

#ifndef CACHEBROWNS_POLLINGCACHEHYDRATOR_H
#define CACHEBROWNS_POLLINGCACHEHYDRATOR_H

#inlcude <memory>

#include "ICacheHydrationStrategy.h"
#include "ICacheStoreStrategy.h"
#include "ICacheDataSource.h"

namespace Microsoft::Azure::CacheBrowns {
    template<
            typename Key,
            typename Value,
            InvalidCacheEntryBehavior whenInvalid = InvalidCacheEntryBehavior::ReturnNotValid>
    class PullCacheHydrator final : public ICacheHydrationStrategy<Key, Value, whenInvalid>
    {
        typedef std::unique_ptr<ICacheStoreStrategy<Key, Value>> cacheStorePtr;
        typedef std::unique_ptr<IRetrievable<Key, Value>> cacheDataRetrieverPtr;

        cacheStorePtr cacheDataStore;
        cacheDataRetrieverPtr cacheDataRetriever;

        std::thread pollingThread;

    public:

        PullCacheHydrator(cacheStorePtr& dataStore, cacheDataRetrieverPtr& dataSource);

        std::tuple<CacheLookupResult, Value> Get(const Key &key) override;

        void Invalidate(const Key &key) override;

        void Delete(const Key &key) override;

        void Flush() override;

    private:

        std::tuple<bool, Value> TryHydrate(const Key &key);
    };

    template<typename Key, typename Value, InvalidCacheEntryBehavior whenInvalid>
    std::tuple<CacheLookupResult, Value>
    PullCacheHydrator<Key, Value, whenInvalid>::Get(const Key &key) {
        bool found, wasHydrated, valid;
        Value datum;
        std::tie(found, datum) = cacheDataStore->Get(key);

        if (found)
        {
            valid = cacheDataSource->IsValid(key, datum);

            if (!valid)
            {
                std::tie(wasHydrated, datum) = TryHydrate(key);
            }
        }
        else
        {
            std::tie(wasHydrated, datum) = TryHydrate(key);
        }

        return CacheBrowns::CreateCacheLookupResult<whenInvalid, Value>(found, valid, wasHydrated, datum);
    }

    template<typename Key, typename Value, InvalidCacheEntryBehavior whenInvalid>
    std::tuple<bool, Value> PullCacheHydrator<Key, Value, whenInvalid>::TryHydrate(const Key &key) {
        auto [wasRetrieved, retrievedValue] = cacheDataSource->Retrieve(key);

        if (wasRetrieved)
        {
            this->cacheDataStore->Set(key, retrievedValue);
        }

        return { wasRetrieved, retrievedValue };
    }

    template<typename Key, typename Value, InvalidCacheEntryBehavior whenInvalid>
    void PullCacheHydrator<Key, Value, whenInvalid>::Invalidate(const Key &key) {
        // TODO: This is only for purposes of purgable cache
    }

    template<typename Key, typename Value, InvalidCacheEntryBehavior whenInvalid>
    void PullCacheHydrator<Key, Value, whenInvalid>::Delete(const Key &key) {
        cacheDataStore->Delete(key);
    }

    template<typename Key, typename Value, InvalidCacheEntryBehavior whenInvalid>
    void PullCacheHydrator<Key, Value, whenInvalid>::Flush() {
        cacheDataStore->Flush();
    }

    template<typename Key, typename Value, InvalidCacheEntryBehavior whenInvalid>
    PullCacheHydrator<Key, Value, whenInvalid>::PollingCacheHydrator(PollingCacheHydrator::cacheStorePtr &dataStore,
                                                                  PollingCacheHydrator::cacheDataRetrieverPtr &dataRetriever)
            : cacheDataStore(std::move(dataStore)),
            cacheDataRetriever(std::move(dataSource)){
    }
}

#endif //CACHEBROWNS_POLLINGCACHEHYDRATOR_H
