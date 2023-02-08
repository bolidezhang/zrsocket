// Some compilers (e.g. VC++) benefit significantly from using this. 
// We've measured 3-4% build speed improvements in apps as a result 
#pragma once

#ifndef ZRSOCKET_LOCKFREE_QUEUE_H
#define ZRSOCKET_LOCKFREE_QUEUE_H
#include <deque>
#include <queue>
#include "atomic.h"

ZRSOCKET_NAMESPACE_BEGIN

// 双指针交换队列: double pointer swap queue
//  用于单消费者场景
template <typename T, typename TMutex = SpinlockMutex, typename TContainer = std::deque<T> >
class DoublePointerSwapQueue
{
public:
    inline DoublePointerSwapQueue()
    {
        active_queue_   = &queue1_;
        standby_queue_  = &queue2_;
    }
    inline ~DoublePointerSwapQueue() = default;
    
    inline int push(const T &value)
    {
        mutex_.lock();
        standby_queue_->push_back(value);
        mutex_.unlock();

        return 1;
    }

    inline int push(T &&value)
    {
        mutex_.lock();
        standby_queue_->push_back(value);
        mutex_.unlock();

        return 1;
    }

    //取队列头
    //只能在消费者线程调用
    inline T* front()
    {
        if (!active_queue_->empty()) {
            return &active_queue_->front();
        }

        return nullptr;
    }

    //删除队列头
    //只能在消费者线程调用
    inline int pop()
    {
        active_queue_->pop_front();
        return 1;
    }

    //交换active_queue_/standby_queue_指针
    //只能在消费者线程且active_queue_->empty()==true时调用
    inline bool swap_pointer()
    {
        mutex_.lock();
        if (!standby_queue_->empty()) {
            std::swap(standby_queue_, active_queue_);
            mutex_.unlock();
            return true;
        }
        mutex_.unlock();

        return false;
    }

    inline const TContainer* active_queue() const
    {
        return active_queue_;
    }

protected:
    TContainer *active_queue_;
    TContainer *standby_queue_;
    TContainer  queue1_;
    TContainer  queue2_;
    TMutex      mutex_;
};

ZRSOCKET_NAMESPACE_END

#endif
