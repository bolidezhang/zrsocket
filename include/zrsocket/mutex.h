// Some compilers (e.g. VC++) benefit significantly from using this. 
// We've measured 3-4% build speed improvements in apps as a result 
#pragma once

#ifndef ZRSOCKET_MUTEX_H
#define ZRSOCKET_MUTEX_H
#include <mutex>
#include <condition_variable>
#include "config.h"
#include "atomic.h"

ZRSOCKET_NAMESPACE_BEGIN

class ZRSOCKET_EXPORT NullMutex
{
public:
    inline NullMutex() = default;
    inline ~NullMutex() = default;

    inline void lock()
    {
    }

    inline void unlock()
    {
    }

    inline bool try_lock()
    {
        return true;
    }

private:
    NullMutex(const NullMutex &) = delete;
    NullMutex(NullMutex &&) = delete;
    NullMutex & operator=(const NullMutex &) = delete;
    NullMutex & operator=(NullMutex &&) = delete;
};

class ZRSOCKET_EXPORT AtomicFlagSpinlockMutex
{
public:
    inline AtomicFlagSpinlockMutex() = default;
    inline ~AtomicFlagSpinlockMutex() = default;

    inline void lock() noexcept
    {
        while (flag_.test_and_set(std::memory_order_acquire));
    }

    inline void unlock() noexcept
    {
        flag_.clear(std::memory_order_release);
    }

    inline bool try_lock() noexcept
    {
        return !flag_.test_and_set(std::memory_order_acquire);
    }

private:
    AtomicFlagSpinlockMutex(const AtomicFlagSpinlockMutex &) = delete;
    AtomicFlagSpinlockMutex(AtomicFlagSpinlockMutex &&) = delete;
    AtomicFlagSpinlockMutex & operator=(const AtomicFlagSpinlockMutex &) = delete;
    AtomicFlagSpinlockMutex & operator=(AtomicFlagSpinlockMutex &&) = delete;

    AtomicFlag flag_ = ATOMIC_FLAG_INIT;
};

template <class TAtomic>
class ZRSOCKET_EXPORT AtomicSpinlockMutex
{
public:
    inline AtomicSpinlockMutex() = default;
    inline ~AtomicSpinlockMutex() = default;

    inline void lock() noexcept
    {
        while (atomic_.exchange(1, std::memory_order_acquire));
    }

    inline void unlock() noexcept
    {
        atomic_.store(0, std::memory_order_release);
    }

    inline bool try_lock() noexcept
    {
        return !atomic_.exchange(1, std::memory_order_acquire);
    }

private:
    AtomicSpinlockMutex(const AtomicSpinlockMutex &) = delete;
    AtomicSpinlockMutex(AtomicSpinlockMutex &&) = delete;
    AtomicSpinlockMutex & operator=(const AtomicSpinlockMutex &) = delete;
    AtomicSpinlockMutex & operator=(AtomicSpinlockMutex &&) = delete;

    TAtomic atomic_ = ATOMIC_VAR_INIT(0);
};

#ifndef ZRSOCKET_OS_WINDOWS
#include <pthread.h>
class PthreadSpinlockMutex
{
public:
    inline PthreadSpinlockMutex(int pshared = PTHREAD_PROCESS_PRIVATE)
    {
        pthread_spin_init(&spin_lock_, pshared);
    }

    inline ~PthreadSpinlockMutex()
    {
        pthread_spin_destroy(&spin_lock_);
    }

    inline void lock()
    {
        pthread_spin_lock(&spin_lock_);
    }

    inline void unlock()
    {
        pthread_spin_unlock(&spin_lock_);
    }

    inline bool trylock()
    {
        return (pthread_spin_trylock(&spin_lock_) == 0);
    }

private:
    PthreadSpinlockMutex(const PthreadSpinlockMutex &) = delete;
    PthreadSpinlockMutex(PthreadSpinlockMutex &&) = delete;
    PthreadSpinlockMutex & operator=(const PthreadSpinlockMutex &) = delete;
    PthreadSpinlockMutex & operator=(PthreadSpinlockMutex &&) = delete;

    pthread_spinlock_t spin_lock_;
};
#endif

using AtomicBoolSpinlockMutex   = AtomicSpinlockMutex<AtomicBool>;
using AtomicCharSpinlockMutex   = AtomicSpinlockMutex<AtomicChar>;
using AtomicShortSpinlockMutex  = AtomicSpinlockMutex<AtomicShort>;
using AtomicIntSpinlockMutex    = AtomicSpinlockMutex<AtomicInt>;
using AtomicLongSpinlockMutex   = AtomicSpinlockMutex<AtomicLong>;
using AtomicLLongSpinlockMutex  = AtomicSpinlockMutex<AtomicLLong>;

using Mutex         = std::mutex;
using ThreadMutex   = std::mutex;
using Condition     = std::condition_variable;

#ifdef ZRSOCKET_OS_WINDOWS
using SpinlockMutex = AtomicFlagSpinlockMutex;
using SpinMutex     = AtomicFlagSpinlockMutex;
#else 
using SpinlockMutex = PthreadSpinlockMutex;
using SpinMutex     = PthreadSpinlockMutex;
#endif

template <typename TMutex>
using LockGuard = std::lock_guard<TMutex>;

#define zrsocket_lockguard(TMutex, mutex, lock) std::lock_guard<TMutex> lock(mutex)

ZRSOCKET_NAMESPACE_END

#endif
