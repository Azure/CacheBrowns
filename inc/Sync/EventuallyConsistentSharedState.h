// Copyright (c) Microsoft Corporation. All rights reserved.

#ifndef CACHEBROWNS_EVENTUALLYCONSISTENTSHAREDSTATE_H
#define CACHEBROWNS_EVENTUALLYCONSISTENTSHAREDSTATE_H

namespace Microsoft::Azure::CacheBrowns::Sync
{
    /// <summary>
    /// Shared state that is mutated only via swap operations using atomic memory operations. Reads during concurrent
    /// writes might result in stale reads, but when snapshots are requested repeatedly they will be eventually consistent.
    /// </summary>
    template<typename T>
    class EventuallyConsistentSharedState
    {
        /// <summary>
        /// Shared pointer allows us to do mostly lock-free replacement of shared state that is updated via an atomic swap in the many-readers single-writer problem. Because the new data is staged independently as a copy, the contention between threads can be reduced to a single pointer write. This abstraction enables readers to operate on a shared_ptr owning an ephemeral snapshot. Swaps can occur before the readers are done with the previous state. Without addtional synchronization.
        ///
        /// In addition to better performance, this application of eventually consistency enables atomicity. To avoid massive contention, a task that reads the shared state potentially operates on multiple versions of the state. Because we can write while readers still have a handle, we can get the best of both. Full atomicity for tasks without large contention windows.
        /// </summary>
        /// <remarks>
        /// We are implicitly leveraging memory_order_relaxed semantics here.
        /// </remarks>
        std::shared_ptr<T> SharedState;

    public:
        [[nodiscard]] auto GetEphemeralSnapshot() const
        {
            return atomic_load(&SharedState);
        }

        void SetSharedState(const std::shared_ptr<T>& sharedState)
        {
            atomic_store(&SharedState, sharedState);
        }
    };
}

#endif// CACHEBROWNS_EVENTUALLYCONSISTENTSHAREDSTATE_H
