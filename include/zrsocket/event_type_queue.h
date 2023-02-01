// Some compilers (e.g. VC++) benefit significantly from using this. 
// We've measured 3-4% build speed improvements in apps as a result 
#pragma once

#ifndef ZRSOCKET_EVENT_TYPE_QUEUE_H
#define ZRSOCKET_EVENT_TYPE_QUEUE_H
#include <cmath>
#include "config.h"
#include "base_type.h"
#include "event_type.h"
#include "atomic.h"
#include "malloc.h"
#include "memory.h"

ZRSOCKET_NAMESPACE_BEGIN

// 双缓冲队列: double buffer event_type_queue
//  一般用于只有一个消费者
template <class TMutex>
class DoubleBufferEventTypeQueue
{
public:
    inline DoubleBufferEventTypeQueue()
    {
        active_buf_  = &buf1_;
        standby_buf_ = &buf2_;
    }

    inline ~DoubleBufferEventTypeQueue()
    {
        clear();
    }

    inline int init(uint_t capacity, uint16_t event_type_len = 8)
    {
        if (capacity < 1) {
            return 0;
        }
        clear();

        //将取模(求余数%)转换为 算术位运算与(&)
        //计算大于等于capacity的最小 2的N次方整数
        auto log2x = std::log2(capacity);
        capacity = static_cast<uint_t>(std::pow(2, std::ceil(log2x)));

        //将event_type_len转换为 移位掩码<<
        uint16_t type_len_mask = static_cast<uint16_t>(std::ceil(std::log2(event_type_len)));

        buf1_.init(capacity, type_len_mask);
        buf2_.init(capacity, type_len_mask);

        return 1;
    }

    inline void clear()
    {
        buf1_.clear();
        buf2_.clear();
    }

    inline uint64_t capacity() const
    {
        return buf1_.capacity_;
    }

    inline int push(const EventType *event)
    {
        int ret;
        mutex_.lock();
        ret = standby_buf_->push(event);
        mutex_.unlock();
        return ret;
    }

    inline EventType * pop()
    {
        return active_buf_->pop();
    }

    //只能在消费者线程且active_buf_.empty()==true时调用
    inline bool swap_buffer()
    {
        mutex_.lock();
        if (!standby_buf_->empty()) {
            std::swap(standby_buf_, active_buf_);
            mutex_.unlock();
            return true;
        }
        mutex_.unlock();

        return false;
    }

private:
    struct Buffer
    {
    public:
        inline Buffer() = default;
        inline ~Buffer()
        {
            clear();
        }

        inline int init(uint_t capacity, uint16_t type_len_mask)
        {
            uint_t buffer_size = capacity << type_len_mask;
            buffer_ = (char *)zrsocket_malloc(buffer_size);
            if (nullptr != buffer_) {
                capacity_ = capacity;
                type_len_mask_ = type_len_mask;
                return 1;
            }
            return 0;
        }

        inline void clear()
        {
            if (nullptr != buffer_) {
                zrsocket_free(buffer_);
                buffer_ = nullptr;
            }
            head_index_ = 0;
            tail_index_ = 0;
            capacity_   = 0;
            type_len_mask_ = 0;
        }

        inline uint64_t capacity() const
        {
            return capacity_;
        }

        inline uint64_t size() const
        {
            return tail_index_ - head_index_;
        }

        inline uint64_t free_size() const
        {
            return capacity_ - (tail_index_ - head_index_);
        }

        inline bool empty() const
        {
            return head_index_ == tail_index_;
        }

        inline int push(const EventType* event)
        {
            uint64_t capacity   = capacity_;
            uint64_t tail_index = tail_index_;
            uint64_t queue_size = tail_index - head_index_;
            if (queue_size < capacity) {
                uint64_t offset = (tail_index & (capacity - 1)) << type_len_mask_;
                zrsocket_memcpy((buffer_ + offset), event->event_ptr(), event->event_len());
                tail_index_ = tail_index + 1;
                return 1;
            }
            return 0;
        }

        inline EventType * pop()
        {
            uint64_t head_index = head_index_;
            if (tail_index_ > head_index) {
                uint64_t offset = (head_index & (capacity_ - 1)) << type_len_mask_;
                EventType *event = (EventType *)(buffer_ + offset);
                head_index_ = head_index + 1;
                return event;
            }
            return nullptr;
        }

    public:
        char       *buffer_         = nullptr;  //事件类型缓冲
        uint64_t    capacity_       = 0;        //队列容量
        uint64_t    head_index_     = 0;        //头下标(单调递增:只增不减)
        uint64_t    tail_index_     = 0;        //尾下标(单调递增:只增不减)
        uint16_t    type_len_mask_  = 0;        //类型长度掩码
    };

