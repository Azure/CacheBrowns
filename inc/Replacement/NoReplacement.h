// Copyright (c) Microsoft Corporation. All rights reserved.

#ifndef CACHEBROWNS_NOREPLACEMENT_H
#define CACHEBROWNS_NOREPLACEMENT_H

#include "ICacheReplacementStrategy.h"

namespace Microsoft::Azure::CacheBrowns
{
    template<typename Key, typename Value>
    class NoReplacement : public ICacheReplacementStrategy<Key, Value>
    {
        typedef std::unique_ptr<ICacheHydrationStrategy<Key, Value>> Hydrator;

        Hydrator cacheHydrator;

    public:
        explicit NoReplacement(Hydrator& hydrator);

        std::tuple<CacheLookupResult, Value> Get(const Key& key);

        void Invalidate(const Key& key);

        void Delete(const Key& key);

        void Flush();
    };

    template<typename Key, typename Value>
    NoReplacement<Key, Value>::NoReplacement(NoReplacement::Hydrator &hydrator)
        : cacheHydrator(std::move(hydrator))
    {

    }

    template<typename Key, typename Value>
    std::tuple<CacheLookupResult, Value> NoReplacement<Key, Value>::Get(const Key &key) {
        return cacheHydrator->Get(key);
    }

    template<typename Key, typename Value>
    void NoReplacement<Key, Value>::Invalidate(const Key &key) {
        cacheHydrator->Invalidate(key);
    }

    template<typename Key, typename Value>
    void NoReplacement<Key, Value>::Delete(const Key &key) {
        cacheHydrator->Delete(key);
    }

    template<typename Key, typename Value>
    void NoReplacement<Key, Value>::Flush() {
        cacheHydrator->Flush();
    }
}

#endif //CACHEBROWNS_NOREPLACEMENT_H
