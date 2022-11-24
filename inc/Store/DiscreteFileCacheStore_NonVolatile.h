// Copyright (c) Microsoft Corporation. All rights reserved.

#ifndef CACHEBROWNS_DISCRETEFILECACHESTORE_NONVOLATILE_H
#define CACHEBROWNS_DISCRETEFILECACHESTORE_NONVOLATILE_H

#include "ICacheStoreStrategy.h"

#include <algorithm>
#include <unordered_map>
#include <utility>
#include <filesystem>
#include <fstream>
#include <sstream>

namespace Microsoft::Azure::CacheBrowns::Store
{
    // Restrict to string, string for now since that's what is most likely to be useful for this type of store
    class DiscreteFileCacheStore_NonVolatile final : public ICacheStoreStrategy<std::string, std::string>
    {
        std::filesystem::path CacheDirectory;
        std::string FileExtension;
        bool FlushOnExit;

    public:
        DiscreteFileCacheStore_NonVolatile(
                std::filesystem::path cacheLocation, std::string extension);

        std::tuple<bool, std::string> Get(const std::string& key) override;

        void Set(const std::string& key, const std::string& value) override;

        auto Delete(const std::string& key) -> bool override;

        void Flush() override;

        ~DiscreteFileCacheStore_NonVolatile();

    private:
        std::filesystem::path GetPath(std::string key);
    };

    DiscreteFileCacheStore_NonVolatile::DiscreteFileCacheStore_NonVolatile(std::filesystem::path cacheLocation, std::string extension = "") :
        CacheDirectory(std::move(cacheLocation)),
        FileExtension(std::move(extension))
    {
    }

    std::tuple<bool, std::string> DiscreteFileCacheStore_NonVolatile::Get(const std::string& key)
    {
        std::ifstream file(GetPath(key));

        if (file.fail())
        {
            return { false, {} };
        }

        std::stringstream buffer;
        buffer << file.rdbuf();

        return { true, buffer.str() };
    }

    void DiscreteFileCacheStore_NonVolatile::Set(const std::string& key, const std::string& value)
    {
        std::ofstream file(GetPath(key), std::ios::trunc);
        file << value;
        file.close();
    }

    auto DiscreteFileCacheStore_NonVolatile::Delete(const std::string& key) -> bool
    {
        return std::filesystem::remove(GetPath(key));
    }

    void DiscreteFileCacheStore_NonVolatile::Flush()
    {
        std::filesystem::remove_all(CacheDirectory);
        std::filesystem::create_directory(CacheDirectory);
    }

    DiscreteFileCacheStore_NonVolatile::~DiscreteFileCacheStore_NonVolatile()
    {
    }

    std::filesystem::path DiscreteFileCacheStore_NonVolatile::GetPath(std::string key)
    {
        return CacheDirectory / (key + FileExtension);
    }

}// namespace Microsoft::Azure::CacheBrowns::Store

#endif//CACHEBROWNS_DISCRETEFILECACHESTORE_NONVOLATILE_H