    Buffer *active_buf_;
    Buffer *standby_buf_;
    Buffer  buf1_;
    Buffer  buf2_;
    TMutex  mutex_;
};

// 单生产者单消费者 队列
//  signle producer single consumer event_type_queue

class SPSCVolatileEventTypeQueue
{
public:
    inline SPSCVolatileEventTypeQueue() = default;
    inline ~SPSCVolatileEventTypeQueue()
    {
        clear();
    }

    inline int init(uint_t capacity, uint16_t event_type_len = 8)
    {
        if (capacity < 1) {
            return 0;
        }
        clear();

        //将取模(求余数%)转换为 算术位运算与(&)
        //计算大于等于capacity的最小 2的N次方整数
        auto log2x = std::log2(capacity);
        capacity = static_cast<uint_t>(std::pow(2, std::ceil(log2x)));

        //将event_type_len换算为移位掩码
        type_len_mask_ = static_cast<uint16_t>(std::ceil(std::log2(event_type_len)));

        uint_t buffer_size = capacity << type_len_mask_;
        buffer_ = (char *)zrsocket_malloc(buffer_size);
        if (nullptr != buffer_) {
            capacity_ = capacity;
            return 1;
        }

        return 0;
    }

    inline void clear()
    {
        if (nullptr != buffer_) {
            zrsocket_free(buffer_);
            buffer_ = nullptr;
        }
        capacity_ = 0;
        type_len_mask_ = 0;
        head_index_ = 0;
        tail_index_ = 0;
    }

    inline uint64_t capacity() const
    {
        return capacity_;
    }

    inline uint64_t size() const
    {
        return tail_index_ - head_index_;
    }

    inline uint64_t free_size() const
    {
        return capacity_ - (tail_index_ - head_index_);
    }

    inline bool empty() const
    {
        return head_index_ == tail_index_;
    }

    inline int push(const EventType *event)
    {
        uint64_t capacity   = capacity_;
        uint64_t tail_index = tail_index_;
        uint64_t queue_size = tail_index - head_index_;
        if (queue_size < capacity) {
            uint64_t offset = (tail_index & (capacity - 1)) << type_len_mask_;
            zrsocket_memcpy((buffer_ + offset), event->event_ptr(), event->event_len());
            tail_index_ = tail_index + 1;
            return 1;
        }
        return 0;
    }

    inline EventType * pop()
    {
        uint64_t head_index = head_index_;
        if (tail_index_ > head_index) {
            uint64_t offset = (head_index & (capacity_ - 1)) << type_len_mask_;
            EventType *event = (EventType *)(buffer_ + offset);
            head_index_ = head_index + 1;
            return event;
        }
        return nullptr;
    }

    inline bool swap_buffer() const
    {
        return false;
    }

private:
    char             *buffer_           = nullptr;  //事件类型缓冲
    uint64_t          capacity_         = 0;        //队列容量
    volatile uint64_t head_index_       = 0;        //头下标(单调递增:只增不减)
    volatile uint64_t tail_index_       = 0;        //尾下标(单调递增:只增不减)
    uint16_t          type_len_mask_    = 0;        //类型长度掩码
};

class SPSCAtomicEventTypeQueue
{
public:
    inline SPSCAtomicEventTypeQueue() = default;
    inline ~SPSCAtomicEventTypeQueue()
    {
        clear();
    }

    inline int init(uint_t capacity, uint16_t event_type_len = 8)
    {
        if (capacity < 1) {
            return 0;
        }
        clear();

        //将取模(求余数%)转换为 算术位运算与(&)
        //计算大于等于capacity的最小 2的N次方整数
        auto log2x = std::log2(capacity);
        capacity = static_cast<uint_t>(std::pow(2, std::ceil(log2x)));

        //将event_type_len换算为移位掩码
        type_len_mask_ = static_cast<uint16_t>(std::ceil(std::log2(event_type_len)));

        uint_t buffer_size = capacity << type_len_mask_;
        buffer_ = (char *)zrsocket_malloc(buffer_size);
        if (nullptr != buffer_) {
            capacity_ = capacity;
            return 1;
        }

        return 0;
    }

    inline void clear()
    {
        if (nullptr != buffer_) {
            zrsocket_free(buffer_);
            buffer_ = nullptr;
        }
        capacity_ = 0;
        type_len_mask_ = 0;
        head_index_.store(0, std::memory_order_relaxed);
        tail_index_.store(0, std::memory_order_relaxed);
    }

