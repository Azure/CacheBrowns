// Copyright (c) Microsoft Corporation. All rights reserved.

#ifndef CACHEBROWNS_PURGABLECACHE_H
#define CACHEBROWNS_PURGABLECACHE_H

#include "IManagedCache.h"

namespace Microsoft::Azure::CacheBrowns
{
    template<typename Key, typename Value>
    class PurgableCache final : public IManagedCache<Key, Value>
    {
    public:
        std::tuple<CacheLookupResult, Value> Get(const Key& key) override;

        void Flush() override;

        void Evict(const Key& key);

        void Replace(const Key& key);

        void Refresh(const Key& key);
    };

    template<typename Key, typename Value>
    std::tuple <CacheLookupResult, Value> PurgableCache<Key, Value>::Get(const Key &key) {
        return nullptr;
    }

    template<typename Key, typename Value>
    void PurgableCache<Key, Value>::Flush() {

    }

    template<typename Key, typename Value>
    void PurgableCache<Key, Value>::Evict(const Key &key) {

    }

    template<typename Key, typename Value>
    void PurgableCache<Key, Value>::Replace(const Key &key) {

    }

    template<typename Key, typename Value>
    void PurgableCache<Key, Value>::Refresh(const Key &key) {

    }
}

#endif //CACHEBROWNS_PURGABLECACHE_H
