// Copyright (c) Microsoft Corporation. All rights reserved.

#ifndef CACHEBROWNS_ICACHESTORESTRATEGY_H
#define CACHEBROWNS_ICACHESTORESTRATEGY_H

namespace Microsoft::Azure::CacheBrowns
{
    template<typename Key, typename Value>
    class ICacheStoreStrategy
    {
    public:
        // TODO: Iterator instead?
        virtual std::tuple<bool, Value> Get(const Key& key) = 0;

        virtual void Set(const Key& key, const Value& value) = 0;

        virtual bool Delete(const Key& key) = 0;

        virtual void Flush() = 0;
    };
}

#endif //CACHEBROWNS_ICACHESTORESTRATEGY_H
