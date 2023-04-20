// Copyright (c) Microsoft Corporation. All rights reserved.

#ifndef CACHEBROWNS_POLLINGTASK_H
#define CACHEBROWNS_POLLINGTASK_H

#include <atomic>
#include <chrono>
#include <functional>
#include <mutex>

namespace Microsoft::Azure::Host::Common
{
    template<class T>
    struct is_duration : std::false_type
    {
    };
    template<class Rep, class Period>
    struct is_duration<std::chrono::duration<Rep, Period>> : std::true_type
    {
    };

    // A periodic task to be executed in a separate background thread.
    // The periodic task begins execution immediately upon construction of the object.
    template<class Duration, class = std::enable_if_t<is_duration<Duration>{}>>
    class PollingTask final
    {
    public:
        PollingTask() = delete;
        PollingTask(_In_ PollingTask&& other) = delete;
        PollingTask(_In_ const PollingTask& other) = delete;
        PollingTask& operator=(_In_ const PollingTask& other) = delete;
        PollingTask operator=(_In_ const PollingTask&& other) = delete;

        /// <summary>
        /// Constructs the polling task and starts work execution.
        /// </summary>
        /// <param name="pollingRate">The duration between each poll attempt.</param>
        /// <param name="task">The task to be executed.</param>
        PollingTask(_In_ Duration pollingRate, _In_ std::function<void()> innerTask) :
            activelyPolling(true), pollingRate(pollingRate), pollingThread(&PollingTask::Poll, this)
        {
            // Discard the bool
            task = [innerTask, &activelyPolling = activelyPolling](const std::atomic<bool>&)
            {
                innerTask();
            };
        }

        /// <summary>
        /// Constructs the polling task and starts work execution.
        /// </summary>
        /// <param name="pollingRate">The duration between each poll attempt.</param>
        /// <param name="task">The task to be executed.</param>
        PollingTask(_In_ Duration pollingRate, _In_ std::function<void(const std::atomic<bool>&)> interuptableTask) :
            activelyPolling(true), task(std::move(interuptableTask)), pollingRate(pollingRate), pollingThread(&PollingTask::Poll, this)
        {}

        ~PollingTask()
        {
            activelyPolling = false;

            pollingThreadKillSignal.notify_all();
            pollingThread.join();
        }

        void SetPollingRate(_In_ const Duration newPollingRate)
        {
            pollingRate = newPollingRate;
        }

    private:
        std::condition_variable pollingThreadKillSignal;
        std::atomic<bool> activelyPolling;
        std::function<void(const std::atomic_bool&)> task;

        std::atomic<Duration> pollingRate;
        std::thread pollingThread;

        void Poll()
        {
            std::mutex mutex;
            std::unique_lock lock(mutex);

            while (activelyPolling)
            {
                // Ignore spurious wake-ups. Used instead of sleep to allow for fast-kill
                const bool stopPolling =
                        pollingThreadKillSignal.wait_for(lock, pollingRate.load(), [&] { return !activelyPolling; });

                if (!stopPolling)
                {
                    task(activelyPolling);
                }
            }
        }
    };

    /// <summary>
    /// Like <cref="PollingTask">, but the task lambda accepts a lambda that can be called to update the
    /// polling rate. Useful when the polling rate itself is determined by the underlying task.
    /// </summary>
    template<class Duration, class = std::enable_if_t<is_duration<Duration>{}>>
    class SelfUpdatingPollingTask final
    {
    public:
        using PollingRateSetter = std::function<void(Duration)>;

        SelfUpdatingPollingTask() = delete;
        SelfUpdatingPollingTask(_In_ SelfUpdatingPollingTask&& other) = delete;
        SelfUpdatingPollingTask(_In_ const SelfUpdatingPollingTask& other) = delete;
        SelfUpdatingPollingTask& operator=(_In_ const SelfUpdatingPollingTask& other) = delete;
        SelfUpdatingPollingTask operator=(_In_ const SelfUpdatingPollingTask&& other) = delete;
        ~SelfUpdatingPollingTask() = default;

        /// <summary>
        /// Constructs the polling task and starts work execution.
        /// </summary>
        /// <param name="pollingRate">The duration between each poll attempt.</param>
        /// <param name="task">The task to be executed.</param>
        SelfUpdatingPollingTask(_In_ Duration pollingRate, _In_ const std::function<void(const PollingRateSetter&)>& task)
        {
            PollingRateSetter setPollingRateWrapper = [this](Duration duration) { Task->SetPollingRate(duration); };

            Task = std::make_unique<PollingTask<Duration>>(
                    pollingRate, [this, task, setPollingRateWrapper]() { task(setPollingRateWrapper); });
        }

        SelfUpdatingPollingTask(_In_ Duration pollingRate, _In_ const std::function<void(const PollingRateSetter&, const std::atomic<bool>&)>& task)
        {
            PollingRateSetter setPollingRateWrapper = [this](Duration duration) { Task->SetPollingRate(duration); };

            Task = std::make_unique<PollingTask<Duration>>(
                    pollingRate, [this, task, setPollingRateWrapper](const std::atomic<bool>& activelyPolling)
                    {
                        task(setPollingRateWrapper, activelyPolling);
                    });
        }

    private:
        std::unique_ptr<PollingTask<Duration>> Task;
    };

}// namespace Microsoft::Azure::Host::Common

#endif//CACHEBROWNS_POLLINGTASK_VOLATILE_H
