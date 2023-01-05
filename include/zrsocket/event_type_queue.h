// Some compilers (e.g. VC++) benefit significantly from using this. 
// We've measured 3-4% build speed improvements in apps as a result 
#pragma once

#ifndef ZRSOCKET_EVENT_TYPE_QUEUE_H
#define ZRSOCKET_EVENT_TYPE_QUEUE_H

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
        : pool_begin_(nullptr)
        , pool_end_(nullptr)
        , write_index_(nullptr)
        , read_index_(nullptr)
        , capacity_(0)
        , queue_size_(0)
        , event_type_len_(0)
    {
    }

    virtual ~EventTypeQueue()
    {
        if (nullptr != pool_begin_) {
            zrsocket_free(pool_begin_);
            pool_begin_ = nullptr;
        }
    }

    inline int init(uint_t capacity, int event_type_len = 64)
    {
        clear();

        uint_t pool_size = capacity * event_type_len;
        pool_begin_ = (char *)zrsocket_malloc(pool_size);
        if (nullptr != pool_begin_) {
            pool_end_       = pool_begin_ + pool_size;
            write_index_    = pool_begin_;
            read_index_     = pool_begin_;
            capacity_       = capacity;
            queue_size_     = 0;
            event_type_len_ = event_type_len;
            return 1;
        }
        return 0;
    }

    inline void clear()
    {
        if (nullptr != pool_begin_) {
            zrsocket_free(pool_begin_);
            pool_begin_ = nullptr;
            pool_end_   = nullptr;
        }
        write_index_    = nullptr;
        read_index_     = nullptr;
        capacity_       = 0;
        queue_size_     = 0;
        event_type_len_ = 0;
    }

    inline int push(const EventType *event)
    {
        if (queue_size_ < capacity_) {
            zrsocket_memcpy(write_index_, event->event_ptr(), event->event_len());
            if (pool_end_ - write_index_ > event_type_len_) {
                write_index_ += event_type_len_;
            }
            else {
                write_index_ = pool_begin_;
            }
            return ++queue_size_;
        }
        return 0;
    }

    inline EventType * pop()
    {
        if (queue_size_ > 0) {
            --queue_size_;

            EventType *event = (EventType *)read_index_;
            if (pool_end_ - read_index_ > event_type_len_) {
                read_index_ += event_type_len_;
            }
            else {
                read_index_ = pool_begin_;
            }
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
    char *pool_begin_;      //事件对象池开始位
    char *pool_end_;        //事件对象池结束位
    char *write_index_;     //写位置
    char *read_index_;      //读位置

    uint_t capacity_;       //队列容量
    uint_t queue_size_;     //队列大小
    int    event_type_len_; //事件类型大小
};

ZRSOCKET_NAMESPACE_END

#endif
