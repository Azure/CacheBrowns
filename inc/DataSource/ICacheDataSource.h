// Copyright (c) Microsoft Corporation. All rights reserved.

#ifndef CACHEBROWNS_ICACHEDATASOURCE_H
#define CACHEBROWNS_ICACHEDATASOURCE_H

#include "IRetrievable.h"

#include <functional>

namespace Microsoft::Azure::CacheBrowns::DataSource
{
    using namespace DataSource;

    template<typename Key, typename Value>
    class ICacheDataSource
    {
    public:
        virtual std::tuple<bool, Value> Retrieve(const Key& key) = 0;

        /// Accepts current value if one exists in case it can be used for optimized load
        /// ex. If building an HTTP cache, and you receive a 304 response, replay the current value
        virtual std::tuple<bool, Value> Retrieve(const Key& key, const Value& currentValue);

        virtual bool IsValid(const Key& key, const Value& value) = 0;

        virtual ~ICacheDataSource() = default;
    };

    /// The default case is no current value based optimization, so provide pass-through implementation
    template<typename Key, typename Value>
    std::tuple<bool, Value> ICacheDataSource<Key, Value>::Retrieve(const Key& key, const Value&)
    {
        return Retrieve(key);
    }
}// namespace Microsoft::Azure::CacheBrowns::DataSource

#endif//CACHEBROWNS_ICACHEDATASOURCE_H
