#ifndef ZRSOCKET_MUTEX_H_
#define ZRSOCKET_MUTEX_H_
#include <mutex>
#include "config.h"
#include "atomic.h"

ZRSOCKET_BEGIN

#define zrsocket_lockguard(TMutex, mutex, lock) std::lock_guard<TMutex> lock(mutex)
typedef std::mutex ThreadMutex;

class ZRSOCKET_EXPORT NullMutex
{
public:
    inline NullMutex()
    {
    }

    inline ~NullMutex()
    {
    }

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
    NullMutex& operator=(const NullMutex &) = delete;
};

class ZRSOCKET_EXPORT SpinMutex
{
public:
    inline SpinMutex()
    {
    }

    inline ~SpinMutex()
    {
    }

    inline void lock()
    {
        while (spin_mutex_.test_and_set(std::memory_order_acquire)) {
        };
    }

    inline void unlock()
    {
        spin_mutex_.clear(std::memory_order_release);
    }

    inline bool try_lock()
    {
        if (spin_mutex_.test_and_set(std::memory_order_acquire)) {
            return true;
        }

        return false;
    }

private:
    SpinMutex(const SpinMutex &) = delete;
    SpinMutex& operator=(const SpinMutex &) = delete;

    std::atomic_flag spin_mutex_ = ATOMIC_FLAG_INIT;
};

ZRSOCKET_END

#endif
