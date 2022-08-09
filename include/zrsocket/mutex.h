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

#define zrsocket_lockguard(TMutex, mutex, lock) std::lock_guard<TMutex> lock(mutex)
typedef std::mutex ThreadMutex;
typedef std::mutex Mutex;
typedef std::condition_variable Condition;

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

class ZRSOCKET_EXPORT SpinMutex
{
public:
    inline SpinMutex() = default;
    inline ~SpinMutex() = default;

    inline void lock() noexcept
    {
        while (spin_mutex_.test_and_set(std::memory_order_acquire));
    }

    inline void unlock() noexcept
    {
        spin_mutex_.clear(std::memory_order_release);
    }

    inline bool try_lock() noexcept
    {
        return !spin_mutex_.test_and_set(std::memory_order_acquire);
    }

private:
    SpinMutex(const SpinMutex &) = delete;
    SpinMutex(SpinMutex &&) = delete;
    SpinMutex & operator=(const SpinMutex &) = delete;
    SpinMutex & operator=(SpinMutex &&) = delete;

    std::atomic_flag spin_mutex_ = ATOMIC_FLAG_INIT;
};

ZRSOCKET_NAMESPACE_END

#endif