    inline uint64_t capacity() const
    {
        return capacity_;
    }

    inline uint64_t size() const
    {
        return tail_index_.load(std::memory_order_relaxed) - head_index_.load(std::memory_order_relaxed);
    }

    inline uint64_t free_size() const
    {
        return capacity_ - (tail_index_.load(std::memory_order_relaxed) - head_index_.load(std::memory_order_relaxed));
    }

    inline bool empty() const
    {
        return head_index_.load(std::memory_order_relaxed) == tail_index_.load(std::memory_order_relaxed);
    }

    inline int push(const EventType *event)
    {
        uint64_t capacity   = capacity_;
        uint64_t tail_index = tail_index_.load(std::memory_order_relaxed);
        uint64_t queue_size = tail_index - head_index_.load(std::memory_order_relaxed);
        if (queue_size < capacity) {
            uint64_t offset = (tail_index & (capacity - 1)) << type_len_mask_;
            zrsocket_memcpy((buffer_ + offset), event->event_ptr(), event->event_len());
            tail_index_.fetch_add(1, std::memory_order_relaxed);
            return 1;
        }
        return 0;
    }

    inline EventType * pop()
    {
        uint64_t head_index = head_index_.load(std::memory_order_relaxed);
        if (tail_index_.load(std::memory_order_relaxed) > head_index) {
            uint64_t offset = (head_index & (capacity_ - 1)) << type_len_mask_;
            EventType *event = (EventType *)(buffer_ + offset);
            head_index_.fetch_add(1, std::memory_order_relaxed);
            return event;
        }
        return nullptr;
    }

    inline bool swap_buffer() const
    {
        return false;
    }

private:
    char           *buffer_         = nullptr;  //事件类型缓冲
    uint64_t        capacity_       = 0;        //队列容量
    AtomicUInt64    head_index_     = { 0 };    //头下标(单调递增:只增不减)
    AtomicUInt64    tail_index_     = { 0 };    //尾下标(单调递增:只增不减)
    uint16_t        type_len_mask_  = 0;        //类型长度掩码
};

//normal implement queue
class NormalEventTypeQueue
{
public:
    inline NormalEventTypeQueue()
    {
    }

    ~NormalEventTypeQueue()
    {
        clear();
    }

    inline int init(uint_t capacity, uint16_t event_type_len = 8)
    {
        if (capacity < 1) {
            return 0;
        }
        clear();

        //将取模(求余数%)转换为 算术位运算与(&)
        //计算大于等于capacity的最小 2的N次方整数
        auto log2x = std::log2(capacity);
        capacity = static_cast<uint_t>(std::pow(2, std::ceil(log2x)));

        uint_t buffer_size = capacity * event_type_len;
        buffer_ = (char *)zrsocket_malloc(buffer_size);
        if (nullptr != buffer_) {
            capacity_ = capacity;
            event_type_len_ = event_type_len;
            return 1;
        }

        return 0;
    }

    inline void clear()
    {
        if (nullptr != buffer_) {
            zrsocket_free(buffer_);
            buffer_ = nullptr;
        }
        write_index_ = 0;
        read_index_  = 0;
        capacity_    = 0;
        queue_size_  = 0;
        event_type_len_ = 0;
    }

    inline int push(const EventType *event)
    {
        if (queue_size_ < capacity_) {
            ++queue_size_;
            uint_t write_index = write_index_;
            zrsocket_memcpy((buffer_ + write_index * event_type_len_), event->event_ptr(), event->event_len());
            write_index_ = (write_index + 1) & (capacity_ - 1);
            return 1;
        }
        return 0;
    }

    inline EventType * pop()
    {
        if (queue_size_ > 0) {
            --queue_size_;
            uint_t read_index = read_index_;
            EventType *event = (EventType *)(buffer_ + read_index * event_type_len_);
            read_index_ = (read_index + 1) & (capacity_ - 1);
            return event;
        }
        return nullptr;
    }

    inline int pop(NormalEventTypeQueue *push_queue, uint_t batch_size)
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

    inline bool swap_buffer() const
    {
        return false;
    }

private:
    char    *buffer_         = nullptr; //事件类型缓冲
    uint_t   capacity_       = 0;       //队列容量
    uint_t   write_index_    = 0;       //写下标
    uint_t   read_index_     = 0;       //读下标
    uint_t   queue_size_     = 0;       //队列大小
    uint16_t event_type_len_ = 0;       //事件类型大小
};

using EventTypeQueue = NormalEventTypeQueue;

ZRSOCKET_NAMESPACE_END

#endif
