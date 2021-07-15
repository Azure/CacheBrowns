// Copyright (c) Microsoft Corporation. All rights reserved.

#ifndef CACHEBROWNS_MEMORYCACHESTORE_H
#define CACHEBROWNS_MEMORYCACHESTORE_H

#include <unordered_map>

#include "ICacheStoreStrategy.h"

namespace Microsoft::Azure::CacheBrowns::Store
{
    template<typename Key, typename Value>
    class MemoryCacheStore : public ICacheStoreStrategy<Key, Value>
    {
        std::unordered_map<Key, Value> data;

    public:
        std::tuple<bool, Value> Get(const Key& key) override;

        void Set(const Key& key, const Value& value) override;

        bool Delete(const Key& key) override;

        void Flush() override;
    };

    template<typename Key, typename Value>
    std::tuple<bool, Value> MemoryCacheStore<Key, Value>::Get(const Key& key)
    {
        auto valueItr = data.find(key);
        auto found = valueItr == data.end();

        return {found, found ? valueItr->second : Value{}};
    }

    template<typename Key, typename Value>
    void MemoryCacheStore<Key, Value>::Set(const Key& key, const Value& value)
    {
        data[key] = value;
    }

    template<typename Key, typename Value>
    bool MemoryCacheStore<Key, Value>::Delete(const Key& key)
    {
        return data.erase(key) > 0;
    }

    template<typename Key, typename Value>
    void MemoryCacheStore<Key, Value>::Flush()
    {
        data.clear();
    }
}// namespace Microsoft::Azure::CacheBrowns::Store

#endif//CACHEBROWNS_MEMORYCACHESTORE_H
