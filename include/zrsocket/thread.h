// Some compilers (e.g. VC++) benefit significantly from using this. 
// We've measured 3-4% build speed improvements in apps as a result 
#pragma once

#ifndef ZRSOCKET_THREAD_H
#define ZRSOCKET_THREAD_H
#include <thread>
#include <vector>
#include <utility>
#include "config.h"
#include "atomic.h"

ZRSOCKET_NAMESPACE_BEGIN

class ZRSOCKET_EXPORT Thread
{
public:
    Thread()
    {
        state_.store(static_cast<int>(State::READY), std::memory_order_relaxed);
    }

    ~Thread()
    {
        join();
    }

    enum class State
    {
        READY   = -1,
        STOPPED = 0,
        RUNNING = 1,
        JOINED  = 2,
    };

    template <class TFunction, class ... TArgs> 
    int start(TFunction &&f, TArgs &&... args)
    {
        if (state() == State::READY) {
            state_.store(static_cast<int>(State::RUNNING), std::memory_order_relaxed);
            thread_ = std::move(std::thread(f, std::forward<TArgs>(args)...));
            return 0;
        }
        return -1;
    }

    inline int join()
    {
        if (thread_.joinable()) {
            State thread_state = state();
            if ((thread_state == State::RUNNING) || (thread_state == State::STOPPED)) {
                state_.store(static_cast<int>(State::JOINED), std::memory_order_relaxed);
                thread_.join();
                return 0;
            }
        }
        return -1;
    }

    inline int stop()
    {
        if (state() == State::RUNNING) {
            state_.store(static_cast<int>(State::STOPPED), std::memory_order_relaxed);
            return 0;
        }
        return -1;
    }

    inline State state()
    {
        return static_cast<State>(state_.load(std::memory_order_relaxed));
    }

private:
    Thread(const Thread &) = delete;
    Thread & operator=(const Thread &) = delete;

private:
    std::thread thread_;
    AtomicInt   state_;
};

class ZRSOCKET_EXPORT ThreadGroup
{
public:
    ThreadGroup()
    {
    }

    ~ThreadGroup()
    {
        clear();
    }

    template <class TFunction, class... TArgs>
    int start(uint_t max_thread_num, uint_t init_thread_num, TFunction&& f, TArgs&&... args)
    {
        if (init_thread_num > max_thread_num) {
            init_thread_num = max_thread_num;
        }

        threads_.reserve(max_thread_num);
        for (uint_t i = 0; i < init_thread_num; ++i) {
            Thread *thread = new Thread();
            threads_.emplace_back(std::move(thread));
            thread->start(f, i, std::forward<TArgs>(args)...);
        }
        return 0;
    }

    int join()
    {
        for (auto &thread : threads_) {
            thread->join();
        }
        return 0;
    }

    int stop()
    {
        for (auto &thread : threads_) {
            thread->stop();
        }
        return 0;
    }

    int clear()
    {
        for (auto &thread : threads_) {
            thread->stop();
            thread->join();
            delete thread;
        }
        threads_.clear();
        return 0;
    }

    Thread * get_thread(uint_t index)
    {
        if (index < threads_.size()) {
            return threads_[index];
        }
        return nullptr;
    }

protected:
    std::vector<Thread * > threads_;

private:
    ThreadGroup(const ThreadGroup &) = delete;
    ThreadGroup & operator=(const ThreadGroup &) = delete;
};

ZRSOCKET_NAMESPACE_END

#endif
