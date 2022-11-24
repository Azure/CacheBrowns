// Copyright (c) Microsoft Corporation. All rights reserved.

#ifndef CACHEBROWNS_MANAGEDCACHE_H
#define CACHEBROWNS_MANAGEDCACHE_H

#include <memory>

#include "IManagedCache.h"
#include "Replacement/ICacheReplacementStrategy.h"
#include "Replacement/NoReplacement.h"

namespace Microsoft::Azure::CacheBrowns
{
    using namespace Hydration;
    using namespace Replacement;

    template<typename Key, typename Value>
    class ManagedCache final : public IManagedCache<Key, Value>
    {
        std::unique_ptr<ICacheReplacementStrategy<Key, Value>> cacheReplacementStrategy;

    public:
        explicit ManagedCache(std::unique_ptr<ICacheReplacementStrategy<Key, Value>>& replacementStrategy);

        explicit ManagedCache(
                std::unique_ptr<ICacheHydrationStrategy<Key, Value>>& hydrator,
                std::shared_ptr<IPrunable<Key>> cacheStorePtr);

        std::tuple<CacheLookupResult, Value> Get(const Key& key) override;

        void Flush() override;
    };

    template<typename Key, typename Value>
    ManagedCache<Key, Value>::ManagedCache(std::unique_ptr<ICacheReplacementStrategy<Key, Value>>& replacementStrategy) :
        cacheReplacementStrategy(std::move(replacementStrategy))
    {
    }

    template<typename Key, typename Value>
    ManagedCache<Key, Value>::ManagedCache(
            std::unique_ptr<ICacheHydrationStrategy<Key, Value>>& hydrator,
            std::shared_ptr<IPrunable<Key>> cacheStorePtr) :
        cacheReplacementStrategy(std::make_unique<NoReplacement<Key, Value>>(hydrator, cacheStorePtr))
    {
    }

    template<typename Key, typename Value>
    std::tuple<CacheLookupResult, Value> ManagedCache<Key, Value>::Get(const Key& key)
    {
        return cacheReplacementStrategy->Get(key);
    }

    template<typename Key, typename Value>
    void ManagedCache<Key, Value>::Flush()
    {
        cacheReplacementStrategy->Flush();
    }
}// namespace Microsoft::Azure::CacheBrowns

#endif//CACHEBROWNS_MANAGEDCACHE_H
