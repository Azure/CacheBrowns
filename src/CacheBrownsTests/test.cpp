// Copyright (c) Microsoft Corporation. All rights reserved.

#include <map>
#include <memory>
#include <string>

#include "../../inc/DataSource/ICacheDataSource.h"
#include "../../inc/Hydration/PullCacheHydrator.h"
#include "../../inc/ManagedCache.h"
#include "../../inc/Store/MemoryCacheStore.h"

#define CATCH_CONFIG_MAIN
#include <catch2/catch.hpp>

using namespace Microsoft::Azure::CacheBrowns;
using namespace DataSource;

class FakeDataSource : public ICacheDataSource<std::string, std::string>
{
    std::map<std::string, std::string> fakeFiles = {{"foo", "foo"}, {"bar", "bar"}};

    auto Retrieve(const std::string& key) -> std::tuple<bool, std::string> override { return {true, fakeFiles[key]}; }

    auto IsValid(const std::string& key, const std::string& value) -> bool override { return key != "foo"; }
};

TEST_CASE("placeholder", "[placeholder]")
{
    std::unique_ptr<ICacheStoreStrategy<std::string, std::string>> testStore =
            std::make_unique<MemoryCacheStore<std::string, std::string>>();
    std::unique_ptr<ICacheDataSource<std::string, std::string>> testDataSource = std::make_unique<FakeDataSource>();
    std::unique_ptr<ICacheHydrationStrategy<std::string, std::string>> testPull =
            std::make_unique<PullCacheHydrator<std::string, std::string>>(testStore, testDataSource);

    auto managedCache = std::make_unique<ManagedCache<std::string, std::string>>(testPull);

    REQUIRE(managedCache != nullptr);
}
