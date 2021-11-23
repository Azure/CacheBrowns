// Copyright (c) Microsoft Corporation. All rights reserved.

#ifndef CACHEBROWNS_LEASTRECENTLYUSED_H
#define CACHEBROWNS_LEASTRECENTLYUSED_H

#include "../Hydration/ICacheHydrationStrategy.h"
#include "ICacheReplacementStrategy.h"

#include <list>
#include <unordered_map>

namespace Microsoft::Azure::CacheBrowns::Replacement
{
    using namespace Hydration;

    template<typename Key, typename Value>
    class LeastRecentlyUsed final : public ICacheReplacementStrategy<Key, Value>
    {
        using Hydrator = std::unique_ptr<ICacheHydrationStrategy<Key, Value>>;
        using cacheStorePtr = std::shared_ptr<IPrunable<Key>>;
        using List = std::list<Key>;
        
        Hydrator cacheHydrator;
        cacheStorePtr dataStore;
        List usageOrder;
        std::unordered_map<Key, typename List::iterator> usageIterators;

        void UpdateUsageOrder(const Key& key, CacheLookupResult result);

    public:
        explicit LeastRecentlyUsed(Hydrator& hydrator, cacheStorePtr store);

        std::tuple<CacheLookupResult, Value> Get(const Key& key);

        void Invalidate(const Key& key);

        void Delete(const Key& key);

        void Flush();
    };

    template<typename Key, typename Value>
    LeastRecentlyUsed<Key, Value>::LeastRecentlyUsed(LeastRecentlyUsed::Hydrator& hydrator, cacheStorePtr store) :
        cacheHydrator(std::move(hydrator)), dataStore(store)
    {
    }
    
    template<typename Key, typename Value>
    std::tuple<CacheLookupResult, Value> LeastRecentlyUsed<Key, Value>::Get(const Key& key)
    {
        auto result = cacheHydrator->Get(key);

        if (CacheEntryCurrentlyPresent(std::get<0>(result)) && key != usageOrder.front())
        {
            UpdateUsageOrder(key, std::get<0>(result));
        }

        return result;
    }
    template<typename Key, typename Value>
    void LeastRecentlyUsed<Key, Value>::UpdateUsageOrder(const Key& key, CacheLookupResult result)
    {
        if (CacheEntryWasAlreadyPresent(result))
        {
            auto itr = usageIterators.find(key);
            usageOrder.erase(itr->second);
        }

        usageOrder.push_front(key);
        usageIterators[key] = usageOrder.begin();
    }

    template<typename Key, typename Value>
    void LeastRecentlyUsed<Key, Value>::Invalidate(const Key& key)
    {
        cacheHydrator->HandleInvalidate(key);
    }

    template<typename Key, typename Value>
    void LeastRecentlyUsed<Key, Value>::Delete(const Key& key)
    {
        dataStore->Delete(key);
        cacheHydrator->HandleDelete(key);
        usageOrder.erase(usageIterators.find(key)->second);
        usageIterators.erase(key);
    }

    template<typename Key, typename Value>
    void LeastRecentlyUsed<Key, Value>::Flush()
    {
        dataStore->Flush();
        cacheHydrator->HandleFlush();
        usageOrder.clear();
        usageIterators.clear();
    }
}

#endif// CACHEBROWNS_LEASTRECENTLYUSED_H
