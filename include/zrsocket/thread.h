#ifndef ZRSOCKET_THREAD_H_
#define ZRSOCKET_THREAD_H_
#include <thread>
#include <vector>
#include <utility>
#include "config.h"

ZRSOCKET_BEGIN

class ZRSOCKET_EXPORT Thread
{
public:
    Thread() : state_(Thread::READY)
    {
    }

    ~Thread()
    {
    }

    enum state
    {
        READY   = -1,
        STOPPED = 0,
        RUNNING = 1,
    };

    template <class TFunction, class ... TArgs> 
    int start(TFunction&& f, TArgs&&... args)
    {
        if (state_ == READY) {
            state_  = RUNNING;
            thread_ = std::thread(f, std::forward<TArgs>(args)...);
            return 0;
        }
        return -1;
    }

    inline int join()
    {
        thread_.join();
        state_ = STOPPED;
        return 0;
    }

    inline int stop()
    {
        if (state_ == RUNNING) { 
            state_ = STOPPED;
            return 0;
        }
        return -1;
    }

    inline int state()
    {
        return state_;
    }

private:
    Thread(const Thread &) = delete;
    Thread& operator=(const Thread &) = delete;

private:
    volatile int state_;
    std::thread  thread_;
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
            threads_.emplace_back(thread);
            thread->start(f, i, std::forward<TArgs>(args)...);
        }
        return 0;
    }

    int join()
    {
        std::vector<Thread * >::iterator iter = threads_.begin();
        std::vector<Thread * >::iterator iter_end = threads_.end();
        for (; iter != iter_end; ++iter) {
            (*iter)->join();
        }
        return 0;
    }

    int stop()
    {
        std::vector<Thread * >::iterator iter = threads_.begin();
        std::vector<Thread * >::iterator iter_end = threads_.end();
        for (; iter != iter_end; ++iter) {
            (*iter)->stop();
        }
        return 0;
    }

    int clear()
    {
        std::vector<Thread * >::iterator iter = threads_.begin();
        std::vector<Thread * >::iterator iter_end = threads_.end();
        for (; iter != iter_end; ++iter) {
            Thread *thread = *iter;
            thread->stop();
            thread->join();
            delete thread;
        }
        threads_.clear();
        return 0;
    }

    Thread* get_thread(uint_t index)
    {
        if (index < threads_.size()) {
            return threads_[index];
        }
        return NULL;
    }

protected:
    std::vector<Thread * > threads_;

private:
    ThreadGroup(const ThreadGroup &) = delete;
    ThreadGroup& operator=(const ThreadGroup &) = delete;
};

ZRSOCKET_END

#endif
