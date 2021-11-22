// Copyright (c) Microsoft Corporation. All rights reserved.

#ifndef CACHEBROWNS_ICACHEDATASOURCE_H
#define CACHEBROWNS_ICACHEDATASOURCE_H

#include "IRetrievable.h"

#include <functional>

namespace Microsoft::Azure::CacheBrowns::DataSource
{
    using namespace DataSource;

    template<typename Key, typename Value>
    class ICacheDataSource : public IRetrievable<Key, Value>
    {

    public:
        virtual ~ICacheDataSource() = default;
        virtual bool IsValid(const Key& key, const Value& value) = 0;
    };
}// namespace Microsoft::Azure::CacheBrowns::DataSource

#endif//CACHEBROWNS_ICACHEDATASOURCE_H
