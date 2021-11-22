// Copyright (c) Microsoft Corporation. All rights reserved.

#ifndef CACHEBROWNS_IHYDRATABLE_H
#define CACHEBROWNS_IHYDRATABLE_H

namespace Microsoft::Azure::CacheBrowns::Store
{
    template<typename Key, typename Value>
    class IHydratable
    {
    public:
        virtual ~IHydratable() = default;

        // TODO: Iterator instead
        virtual std::tuple<bool, Value> Get(const Key& key) = 0;

        virtual void Set(const Key& key, const Value& value) = 0;
    };
}// namespace Microsoft::Azure::CacheBrowns::Store

#endif//CACHEBROWNS_IHYDRATABLE_H
