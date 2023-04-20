// Copyright (c) Microsoft Corporation. All rights reserved.

#ifndef CACHEBROWNS_STOREDATAVALIDOVERIDER_H
#define CACHEBROWNS_STOREDATAVALIDOVERIDER_H

#include "ICacheStoreStrategy.h"

#include <unordered_set>
#include <memory>

namespace Microsoft::Azure::CacheBrowns::Store
{
    template<typename Key, typename Value>
    class OveridableDataValidityStore final : public ICacheStoreStrategy<Key, Value>
    {
    public:
        OveridableDataValidityStore(std::unique_ptr<ICacheStoreStrategy<Key, Value>>&& store);

        std::tuple<bool, Value> Get(const Key& key) override;

        void Set(const Key& key, const Value& value) override;

        bool Delete(const Key& key) override;

        void Flush() override;
        
        void MarkInvalid(const Key& key);
        
        void IsMarkedInvalid(const Key& key);

    private:
        std::unordered_set<Key> InvalidEntries;
        std::unique_ptr<ICacheStoreStrategy<Key, Value>> Store;
    };

    template<typename Key, typename Value>
    OveridableDataValidityStore<Key, Value>::StoreDataValidOverider(std::unique_ptr<ICacheStoreStrategy<Key, Value>>&& store)
    {
        Data = std::make_unique<InternalData>(store);
    }

    template<typename Key, typename Value>
    std::tuple<bool, Value> StoreDataValidOverider<Key, Value>::Get(const Key& key)
    {
        return Store->Get(key);
    }

    template<typename Key, typename Value>
    void StoreDataValidOverider<Key, Value>::Set(const Key& key, const Value& value)
    {
        InvalidEntries->erase(key);
        Store->Set(key, value);
    }

    template<typename Key, typename Value>
    bool StoreDataValidOverider<Key, Value>::Delete(const Key& key)
    {
        InvalidEntries->erase(key);
        Store->Delete(key);
    }

    template<typename Key, typename Value>
    void StoreDataValidOverider<Key, Value>::Flush()
    {
        InvalidEntries.clear();
        Store->Flush();
    }

    template<typename Key, typename Value>
    void StoreDataValidOverider<Key, Value>::MarkInvalid(const Key& key)
    {
        InvalidEntries.insert(key);
    }

    template<typename Key, typename Value>
    void StoreDataValidOverider<Key, Value>::IsMarkedInvalid(const Key& key)
    {
        return InvalidEntries.find(key) != InvalidEntries.end();
    }
}

#endif//CACHEBROWNS_STOREDATAVALIDOVERIDER_H
