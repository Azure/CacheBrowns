// Copyright (c) Microsoft Corporation. All rights reserved.

#define CATCH_CONFIG_RUNNER
#include <catch2/catch.hpp>

int main( int argc, char* argv[] ) {
    int result = Catch::Session().run( argc, argv );

    return result;
}
