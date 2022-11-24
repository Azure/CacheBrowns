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

add_executable(CacheBrownsTests src/CacheBrownsTests/test.cpp)
target_link_libraries(CacheBrownsTests PRIVATE Catch2::Catch2)

target_include_directories(CacheBrownsTests PRIVATE ${Catch2}/include inc)

add_executable(LibTests lib/PollingTaskTests.cpp)
target_link_libraries(LibTests PRIVATE Catch2::Catch2)

target_include_directories(LibTests PRIVATE ${Catch2}/include lib)

set_target_properties(CacheBrownsTests PROPERTIES LINKER_LANGUAGE CXX)