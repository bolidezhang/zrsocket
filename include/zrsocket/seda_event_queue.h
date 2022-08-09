#ifndef ZRSOCKET_SEDA_EVENT_QUEUE_H
#define ZRSOCKET_SEDA_EVENT_QUEUE_H

#include "config.h"
#include "base_type.h"
#include "seda_event.h"
#include "malloc.h"
#include "memory.h"

ZRSOCKET_NAMESPACE_BEGIN

class SedaEventQueue
{
public:
    inline SedaEventQueue()
        : pool_begin_(nullptr)
        , pool_end_(nullptr)
        , write_index_(nullptr)
        , read_index_(nullptr)
        , capacity_(0)
        , queue_size_(0)
        , event_len_(0)
    {
    }

    virtual ~SedaEventQueue()
    {
        if (nullptr != pool_begin_) {
            zrsocket_free(pool_begin_);
        }
    }

    inline int init(uint_t capacity, uint_t event_len = 64)
    {
        clear();

        uint_t pool_size = capacity * event_len;
        pool_begin_ = (char *)zrsocket_malloc(pool_size);
        if (nullptr != pool_begin_) {
            pool_end_       = pool_begin_ + pool_size;
            write_index_    = pool_begin_;
            read_index_     = pool_begin_;
            capacity_       = capacity;
            queue_size_     = 0;
            event_len_      = event_len;
            return 1;
        }
        return -1;
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
        event_len_      = 0;
    }

    inline int push(const SedaEvent *event)
    {
        if (queue_size_ < capacity_) {
            zrsocket_memcpy(write_index_, event->event_ptr(), event->event_len());
            if (write_index_ + event_len_ < pool_end_) {
                write_index_ += event_len_;
            }
            else {
                write_index_ = pool_begin_;
            }
            return ++queue_size_;
        }
        return -1;
    }

    inline SedaEvent * pop()
    {
        if (queue_size_ > 0) {
            --queue_size_;

            SedaEvent *event = (SedaEvent *)read_index_;
            if (read_index_ + event_len_ < pool_end_) {
                read_index_ += event_len_;
            }
            else {
                read_index_ = pool_begin_;
            }
            return event;
        }
        return nullptr;
    }

    inline int pop(SedaEventQueue *push_queue, uint_t batch_size)
    {
        uint_t push_free_size = push_queue->free_size();
        if ((queue_size_ < 1) || (push_free_size < 1)) {
            return -1;
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
    uint_t event_len_;      //事件对象大小
};

ZRSOCKET_NAMESPACE_END

#endif
