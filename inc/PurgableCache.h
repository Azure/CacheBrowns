// Copyright (c) Microsoft Corporation. All rights reserved.

#ifndef CACHEBROWNS_PURGABLECACHE_H
#define CACHEBROWNS_PURGABLECACHE_H

#include "IManagedCache.h"

namespace Microsoft::Azure::CacheBrowns
{
    template<typename Key, typename Value>
    class PurgableCache final : public IManagedCache<Key, Value>
    {
        typedef std::unique_ptr<ICacheReplacementStrategy<Key, Value>> CacheReplacementStrategy;

        CacheReplacementStrategy cacheReplacementStrategy;

    public:
        explicit PurgableCache(CacheReplacementStrategy& replacementStrategy);

        explicit PurgableCache(std::unique_ptr<ICacheHydrationStrategy<Key, Value>>& hydrator);

        std::tuple<CacheLookupResult, Value> Get(const Key& key) override;

        void Flush() override;

        void Evict(const Key& key);

        /// Evict then reload
        bool Replace(const Key& key);

        /// Invalidate then reload
        bool Refresh(const Key& key);

        /// Invalidate the underlying entry
        void Invalidate(const Key& key);
    };

    template<typename Key, typename Value>
    PurgableCache<Key, Value>::PurgableCache(
            ManagedCache::CacheReplacementStrategy &replacementStrategy)
            : cacheReplacementStrategy(replacementStrategy)
    {

    }

    template<typename Key, typename Value>
    PurgableCache<Key, Value>::PurgableCache(std::unique_ptr<ICacheHydrationStrategy<Key, Value>> &hydrator)
            : cacheReplacementStrategy(std::make_unique<NoReplacement<Key, Value>>(hydrator))
    {

    }

    template<typename Key, typename Value>
    std::tuple <CacheLookupResult, Value> PurgableCache<Key, Value>::Get(const Key &key) {
        return cacheReplacementStrategy->Get(key);
    }

    template<typename Key, typename Value>
    void PurgableCache<Key, Value>::Flush() {
        cacheReplacementStrategy->Flush();
    }

    template<typename Key, typename Value>
    void PurgableCache<Key, Value>::Evict(const Key &key) {
        cacheReplacementStrategy->Delete(key);
    }

    template<typename Key, typename Value>
    bool PurgableCache<Key, Value>::Replace(const Key &key) {
        cacheReplacementStrategy->Delete(key);
        cacheReplacementStrategy->Get(key);
    }

    template<typename Key, typename Value>
    bool PurgableCache<Key, Value>::Refresh(const Key &key) {
        cacheReplacementStrategy->Invalidate(key);
        cacheReplacementStrategy->Get(key);
    }

    template<typename Key, typename Value>
    void PurgableCache<Key, Value>::Invalidate(const Key &key) {
        cacheReplacementStrategy->Invalidate(key);
    }
}

#endif //CACHEBROWNS_PURGABLECACHE_H
