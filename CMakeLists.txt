cmake_minimum_required(VERSION 3.19)
project(Cachebrowns)

set(CMAKE_CXX_STANDARD 17)

file(GLOB cachebrowns_src
        "inc/*.h"
        "inc/**/*.h"
        )

Include(FetchContent)

FetchContent_Declare(
        Catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG v2.13.1)

FetchContent_MakeAvailable(Catch2)

set(BOOST_ENABLE_CMAKE ON)

FetchContent_Declare(
        Boost
        GIT_REPOSITORY https://github.com/boostorg/boost.git
        GIT_SUBMODULES libs/uuid
        GIT_TAG boost-1.76.0
)

#FetchContent_MakeAvailable(Boost)

add_executable(CacheBrownsTests src/CacheBrownsTests/test.cpp ${cachebrowns_src} inc/Store/IHydratable.h inc/Store/IPrunable.h src/CacheBrownsTests/CrpDataSource.cpp src/CacheBrownsTests/CrpDataSource.h src/CacheBrownsTests/VdcDataSource.cpp src/CacheBrownsTests/VdcDataSource.h inc/Store/DiscreteFileCacheStore.h)
target_link_libraries(CacheBrownsTests PRIVATE Catch2::Catch2)

target_include_directories(CacheBrownsTests PRIVATE ${Catch2}/include ${BoostUuid}/include)

set_target_properties(CacheBrownsTests PROPERTIES LINKER_LANGUAGE CXX)