// Copyright (c) Microsoft Corporation. All rights reserved.

#ifndef CACHEBROWNS_PULLCACHEHYDRATOR_H
#define CACHEBROWNS_PULLCACHEHYDRATOR_H

#include "../DataSource/ICacheDataSource.h"
#include "../Store/ICacheStoreStrategy.h"
#include "ICacheHydrationStrategy.h"

#include <memory>
#include <set>

namespace Microsoft::Azure::CacheBrowns::Hydration
{

    using namespace Store;

    template<
            typename Key,
            typename Value,
            InvalidCacheEntryBehavior whenInvalid = InvalidCacheEntryBehavior::ReturnNotValid>
    class PullCacheHydrator final : public ICacheHydrationStrategy<Key, Value, whenInvalid>
    {
        typedef std::shared_ptr<IHydratable<Key, Value>> cacheStorePtr;
        typedef std::unique_ptr<Microsoft::Azure::CacheBrowns::DataSource::ICacheDataSource<Key, Value>>
                cacheDataSourcePtr;

        cacheStorePtr cacheDataStore;
        cacheDataSourcePtr cacheDataRetriever;

        /// Marks entries as invalid, overriding whatever would be returned by ICacheDataSource::IsValid
        std::set<Key> invalidEntryOverrides;

    public:
        PullCacheHydrator(cacheStorePtr dataStore, cacheDataSourcePtr& dataSource);

        std::tuple<CacheLookupResult, Value> Get(const Key& key) override;

        void HandleInvalidate(const Key& key) override;

        void HandleDelete(const Key& key) override;

        void HandleFlush() override;

    private:
        std::tuple<bool, Value> TryHydrate(const Key& key);

        std::tuple<bool, Value> TryHydrate(const Key& key, const Value& currentValue);

        std::tuple<bool, Value> PullCacheHydrator<Key, Value, whenInvalid>::HandleRetrieveResult(
                const Key& key,
                bool wasRetrieved,
                Value retrievedValue);
    };

    template<typename Key, typename Value, InvalidCacheEntryBehavior whenInvalid>
    std::tuple<CacheLookupResult, Value> PullCacheHydrator<Key, Value, whenInvalid>::Get(const Key& key)
    {
        bool found, wasHydrated = false, valid = false;
        Value datum;
        std::tie(found, datum) = cacheDataStore->Get(key);

        if (found)
        {
            valid = cacheDataRetriever->IsValid(key, datum);

            if (!valid || invalidEntryOverrides.find(key) != invalidEntryOverrides.end())
            {
                std::tie(wasHydrated, datum) = TryHydrate(key, datum);
            }
        }
        else
        {
            std::tie(wasHydrated, datum) = TryHydrate(key);
        }

        return Hydration::CreateCacheLookupResult<whenInvalid, Value>(found, valid, wasHydrated, datum);
    }

    template<typename Key, typename Value, InvalidCacheEntryBehavior whenInvalid>
    std::tuple<bool, Value> PullCacheHydrator<Key, Value, whenInvalid>::TryHydrate(const Key& key)
    {
        auto [wasRetrieved, retrievedValue] = cacheDataRetriever->Retrieve(key);

        return HandleRetrieveResult(key, wasRetrieved, retrievedValue);
    }

    template<typename Key, typename Value, InvalidCacheEntryBehavior whenInvalid>
    std::tuple<bool, Value> PullCacheHydrator<Key, Value, whenInvalid>::HandleRetrieveResult(
            const Key& key,
            bool wasRetrieved,
            Value retrievedValue)
    {
        if (wasRetrieved)
        {
            this->cacheDataStore->Set(key, retrievedValue);
            this->invalidEntryOverrides.erase(key);
        }

        return {wasRetrieved, retrievedValue};
    }

    template<typename Key, typename Value, InvalidCacheEntryBehavior whenInvalid>
    std::tuple<bool, Value> PullCacheHydrator<Key, Value, whenInvalid>::TryHydrate(const Key& key, const Value& currentValue)
    {
        auto [wasRetrieved, value] = cacheDataRetriever->Retrieve(key, currentValue);

        return HandleRetrieveResult(key, wasRetrieved, value);
    }


    template<typename Key, typename Value, InvalidCacheEntryBehavior whenInvalid>
    void PullCacheHydrator<Key, Value, whenInvalid>::HandleInvalidate(const Key& key)
    {
        invalidEntryOverrides.insert(key);
    }

    template<typename Key, typename Value, InvalidCacheEntryBehavior whenInvalid>
    void PullCacheHydrator<Key, Value, whenInvalid>::HandleDelete(const Key& key)
    {
        invalidEntryOverrides.erase(key);
    }

    template<typename Key, typename Value, InvalidCacheEntryBehavior whenInvalid>
    void PullCacheHydrator<Key, Value, whenInvalid>::HandleFlush()
    {
        invalidEntryOverrides.clear();
    }

    template<typename Key, typename Value, InvalidCacheEntryBehavior whenInvalid>
    PullCacheHydrator<Key, Value, whenInvalid>::PullCacheHydrator(
            PullCacheHydrator::cacheStorePtr dataStore,
            PullCacheHydrator::cacheDataSourcePtr& dataSource) :
        cacheDataStore(std::move(dataStore)),
        cacheDataRetriever(std::move(dataSource))
    {
    }
}// namespace Microsoft::Azure::CacheBrowns::Hydration

#endif//CACHEBROWNS_PULLCACHEHYDRATOR_H
