// Copyright (c) Microsoft Corporation. All rights reserved.

#ifndef CACHEBROWNS_IRETRIEVABLE_H
#define CACHEBROWNS_IRETRIEVABLE_H

namespace Microsoft::Azure::CacheBrowns::DataSource
{
    template<typename Key, typename Value>
    class IRetrievable
    {
    public:
        virtual std::tuple<bool, Value> Retrieve(const Key& key) = 0;
    };
}// namespace Microsoft::Azure::CacheBrowns::DataSource

#endif//CACHEBROWNS_IRETRIEVABLE_H
