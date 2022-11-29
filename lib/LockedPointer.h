// Copyright (c) Microsoft Corporation. All rights reserved.

#ifndef CACHEBROWNS_LOCKEDPOINTER_H
#define CACHEBROWNS_LOCKEDPOINTER_H

#include <memory>
#include <shared_mutex>

namespace Microsoft::Azure::Host::Synchronization
{
    /// A RAII container for a pointer and associated mutex/lock. This class removes boilerplate for creating locks,
    /// and ensures that access to the underlying data is always done with a lock present.
    template<typename T, typename LockT>
    class LockedPtr
    {
        std::shared_ptr<T> Pointer;
        std::shared_ptr<std::shared_mutex> Mutex;
        LockT Lock;

    public:
        LockedPtr(std::shared_ptr<T> pointer, std::shared_ptr<std::shared_mutex> mutex) :
            Pointer(std::move(pointer)), Mutex(std::move(mutex)), Lock(std::move(LockT{*Mutex}))
        {
        }

        void unlock()
        {
            Lock.unlock();

            // Unlocking creates an abstraction leak, this object is intended to preclude unlocked access.
            // Thus, when properly used this function signals that the user is done with this LockedPtr and
            // we can release the underlying memory. This both potentially frees resources earlier and ensures
            // that misuse will result in a deterministic failure (dereferencing a null pointer) rather than
            // allowing for nondeterministic race conditions.
            Pointer = nullptr;
            Mutex = nullptr;
        }

        T& operator*() const
        {
            return *Pointer.get();
        }

        T* operator->() const
        {
            return Pointer.get();
        }

        T* get() const
        {
            return Pointer.get();
        }
    };

    template<typename T>
    using UniquelyLockedPtr = LockedPtr<T, std::unique_lock<std::shared_mutex>>;
    template<typename T>
    using SharedLockedPtr = LockedPtr<T, std::shared_lock<std::shared_mutex>>;

    /// Encapsulates a pointer to force access to occur with proper locks established.
    /// The underlying LockablePtr objects manage the lifetime of the underlying locks to reduce boilerplate.
    template<typename T>
    class LockablePtr
    {
        std::shared_ptr<std::shared_mutex> Mutex = std::make_shared<std::shared_mutex>();
        std::shared_ptr<T> Pointer;

    public:
        LockablePtr(std::unique_ptr<T>&& pointer) : Pointer(std::move(pointer)) {}

        [[nodiscard]] UniquelyLockedPtr<T> GetWithUniqueLock() const
        {
            return UniquelyLockedPtr<T>(Pointer, Mutex);
        }

        [[nodiscard]] SharedLockedPtr<T> GetWithSharedLock() const
        {
            return SharedLockedPtr<T>(Pointer, Mutex);
        }
    };
}

#endif//CACHEBROWNS_LOCKEDPOINTER_H
