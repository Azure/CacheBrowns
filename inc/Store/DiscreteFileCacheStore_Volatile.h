// Copyright (c) Microsoft Corporation. All rights reserved.

#ifndef CACHEBROWNS_DISCRETEFILECACHESTORE_VOLATILE_H
#define CACHEBROWNS_DISCRETEFILECACHESTORE_VOLATILE_H

#include "ICacheStoreStrategy.h"

#include <algorithm>
#include <unordered_map>
#include <utility>
#include <filesystem>
#include <fstream>
#include <sstream>

#include "DiscreteFileCacheStore_NonVolatile.h"

namespace Microsoft::Azure::CacheBrowns::Store
{
    // Restrict to string, string for now since that's what is most likely to be useful for this type of store
    class DiscreteFileCacheStore_Volatile final : public ICacheStoreStrategy<std::string, std::string>
    {
        std::unique_ptr<DiscreteFileCacheStore_NonVolatile> Store;
        bool FlushOnExit;

    public:
        DiscreteFileCacheStore_Volatile(
                std::filesystem::path cacheLocation, std::string extension, bool flushOnExit);

        std::tuple<bool, std::string> Get(const std::string& key) override;

        void Set(const std::string& key, const std::string& value) override;

        auto Delete(const std::string& key) -> bool override;

        void Flush() override;

        ~DiscreteFileCacheStore_Volatile();
    };

    /// It can be useful to delay volatile flush until next boot for debugging purposes
    DiscreteFileCacheStore_Volatile::DiscreteFileCacheStore_Volatile(std::filesystem::path cacheLocation, std::string extension = "", bool flushOnExit=false) :
        Store(std::make_unique<DiscreteFileCacheStore_NonVolatile>(cacheLocation, extension)), FlushOnExit(flushOnExit)
    {
        // Call local for consistency
        DiscreteFileCacheStore_Volatile::Flush();
    }

    std::tuple<bool, std::string> DiscreteFileCacheStore_Volatile::Get(const std::string& key)
    {
        return Store->Get(key);
    }

    void DiscreteFileCacheStore_Volatile::Set(const std::string& key, const std::string& value)
    {
       Store->Set(key, value);
    }

    auto DiscreteFileCacheStore_Volatile::Delete(const std::string& key) -> bool
    {
        return Store->Delete(key);
    }

    void DiscreteFileCacheStore_Volatile::Flush()
    {
        Store->Flush();
    }

    DiscreteFileCacheStore_Volatile::~DiscreteFileCacheStore_Volatile()
    {
        if (FlushOnExit)
        {
            Store->Flush();
        }
    }

}// namespace Microsoft::Azure::CacheBrowns::Store

#endif//CACHEBROWNS_DISCRETEFILECACHESTORE_VOLATILE_H
