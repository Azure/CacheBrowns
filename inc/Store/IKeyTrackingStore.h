// Copyright (c) Microsoft Corporation. All rights reserved.

#ifndef CACHEBROWNS_IKEYTRACKINGSTORE_H
#define CACHEBROWNS_IKEYTRACKINGSTORE_H

#include "ICacheStoreStrategy.h"

namespace Microsoft::Azure::CacheBrowns::Store
{
    template<typename Key, typename Value>
    class IKeyTrackingStore : ICacheStoreStrategy<Key, Value>
    {
    public:
        std::set<Key> GetKeys();

        bool Contains(const Key& key);

        void MarkInvalid(const Key& key);

        bool IsMarkedInvalid(const Key& key);

        virtual ~IKeyTrackingStore() = default;
    };
}// namespace Microsoft::Azure::CacheBrowns::Store

#endif//CACHEBROWNS_ICACHESTORESTRATEGY_H


#endif//CACHEBROWNS_IKEYTRACKINGSTORE_H
