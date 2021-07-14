// Copyright (c) Microsoft Corporation. All rights reserved.

#ifndef CACHEBROWNS_ICACHEDATASOURCE_H
#define CACHEBROWNS_ICACHEDATASOURCE_H

#include "IRetrievable.h"

namespace Microsoft::Azure::CacheBrowns
{
    template<typename Key, typename Value>
    class ICacheDataSource : public IRetrievable<Key, Value>
    {
    public:
        virtual std::tuple<bool, Value> Retrieve(const Key& key) = 0;

        virtual bool IsValid(const Key& key, const Value& value) = 0;
    };
}

#endif //CACHEBROWNS_ICACHEDATASOURCE_H
