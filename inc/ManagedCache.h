// Copyright (c) Microsoft Corporation. All rights reserved.

#ifndef CACHEBROWNS_MANAGEDCACHE_H
#define CACHEBROWNS_MANAGEDCACHE_H

#include <memory>

#include "IManagedCache.h"
#include "Replacement/ICacheReplacementStrategy.h"
#include "Replacement/NoReplacement.h"

namespace Microsoft::Azure::CacheBrowns
{
    template<typename Key, typename Value>
    class ManagedCache final : public IManagedCache<Key, Value>
    {
        typedef std::unique_ptr<ICacheReplacementStrategy<Key, Value>> CacheReplacementStrategy;

        CacheReplacementStrategy cacheReplacementStrategy;

    public:

        explicit ManagedCache(CacheReplacementStrategy& replacementStrategy);

        explicit ManagedCache(std::unique_ptr<ICacheHydrationStrategy<Key, Value>>& hydrator);

        std::tuple<CacheLookupResult, Value> Get(const Key& key) override;

        void Flush() override;
    };

    template<typename Key, typename Value>
    ManagedCache<Key, Value>::ManagedCache(
            ManagedCache::CacheReplacementStrategy &replacementStrategy)
        : cacheReplacementStrategy(replacementStrategy)
    {

    }

    template<typename Key, typename Value>
    ManagedCache<Key, Value>::ManagedCache(std::unique_ptr<ICacheHydrationStrategy<Key, Value>> &hydrator)
            : cacheReplacementStrategy(std::make_unique<NoReplacement<Key, Value>>(hydrator))
    {

    }

    template<typename Key, typename Value>
    std::tuple <CacheLookupResult, Value> ManagedCache<Key, Value>::Get(const Key &key) {
        return cacheReplacementStrategy->Get(key);
    }

    template<typename Key, typename Value>
    void ManagedCache<Key, Value>::Flush() {
        cacheReplacementStrategy->Flush();
    }
}

#endif //CACHEBROWNS_MANAGEDCACHE_H
