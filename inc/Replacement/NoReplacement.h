// Copyright (c) Microsoft Corporation. All rights reserved.

#ifndef CACHEBROWNS_NOREPLACEMENT_H
#define CACHEBROWNS_NOREPLACEMENT_H

#include "ICacheReplacementStrategy.h"

namespace Microsoft::Azure::CacheBrowns::Replacement
{
    using namespace Hydration;

    /// When a cache is permitted to grow without bound, there is no need for a replacement strategy.
    template<typename Key, typename Value>
    class NoReplacement : public ICacheReplacementStrategy<Key, Value>
    {
        typedef std::unique_ptr<ICacheHydrationStrategy<Key, Value>> Hydrator;
        typedef std::shared_ptr<IPrunable<Key>> cacheStorePtr;

        Hydrator cacheHydrator;
        cacheStorePtr dataStore;

    public:
        explicit NoReplacement(Hydrator& hydrator, cacheStorePtr store);

        std::tuple<CacheLookupResult, Value> Get(const Key& key);

        void Invalidate(const Key& key);

        void Delete(const Key& key);

        void Flush();
    };

    template<typename Key, typename Value>
    NoReplacement<Key, Value>::NoReplacement(NoReplacement::Hydrator& hydrator, cacheStorePtr store) :
        cacheHydrator(std::move(hydrator)), dataStore(store)
    {
    }

    template<typename Key, typename Value>
    std::tuple<CacheLookupResult, Value> NoReplacement<Key, Value>::Get(const Key& key)
    {
        return cacheHydrator->Get(key);
    }

    template<typename Key, typename Value>
    void NoReplacement<Key, Value>::Invalidate(const Key& key)
    {
        cacheHydrator->HandleInvalidate(key);
    }

    template<typename Key, typename Value>
    void NoReplacement<Key, Value>::Delete(const Key& key)
    {
        dataStore->Delete(key);
        cacheHydrator->HandleDelete(key);
    }

    template<typename Key, typename Value>
    void NoReplacement<Key, Value>::Flush()
    {
        dataStore->Flush();
        cacheHydrator->HandleFlush();
    }
}// namespace Microsoft::Azure::CacheBrowns::Replacement

#endif//CACHEBROWNS_NOREPLACEMENT_H
