// Copyright (c) Microsoft Corporation. All rights reserved.

#ifndef CACHEBROWNS_STOREDATAKEYTRACKER_H
#define CACHEBROWNS_STOREDATAKEYTRACKER_H

#include "StoreDataValidOverider.h"
#include "ICacheStoreStrategy.h"

#include <set>
#include <memory>

namespace Microsoft::Azure::CacheBrowns::Store
{
    template<typename Key, typename Value>
    class KeyTrackedStore final : public ICacheStoreStrategy<Key, Value>
    {
    public:
        StoreDataKeyTracker(std::unique_ptr<StoreDataValidOverider<Key, Value>>&& store);

        std::tuple<bool, Value> Get(const Key& key) override;

        void Set(const Key& key, const Value& value) override;

        bool Delete(const Key& key) override;

        void Flush() override;

        std::set<Key> GetKeys();

        bool Contains(const Key& key);

        void MarkInvalid(const Key& key);

        bool IsMarkedInvalid(const Key& key);

    private:
        
        std::set<Key> Keys;
        std::unique_ptr<ICacheStoreStrategy<Key, Value>> Store;
    };

    template<typename Key, typename Value>
    StoreDataKeyTracker<Key, Value>::StoreDataKeyTracker(std::unique_ptr<StoreDataValidOverider<Key, Value>>&& store)
    {
        Data = std::make_unique<InternalData>(store);
    }

    template<typename Key, typename Value>
    std::tuple<bool, Value> StoreDataKeyTracker<Key, Value>::Get(const Key& key)
    {
        return Store->Get(key);
    }

    template<typename Key, typename Value>
    void StoreDataKeyTracker<Key, Value>::Set(const Key& key, const Value& value)
    {
        Keys.insert(key);
        Store->Set(key, value);
    }

    template<typename Key, typename Value>
    bool StoreDataKeyTracker<Key, Value>::Delete(const Key& key)
    {
        Keys->erase(key);
        return Store->Delete(key);
    }

    template<typename Key, typename Value>
    void StoreDataKeyTracker<Key, Value>::Flush()
    {
        Keys.clear();
        Store->Flush();
    }

    template<typename Key, typename Value>
    std::set<Key> StoreDataKeyTracker<Key, Value>::GetKeys()
    {
        return Keys;
    }

    template<typename Key, typename Value>
    bool StoreDataKeyTracker<Key, Value>::Contains(const Key& key)
    {
        return Keys.find(key) != Keys.end();
    }
    template<typename Key, typename Value>
    void StoreDataKeyTracker<Key, Value>::MarkInvalid(const Key& key)
    {
        Store->MarkInvalid(key);
    }
    template<typename Key, typename Value>
    bool StoreDataKeyTracker<Key, Value>::IsMarkedInvalid(const Key& key)
    {
        return Store->IsMarkedInvalid(key);
    }
}

#endif//CACHEBROWNS_STOREDATAKEYTRACKER_H
