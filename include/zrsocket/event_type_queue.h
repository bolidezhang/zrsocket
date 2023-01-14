// Some compilers (e.g. VC++) benefit significantly from using this. 
// We've measured 3-4% build speed improvements in apps as a result 
#pragma once

#ifndef ZRSOCKET_EVENT_TYPE_QUEUE_H
#define ZRSOCKET_EVENT_TYPE_QUEUE_H
#include <cmath>
#include "config.h"
#include "base_type.h"
#include "event_type.h"
#include "malloc.h"
#include "memory.h"

ZRSOCKET_NAMESPACE_BEGIN

class EventTypeQueue
{
public:
    inline EventTypeQueue()
        : pool_(nullptr)
        , capacity_(0)
        , queue_size_(0)
        , event_type_len_(0)
        , write_idx_(0)
        , read_idx_(0)
    {
    }

    virtual ~EventTypeQueue()
    {
        if (nullptr != pool_) {
            zrsocket_free(pool_);
            pool_ = nullptr;
        }
    }

    inline int init(uint_t capacity, int event_type_len = 8)
    {
        if (capacity < 1) {
            return 0;
        }
        clear();

        //计算大于等于capacity的最小 2的N次方整数
        auto log2x = std::log2(capacity);
        capacity = static_cast<uint_t>(std::pow(2, std::ceil(log2x)));

        uint_t pool_size = capacity * event_type_len;
        pool_ = (char *)zrsocket_malloc(pool_size);
        if (nullptr != pool_) {
            capacity_       = capacity;
            event_type_len_ = event_type_len;
            return 1;
        }

        return 0;
    }

    inline void clear()
    {
        if (nullptr != pool_) {
            zrsocket_free(pool_);
            pool_ = nullptr;
        }
        capacity_       = 0;
        queue_size_     = 0;
        event_type_len_ = 0;
        write_idx_      = 0;
        read_idx_       = 0;
    }

    inline int push(const EventType *event)
    {
        if (queue_size_ < capacity_) {
            ++queue_size_;
            zrsocket_memcpy((pool_ + write_idx_ * event_type_len_), event->event_ptr(), event->event_len());
            write_idx_ = (write_idx_ + 1) & (capacity_ - 1);
            return queue_size_;
        }
        return 0;
    }

    inline EventType * pop()
    {
        if (queue_size_ > 0) {
            --queue_size_;
            EventType *event = (EventType *)(pool_ + read_idx_ * event_type_len_);
            read_idx_ = (read_idx_ + 1) & (capacity_ - 1);
            return event;
        }
        return nullptr;
    }

    inline int pop(EventTypeQueue *push_queue, uint_t batch_size)
    {
        uint_t push_free_size = push_queue->free_size();
        if ((queue_size_ < 1) || (push_free_size < 1)) {
            return 0;
        }

        uint_t pop_size = ZRSOCKET_MIN(queue_size_, push_free_size);
        pop_size = ZRSOCKET_MIN(batch_size, pop_size);
        for (uint_t i = 0; i < pop_size; ++i) {
            push_queue->push(pop());
        }
        return pop_size;
    }

    inline uint_t capacity() const
    {
        return capacity_;
    }

    inline uint_t size() const
    {
        return queue_size_;
    }

    inline uint_t free_size() const
    {
        return capacity_ - queue_size_;
    }

    inline bool empty() const
    {
        return 0 == queue_size_;
    }

private:
    char   *pool_;              //事件类型数组开始位
    uint_t  capacity_;          //队列容量
    uint_t  queue_size_;        //队列大小
    uint_t  event_type_len_;    //事件类型大小
    uint_t  write_idx_;         //写下标
    uint_t  read_idx_;          //读下标
};

ZRSOCKET_NAMESPACE_END

#endif
