// Copyright (c) Microsoft Corporation. All rights reserved.

#ifndef CACHEBROWNS_PULLCACHEHYDRATOR_H
#define CACHEBROWNS_PULLCACHEHYDRATOR_H

#include "ICacheHydrationStrategy.h"
#include "../Store/ICacheStoreStrategy.h"
#include "../DataSource/ICacheDataSource.h"
#include <set>

namespace Microsoft::Azure::CacheBrowns {
    template<
            typename Key,
            typename Value,
            InvalidCacheEntryBehavior whenInvalid = InvalidCacheEntryBehavior::ReturnNotValid>
    class PullCacheHydrator final : public ICacheHydrationStrategy<Key, Value, whenInvalid>
    {
        typedef std::unique_ptr<ICacheStoreStrategy<Key, Value>> cacheStorePtr;
        typedef std::unique_ptr<ICacheDataSource<Key, Value>> cacheDataSourcePtr;

        cacheStorePtr cacheDataStore;
        cacheDataSourcePtr cacheDataRetriever;

        /// Marks entries as invalid, overriding whatever would be returned by ICacheDataSource::IsValid
        std::set<Key> invalidEntryOverrides;

    public:

        PullCacheHydrator(cacheStorePtr& dataStore, cacheDataSourcePtr& dataSource);

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
            valid = cacheDataRetriever->IsValid(key, datum);

            if (!valid || invalidEntryOverrides.find(key) != invalidEntryOverrides.end())
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
        auto [wasRetrieved, retrievedValue] = cacheDataRetriever->Retrieve(key);

        if (wasRetrieved)
        {
            this->cacheDataStore->Set(key, retrievedValue);
            invalidEntryOverrides.erase(key);
        }

        return { wasRetrieved, retrievedValue };
    }

    template<typename Key, typename Value, InvalidCacheEntryBehavior whenInvalid>
    void PullCacheHydrator<Key, Value, whenInvalid>::Invalidate(const Key &key) {
        invalidEntryOverrides.insert(key);
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
    PullCacheHydrator<Key, Value, whenInvalid>::PullCacheHydrator(PullCacheHydrator::cacheStorePtr &dataStore,
                                                                  PullCacheHydrator::cacheDataSourcePtr &dataSource)
                                                                  : cacheDataStore(std::move(dataStore)),
                                                                    cacheDataRetriever(std::move(dataSource)){
    }
}

#endif //CACHEBROWNS_PULLCACHEHYDRATOR_H
