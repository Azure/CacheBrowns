// Copyright (c) Microsoft Corporation. All rights reserved.

#ifndef CACHEBROWNS_DISCRETEFILECACHESTORE_H
#define CACHEBROWNS_DISCRETEFILECACHESTORE_H

#include "ICacheStoreStrategy.h"

#include <algorithm>

#include "../../cmake-build-debug/_deps/boost-src/libs/uuid/include/boost/uuid/uuid.hpp"
#include "../../cmake-build-debug/_deps/boost-src/libs/uuid/include/boost/uuid/uuid_generators.hpp"
#include "../../cmake-build-debug/_deps/boost-src/libs/uuid/include/boost/uuid/uuid_io.hpp"
//#include <boost/uuid/uuid.hpp>
//#include <boost/uuid/uuid_generators.hpp>
//#include <boost/uuid/uuid_io.hpp>
#include <unordered_map>
#include <utility>

namespace Microsoft::Azure::CacheBrowns::Store
{
    // Restrict to string, string for now since that's what is most likely to be useful for this type of store
    class DiscreteFileCacheStore : public ICacheStoreStrategy<std::string, std::string>
    {
        std::unordered_map<Key, Value> data;
        std::filesystem::path cacheDirectory;
        std::string fileExtension;

        static auto uuidGenerator = boost::uuids::random_generator();

    public:
        DiscreteFileCacheStore(
                std::filesystem::path cacheLocation = []() {

                }(),
                std::string extension = "");

        std::tuple<bool, Value> Get(const Key& key) override;

        void Set(const Key& key, const Value& value) override;

        auto Delete(const Key& key) -> bool override;

        void Flush() override;

        ~DiscreteFileCacheStore();

    private:
        std::filesystem::path GetPath(std::string key);
    };

    DiscreteFileCacheStore::DiscreteFileCacheStore(std::filesystem::path cacheLocation, std::string extension = "") :
        cacheDirectory(std::move(cacheLocation)), fileExtension(std::move(extension))
    {
        std::filesystem::create_directory(cacheDirectory);
    }

    std::tuple<bool, Value> DiscreteFileCacheStore::Get(const Key& key)
    {
        return
    }

    void DiscreteFileCacheStore::Set(const Key& key, const Value& value)
    {
        data[key] = value;
    }

    auto DiscreteFileCacheStore::Delete(const Key& key) -> bool
    {
        return std::filesystem::remove(GetPath(key));
    }

    void DiscreteFileCacheStore::Flush()
    {
        std::filesystem::remove_all(cacheDirectory);
        std::filesystem::create_directory(cacheDirectory);
    }

    DiscreteFileCacheStore::~DiscreteFileCacheStore()
    {
        std::filesystem::remove_all(cacheDirectory);
    }

    std::filesystem::path DiscreteFileCacheStore::GetPath(std::string key)
    {
        return cacheDirectory / key / fileExtension;
    }

}// namespace Microsoft::Azure::CacheBrowns::Store

#endif//CACHEBROWNS_DISCRETEFILECACHESTORE_H
