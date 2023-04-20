// Copyright (c) Microsoft Corporation. All rights reserved.

#include "LockedPointer.h"

#include <catch2/catch.hpp>
#include <map>

using namespace Microsoft::Azure::Host::Synchronization;

TEST_CASE ("ParallelReads_SharedAccess_NoAccessViolations", "[LockedPointerTests]")
{
    const std::map<int, int> BaseMap = { { 1, 1 }, { 2, 2 }, { 3, 3 } };
    LockablePtr<std::map<int, int>> TrackedMachines = std::make_unique<std::map<int, int>>(BaseMap);

    const auto lock1 = TrackedMachines.GetWithSharedLock();
    const auto lock2 = TrackedMachines.GetWithSharedLock();

    REQUIRE(lock1->find(1)->second == lock2->find(1)->second);
}

TEST_CASE ("ParallelReads_RandomAccess_NoAccessViolations", "[LockedPointerTests]")
{
    LockablePtr<std::map<int, int>> TrackedMachines = std::make_unique<std::map<int, int>>();

    constexpr int threadCount = 6;
    constexpr int readCount = 1000;

    std::vector<std::thread> threads;

    // This is a workaround because catch2 can't assert in multiple threads...
    std::atomic<size_t> successCounter = 0;

    for (int i = 0; i < threadCount; i++)
    {
        const int index = i % threadCount;
        (*TrackedMachines.GetWithUniqueLock())[index] = index;

        threads.emplace_back(
                [readCount, index, &TrackedMachines, &successCounter]()
                {
                    for (int j = 0; j < readCount; j++)
                    {
                        if (j % 2 == 0)
                        {
                            const auto machines = TrackedMachines.GetWithSharedLock();
                            if (index == machines->find(index)->second)
                            {
                                successCounter++;
                            }

                        }
                        else
                        {
                            const auto machines = TrackedMachines.GetWithUniqueLock();
                            if (index == machines->find(index)->second)
                            {
                                successCounter++;
                            }
                        }
                    }
                });
    }

    for (auto& thread : threads)
    {
        thread.join();
    }

    REQUIRE(successCounter == threadCount * readCount);
}

TEST_CASE ("ParallelWrites_NoAccessViolations", "[LockedPointerTests]")
{
    LockablePtr<std::map<int, int>> TrackedMachines = std::make_unique<std::map<int, int>>();

    constexpr int threadCount = 6;

    std::vector<std::thread> threads;

    for (int i = 0; i < threadCount; i++)
    {
        constexpr int writeCount = 1000;
        threads.emplace_back(
                [threadCount, i, &TrackedMachines, writeCount]()
                {
                    for (int j = 0; j < writeCount; j++)
                    {
                        auto machines = TrackedMachines.GetWithUniqueLock();
                        (*machines)[j % threadCount] = i;
                    }
                });
    }

    for (auto& thread : threads)
    {
        thread.join();
    }

    REQUIRE_FALSE(TrackedMachines.GetWithSharedLock()->empty());
}

TEST_CASE ("ParallelReadsAndWrites_NoAccessViolations", "[LockedPointerTests]")
{
    LockablePtr<std::map<int, int>> TrackedMachines = std::make_unique<std::map<int, int>>();

    constexpr int threadCount = 6;
    constexpr int operationCount = 1000;

    // This is a workaround because catch2 can't assert in multiple threads...
    std::atomic<size_t> successCounter = 0;

    std::vector<std::thread> threads;

    for (int i = 0; i < threadCount; i++)
    {
        const int index = i % threadCount;
        (*TrackedMachines.GetWithUniqueLock())[index] = index;

        threads.emplace_back(
                [operationCount, index, &successCounter, &TrackedMachines, &threadCount]()
                {
                    for (int j = 0; j < operationCount; j++)
                    {
                        if (j % 2 == 0)
                        {
                            const auto machines = TrackedMachines.GetWithSharedLock();
                            if (machines->find(index) != machines->end())
                            {
                                successCounter++;
                            }
                        }
                        else
                        {
                            auto machines = TrackedMachines.GetWithUniqueLock();
                            (*machines)[j % threadCount] = j;
                        }
                    }
                });
    }

    for (auto& thread : threads)
    {
        thread.join();
    }

    REQUIRE(successCounter == threadCount * operationCount / 2);
    REQUIRE_FALSE(TrackedMachines.GetWithSharedLock()->empty());
}
