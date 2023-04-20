// Copyright (c) Microsoft Corporation. All rights reserved.

#include "PollingTask.h"

#include <atomic>
#include <catch2/catch.hpp>

using namespace Microsoft::Azure::Host::Common;

std::atomic<int> counter = 0;

void Increment()
{
    ++counter;
}

TEST_CASE ("Poll_Works", "[PollingTaskTests]")
{
    auto task = PollingTask(std::chrono::milliseconds(1), [&]() { Increment(); });

    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    REQUIRE(counter > 1);
}

TEST_CASE ("SetPollingRate_Works", "[PollingTaskTests]")
{
    auto task = PollingTask(std::chrono::milliseconds(1), [&]() { Increment(); });

    // Wait for task to actually run
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    task.SetPollingRate(std::chrono::seconds(5));

    // Wait for effect
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    // Observe no change in counter
    const int counterVal = counter;
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    REQUIRE(counterVal == counter.load());

    // Also that polling clean exits with such a high duration
}
