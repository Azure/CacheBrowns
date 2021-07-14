cmake_minimum_required(VERSION 3.19)
project(Cachebrowns)

set(CMAKE_CXX_STANDARD 17)

add_library(Cachebrowns inc/IManagedCache.h inc/ICacheHydrationStrategy.h inc/ICacheReplacementStrategy.h inc/PurgableCache.h inc/ManagedCache.h inc/ICacheStoreStrategy.h inc/ICacheDataSource.h inc/MemoryCacheStore.h inc/PullCacheHydrator.h inc/PollingCacheHydrator.h inc/IRetrievable.h inc/NoReplacement.h src/CacheBrownsTests/test.cpp )

set_target_properties(Cachebrowns PROPERTIES LINKER_LANGUAGE CXX)