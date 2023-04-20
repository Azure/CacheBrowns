// Copyright (c) Microsoft Corporation. All rights reserved.

#ifndef CACHEBROWNS_EVENTUALLYCONSISTENTSHAREDSTATE_H
#define CACHEBROWNS_EVENTUALLYCONSISTENTSHAREDSTATE_H

#include <memory>
#include <shared_mutex>

/// <summary>
/// Shared state that is mutated only via swap operations using atomic memory operations. Reads during concurrent
/// writes might result in stale reads, but when snapshots are requested repeatedly they will be eventually
/// consistent.
/// </summary>
template<typename T>
class EventuallyConsistentSharedState
{
    /// <summary>
    /// Shared pointer allows us to do mostly lock-free replacement of shared state that is updated via an atomic
    /// swap in the many-readers single-writer problem. Because the new data is staged independently as a copy, the
    /// contention between threads can be reduced to a single pointer write. This abstraction enables readers to
    /// operate on a shared_ptr owning an ephemeral snapshot. Swaps can occur before the readers are done with the
    /// previous state. Without additional synchronization.
    ///
    /// In addition to better performance, this application of eventual consistency enables atomicity. To avoid
    /// massive contention, a task that reads the shared state potentially operates on multiple versions of the
    /// state. Because we can write while readers still have a handle, we can get the best of both. Full atomicity
    /// for tasks without large contention windows.
    /// </summary>
    /// <remarks>
    /// We are leveraging memory_order_relaxed semantics because we are both:
    ///     1. A single writer
    ///     2. Ok with stale reads
    ///
    /// Thus we don't care what order these operations occur in.
    /// </remarks>
    std::atomic<std::shared_ptr<T>> SharedState;

public:
    [[nodiscard]] auto GetEphemeralSnapshot() const
    {
        return SharedState.load(std::memory_order_relaxed);
    }

    /// <summary>
    /// Safely set the underlying state, which will eventually be read as a part of future snapshots.
    /// </summary>
    /// <remarks>
    /// This idiom is only safe because it's gating access to the shared state. Thus we force a moved unique_ptr
    /// to make this clear + the default is that we privately own the state.
    /// </remarks>
    void SetSharedState(std::unique_ptr<T>&& sharedState)
    {
        std::shared_ptr conversionBuffer = std::move(sharedState);

        SharedState.store(conversionBuffer, std::memory_order_relaxed);
    }
};

#endif//CACHEBROWNS_EVENTUALLYCONSISTENTSHAREDSTATE_H
