// Some compilers (e.g. VC++) benefit significantly from using this. 
// We've measured 3-4% build speed improvements in apps as a result 
#pragma once

#ifndef ZRSOCKET_EVENT_TYPE_QUEUE_H
#define ZRSOCKET_EVENT_TYPE_QUEUE_H
#include <cmath>
#include <algorithm>
#include "config.h"
#include "base_type.h"
#include "event_type.h"
#include "atomic.h"
#include "malloc.h"
#include "memory.h"
#include "lockfree.h"

ZRSOCKET_NAMESPACE_BEGIN

// 双缓冲队列: double buffer event_type_queue
//  一般用于单消费者场景
template <typename TMutex>
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

    inline int init(uint_t capacity, uint_t event_type_len = 8)
    {
        if (capacity < 1) {
            return 0;
        }
        clear();

        //将取模(求余数%)转换为 算术位运算与(&)
        //计算大于等于capacity的最小 2的N次方整数
        auto log2x = std::log2(capacity);
        capacity   = static_cast<uint_t>(std::pow(2, std::ceil(log2x)));

        //将对event_type_len乘法转换为 移位掩码<<
        uint8_t type_len_mask = static_cast<uint8_t>(std::ceil(std::log2(event_type_len)));

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

    template <typename THandler>
    EventType * pop(THandler &handler)
    {
        return active_buf_->pop(handler);
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

        inline int init(uint_t capacity, uint8_t type_len_mask)
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
            read_index_ = 0;
            write_index_ = 0;
            capacity_   = 0;
            type_len_mask_ = 0;
        }

        inline uint64_t capacity() const
        {
            return capacity_;
        }

        inline uint64_t size() const
        {
            return write_index_ - read_index_;
        }

        inline uint64_t free_size() const
        {
            return capacity_ - (write_index_ - read_index_);
        }

        inline bool empty() const
        {
            return read_index_ == write_index_;
        }

        inline int push(const EventType *event)
        {
            uint64_t capacity    = capacity_;
            uint64_t write_index = write_index_;
            if (write_index - read_index_ < capacity) {
                uint64_t offset = (write_index & (capacity - 1)) << type_len_mask_;
                zrsocket_memcpy((buffer_ + offset), event->event_ptr(), event->event_len());
                write_index_ = write_index + 1;
                return 1;
            }
            return 0;
        }

        inline EventType * pop()
        {
            uint64_t read_index = read_index_;
            if (write_index_ > read_index) {
                uint64_t offset  = (read_index & (capacity_ - 1)) << type_len_mask_;
                EventType *event = (EventType*)(buffer_ + offset);
                read_index_ = read_index + 1;
                return event;
            }
            return nullptr;
        }

        //THandler类实现handle_event(EventType *event)
        template <typename THandler>
        EventType * pop(THandler &handler)
        {
            uint64_t read_index = read_index_;
            if (write_index_ > read_index) {
                uint64_t offset  = (read_index & (capacity_ - 1)) << type_len_mask_;
                EventType *event = (EventType *)(buffer_ + offset);

                //处理事件
                handler.handle_event(event);

                read_index_ = read_index + 1;
                return event;
            }

            return nullptr;
        }

    public:
        char       *buffer_         = nullptr;  //事件类型缓冲
        uint64_t    capacity_       = 0;        //队列容量(只能是2的N次方)
        uint8_t     type_len_mask_  = 0;        //类型长度掩码N(类型长度只能是 2的N次方)

        uint64_t    read_index_     = 0;        //读位置(单调递增:只增不减)
        uint64_t    write_index_    = 0;        //写位置(单调递增:只增不减)
    };

    Buffer *active_buf_;
    Buffer *standby_buf_;
    Buffer  buf1_;
    Buffer  buf2_;
    TMutex  mutex_;
};

// 单生产者单消费者 队列
//  signle producer single consumer event_type_queue

//normal implement queue
class SPSCNormalEventTypeQueue
{
public:
    inline SPSCNormalEventTypeQueue() = default;
    ~SPSCNormalEventTypeQueue()
    {
        clear();
    }

    inline int init(uint_t capacity, uint_t event_type_len = 8)
    {
        if (capacity < 1) {
            return 0;
        }
        clear();

        //将取模(求余数%)转换为 算术位运算与(&)
        //计算大于等于capacity的最小 2的N次方整数
        auto log2x = std::log2(capacity);
        capacity = static_cast<uint_t>(std::pow(2, std::ceil(log2x)));

        //将对event_type_len乘法转换为 移位掩码<<
        type_len_mask_ = static_cast<uint8_t>(std::ceil(std::log2(event_type_len)));

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
        write_index_ = 0;
        read_index_ = 0;
    }

    inline uint_t capacity() const
    {
        return capacity_;
    }

    inline uint_t size() const
    {
        uint_t write_index = write_index_;
        uint_t read_index  = read_index_;
        if (write_index >= read_index) {
            return write_index - read_index;
        }
        else {
            return capacity_ - read_index + write_index;
        }
    }

    inline uint_t free_size() const
    {
        return capacity_ - size();
    }

    inline bool empty() const
    {
        return write_index_ == read_index_;
    }

    inline bool full() const
    {
        return ((write_index_ + 1) & (capacity_ - 1)) == read_index_;
    }

    inline int push(const EventType* event)
    {
        uint_t write_index = write_index_;
        if (((write_index + 1) & (capacity_ - 1)) != read_index_) {
            uint_t offset = write_index << type_len_mask_;
            zrsocket_memcpy((buffer_ + offset), event->event_ptr(), event->event_len());
            write_index_ = (write_index + 1) & (capacity_ - 1);
            return 1;
        }

        return 0;
    }

    inline EventType * pop()
    {
        uint_t read_index = read_index_;
        if (write_index_ != read_index) {
            uint_t offset = read_index << type_len_mask_;
            EventType *event = (EventType *)(buffer_ + offset);
            read_index_ = (read_index + 1) & (capacity_ - 1);
            return event;
        }
        return nullptr;
    }

    template <typename THandler>
    EventType * pop(THandler &handler)
    {
        uint_t read_index = read_index_;
        if (write_index_ != read_index) {
            uint_t offset = read_index << type_len_mask_;
            EventType *event = (EventType *)(buffer_ + offset);

            //处理事件
            handler.handle_event(event);

            read_index_ = (read_index + 1) & (capacity_ - 1);
            return event;
        }
        return nullptr;
    }

    inline int pop(SPSCNormalEventTypeQueue *push_queue, uint_t batch_size)
    {
        auto queue_size = size();
        uint_t push_free_size = push_queue->free_size();
        if ((queue_size < 1) || (push_free_size < 1)) {
            return 0;
        }

        uint_t pop_size = std::min<uint_t>(queue_size, push_free_size);
        pop_size = std::min<uint_t>(batch_size, pop_size);
        for (uint_t i = 0; i < pop_size; ++i) {
            push_queue->push(pop());
        }
        return pop_size;
    }

    inline bool swap_buffer() const
    {
        return false;
    }

private:
    static constexpr const int PADDING_SIZE = (CACHE_LINE_SIZE - sizeof(uint_t));

    uint_t   read_index_    = 0;        //读下标
    char     padding1_[PADDING_SIZE];
    uint_t   write_index_   = 0;        //写下标
    char     padding2_[PADDING_SIZE];

    char    *buffer_        = nullptr;  //事件类型缓冲
    uint_t   capacity_      = 0;        //队列容量(只能是2的N次方)
    uint8_t  type_len_mask_ = 0;        //类型长度掩码N(类型长度只能是 2的N次方)
};

class SPSCVolatileEventTypeQueue
{
public:
    inline SPSCVolatileEventTypeQueue() = default;
    inline ~SPSCVolatileEventTypeQueue()
    {
        clear();
    }

    inline int init(uint_t capacity, uint_t event_type_len = 8)
    {
        if (capacity < 1) {
            return 0;
        }
        clear();

        //将取模(求余数%)转换为 算术位运算与(&)
        //计算大于等于capacity的最小 2的N次方整数
        auto log2x = std::log2(capacity);
        capacity   = static_cast<uint_t>(std::pow(2, std::ceil(log2x)));

        //将对event_type_len乘法转换为 移位掩码<<
        type_len_mask_ = static_cast<uint8_t>(std::ceil(std::log2(event_type_len)));

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
        read_index_    = 0;
        write_index_   = 0;
    }

    inline uint64_t capacity() const
    {
        return capacity_;
    }

    inline uint64_t size() const
    {
        return write_index_ - read_index_;
    }

    inline uint64_t free_size() const
    {
        return capacity_ - (write_index_ - read_index_);
    }

    inline bool empty() const
    {
        return read_index_ == write_index_;
    }

    inline int push(const EventType *event)
    {
        uint64_t capacity    = capacity_;
        uint64_t write_index = write_index_;
        if (write_index - read_index_ < capacity) {
            uint64_t offset = (write_index & (capacity - 1)) << type_len_mask_;
            zrsocket_memcpy((buffer_ + offset), event->event_ptr(), event->event_len());
            write_index_ = write_index + 1;
            return 1;
        }

        return 0;
    }

    template <typename THandler>
    EventType * pop(THandler &handler)
    {
        uint64_t read_index = read_index_;
        if (write_index_ > read_index) {
            uint64_t offset  = (read_index & (capacity_ - 1)) << type_len_mask_;
            EventType *event = (EventType *)(buffer_ + offset);

            //处理事件
            handler.handle_event(event);

            read_index_ = read_index + 1;
            return event;
        }
        return nullptr;
    }

    inline bool swap_buffer() const
    {
        return false;
    }

private:
    static constexpr const int PADDING_SIZE = (CACHE_LINE_SIZE - sizeof(uint64_t));
    volatile uint64_t read_index_    = 0;       //读下标(单调递增:只增不减)
    char padding1_[PADDING_SIZE];
    volatile uint64_t write_index_   = 0;       //写下标(单调递增:只增不减)
    char padding2_[PADDING_SIZE];

    char             *buffer_        = nullptr; //事件类型缓冲
    uint64_t          capacity_      = 0;       //队列容量(只能是2的N次方)
    uint8_t           type_len_mask_ = 0;       //类型长度掩码N(类型长度只能是 2的N次方)
};

class SPSCAtomicEventTypeQueue
{
public:
    inline SPSCAtomicEventTypeQueue() = default;
    inline ~SPSCAtomicEventTypeQueue()
    {
        clear();
    }

    inline int init(uint_t capacity, uint_t event_type_len = 8)
    {
        if (capacity < 1) {
            return 0;
        }
        clear();

        //将取模(求余数%)转换为 算术位运算与(&)
        //计算大于等于capacity的最小 2的N次方整数
        auto log2x = std::log2(capacity);
        capacity   = static_cast<uint_t>(std::pow(2, std::ceil(log2x)));

        //将对event_type_len乘法转换为 移位掩码<<
        type_len_mask_ = static_cast<uint8_t>(std::ceil(std::log2(event_type_len)));

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
        read_index_.store(0, std::memory_order_relaxed);
        write_index_.store(0, std::memory_order_relaxed);
    }

    inline uint64_t capacity() const
    {
        return capacity_;
    }

    inline uint64_t size() const
    {
        return write_index_.load(std::memory_order_relaxed) - read_index_.load(std::memory_order_relaxed);
    }

    inline uint64_t free_size() const
    {
        return capacity_ - (write_index_.load(std::memory_order_relaxed) - read_index_.load(std::memory_order_relaxed));
    }

    inline bool empty() const
    {
        return read_index_.load(std::memory_order_relaxed) == write_index_.load(std::memory_order_relaxed);
    }

    inline int push(const EventType *event)
    {
        uint64_t capacity    = capacity_;
        uint64_t write_index = write_index_.load(std::memory_order_relaxed);
        if (write_index - read_index_.load(std::memory_order_relaxed) < capacity) {
            uint64_t offset = (write_index & (capacity - 1)) << type_len_mask_;
            zrsocket_memcpy((buffer_ + offset), event->event_ptr(), event->event_len());
            write_index_.fetch_add(1, std::memory_order_relaxed);
            return 1;
        }

        return 0;
    }
    
    template <typename THandler>
    EventType * pop(THandler &handler)
    {
        uint64_t read_index = read_index_.load(std::memory_order_relaxed);
        if (write_index_.load(std::memory_order_relaxed) > read_index) {
            uint64_t offset  = (read_index & (capacity_ - 1)) << type_len_mask_;
            EventType *event = (EventType *)(buffer_ + offset);

            //处理事件
            handler.handle_event(event);

            read_index_.fetch_add(1, std::memory_order_relaxed);
            return event;
        }

        return nullptr;
    }

    inline bool swap_buffer() const
    {
        return false;
    }

private:
    static constexpr const int PADDING_SIZE = (CACHE_LINE_SIZE - sizeof(AtomicUInt64));
    AtomicUInt64    read_index_     = { 0 };    //读位置(单调递增:只增不减)
    char padding1_[PADDING_SIZE];
    AtomicUInt64    write_index_    = { 0 };    //写位置(单调递增:只增不减)
    char padding2_[PADDING_SIZE];

    char           *buffer_         = nullptr;  //事件类型缓冲
    uint64_t        capacity_       = 0;        //队列容量(只能是2的N次方)
    uint8_t         type_len_mask_  = 0;        //类型长度掩码N(类型长度只能是 2的N次方)
};

// 多生产者单消费者 队列
//  multiple producer single consumer event_type_queue
class MPSCEventTypeQueue
{
public:
    inline MPSCEventTypeQueue() = default;
    inline ~MPSCEventTypeQueue()
    {
        clear();
    }

    inline int init(uint_t capacity, uint16_t event_type_len = 8)
    {
        if (capacity < 1) {
            return 0;
        }
        clear();

        //将取模(求余数%)转换为 算术与&
        //计算大于等于capacity的最小 2的N次方整数
        auto log2x = std::log2(capacity);
        capacity   = static_cast<uint_t>(std::pow(2, std::ceil(log2x)));

        //将对event_type_len乘法转换为 移位掩码<<
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
        capacity_       = 0;
        type_len_mask_  = 0;
        read_index_     = 0;
        write_index_.store(0, std::memory_order_relaxed);
        max_read_index_.store(0, std::memory_order_relaxed);
        max_read_index_.store(0, std::memory_order_relaxed);
    }

    inline uint64_t capacity() const
    {
        return capacity_;
    }

    inline uint64_t size() const
    {
        return write_index_.load(std::memory_order_relaxed) - read_index_;
    }

    inline uint64_t free_size() const
    {
        return capacity_ - (write_index_.load(std::memory_order_relaxed) - read_index_);
    }

    inline bool empty() const
    {
        return read_index_ == write_index_.load(std::memory_order_relaxed);
    }

    inline int push(const EventType *event)
    {
        char    *buffer   = buffer_;
        uint64_t capacity = capacity_;
        uint16_t type_len_mask = type_len_mask_;
        uint64_t write_index;

        for (int i = 0; i < SPIN_LOOP_TIMES; ++i) {
            write_index = write_index_.load(std::memory_order_relaxed);
            if (write_index - read_index_ < capacity) {
                if (write_index_.compare_exchange_weak(write_index, write_index + 1, std::memory_order_relaxed, std::memory_order_relaxed)) {

                    //在write_index(不能用write_index_)位置写入数据
                    uint64_t offset = (write_index & (capacity - 1)) << type_len_mask;
                    zrsocket_memcpy((buffer + offset), event->event_ptr(), event->event_len());

                    //发布刚写入数据的位置(更新最大读位置)
                    while (!max_read_index_.compare_exchange_weak(write_index, write_index + 1, std::memory_order_relaxed, std::memory_order_relaxed));

                    return 1;
                }
            }
        }

        return 0;
    }

    template <typename THandler>
    EventType * pop(THandler &handler)
    {
        uint64_t read_index = read_index_;
        if (max_read_index_.load(std::memory_order_relaxed) > read_index) {

            uint64_t offset  = ((read_index & (capacity_ - 1)) << type_len_mask_);
            EventType *event = (EventType *)(buffer_ + offset);

            //处理事件
            handler.handle_event(event);

            read_index_ = read_index + 1;
            return event;
        }

        return nullptr;
    }

    inline bool swap_buffer() const
    {
        return false;
    }

private:
    static constexpr const int PADDING_SIZE1 = (CACHE_LINE_SIZE - sizeof(uint64_t));
    static constexpr const int PADDING_SIZE2 = (CACHE_LINE_SIZE - sizeof(AtomicUInt64));

    volatile uint64_t read_index_ = 0;          //读位置(单调递增:只增不减)
    char padding1_[PADDING_SIZE1];

    AtomicUInt64 write_index_     = { 0 };       //写位置(单调递增:只增不减)
    char padding2_[PADDING_SIZE2];

    //更新write_index_和真实的写入数据是两个步骤
    //所以引入max_read_index_，写入完成后更新max_read_index_
    AtomicUInt64 max_read_index_  = { 0 };       //最大读位置(单调递增:只增不减)
    char padding3_[PADDING_SIZE2];

    char         *buffer_         = nullptr;     //事件类型缓冲
    uint64_t      capacity_       = 0;           //队列容量
    uint16_t      type_len_mask_  = 0;           //类型长度掩码
};

// 多生产者多消费者 队列
//  multiple producer multiple consumer event_type_queue
class MPMCEventTypeQueue
{
public:
    inline MPMCEventTypeQueue() = default;
    inline ~MPMCEventTypeQueue()
    {
        clear();
    }

    inline int init(uint_t capacity, uint16_t event_type_len = 8)
    {
        if (capacity < 1) {
            return 0;
        }
        clear();

        //将取模(求余数%)转换为 算术与&
        //计算大于等于capacity的最小 2的N次方整数
        auto log2x = std::log2(capacity);
        capacity = static_cast<uint_t>(std::pow(2, std::ceil(log2x)));

        //将对event_type_len乘法转换为 移位掩码<<
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
        read_index_.store(0, std::memory_order_relaxed);
        write_index_.store(0, std::memory_order_relaxed);
        max_read_index_.store(0, std::memory_order_relaxed);
        min_write_index_.store(0, std::memory_order_relaxed);
    }

    inline uint64_t capacity() const
    {
        return capacity_;
    }

    inline uint64_t size() const
    {
        return write_index_.load(std::memory_order_relaxed) - read_index_.load(std::memory_order_relaxed);
    }

    inline uint64_t free_size() const
    {
        return capacity_ - (write_index_.load(std::memory_order_relaxed) - read_index_.load(std::memory_order_relaxed));
    }

    inline bool empty() const
    {
        return write_index_.load(std::memory_order_relaxed) == read_index_.load(std::memory_order_relaxed);
    }

    inline int push(const EventType *event)
    {
        char *buffer = buffer_;
        uint64_t capacity = capacity_;
        uint16_t type_len_mask = type_len_mask_;
        uint64_t write_index;

        for (int i = 0; i < SPIN_LOOP_TIMES; ++i) {
            write_index = write_index_.load(std::memory_order_relaxed);
            if (write_index - min_write_index_.load(std::memory_order_relaxed) < capacity) {
                if (write_index_.compare_exchange_weak(write_index, write_index + 1, std::memory_order_relaxed, std::memory_order_relaxed)) {

                    //在write_index(不能用write_index_)位置写入数据
                    uint64_t offset = (write_index & (capacity - 1)) << type_len_mask;
                    zrsocket_memcpy((buffer + offset), event->event_ptr(), event->event_len());

                    //发布刚写入数据的位置(更新最大读位置)
                    while (!max_read_index_.compare_exchange_weak(write_index, write_index + 1, std::memory_order_relaxed, std::memory_order_relaxed));

                    return 1;
                }
            }
        }

        return 0;
    }

    template <typename THandler>
    EventType * pop(THandler &handler)
    {
        char *buffer = buffer_;
        uint64_t capacity = capacity_;
        uint16_t type_len_mask = type_len_mask_;
        uint64_t read_index;

        for (int i = 0; i < SPIN_LOOP_TIMES; ++i) {
            read_index = read_index_.load(std::memory_order_relaxed);
            if (max_read_index_.load(std::memory_order_relaxed) > read_index) {
                if (read_index_.compare_exchange_weak(read_index, read_index + 1, std::memory_order_relaxed, std::memory_order_relaxed)) {

                    //在read_index(不能用read_index_)位置读数据
                    uint64_t offset  = (read_index & (capacity - 1)) << type_len_mask;
                    EventType *event = (EventType *)(buffer + offset);

                    //处理事件
                    handler.handle_event(event);

                    //发布刚读数据的位置(更新最小写位置)                                             
                    while (!min_write_index_.compare_exchange_weak(read_index, read_index + 1, std::memory_order_relaxed));

                    return event;
                }
            }
            else {
                return nullptr;
            }
        }

        return nullptr;
    }

    inline bool swap_buffer() const
    {
        return false;
    }

private:
    static constexpr const int PADDING_SIZE = (CACHE_LINE_SIZE - sizeof(AtomicUInt64));

    AtomicUInt64 read_index_ = { 0 };        //读位置(单调递增:只增不减)
    char padding1_[PADDING_SIZE];

    AtomicUInt64 write_index_ = { 0 };       //写位置(单调递增:只增不减)
    char padding2_[PADDING_SIZE];

    //更新write_index_和真实的写入数据是两个步骤
    //所以引入max_read_index_，写入完成后更新max_read_index_
    AtomicUInt64 max_read_index_ = { 0 };   //最大读位置(单调递增:只增不减)
    char padding3_[PADDING_SIZE];

    //与max_read_index_同理
    AtomicUInt64 min_write_index_ = { 0 };  //最小写位置(单调递增:只增不减)
    char padding4_[PADDING_SIZE];

    char           *buffer_   = nullptr;    //事件类型缓冲
    uint64_t        capacity_ = 0;          //队列容量
    uint16_t        type_len_mask_ = 0;     //类型长度掩码
};

// 单生产者多消费者 队列
//  single producer multiple consumer event_type_queue
class SPMCEventTypeQueue
{
public:
    inline SPMCEventTypeQueue() = default;
    inline ~SPMCEventTypeQueue()
    {
        clear();
    }

    inline int init(uint_t capacity, uint16_t event_type_len = 8)
    {
        if (capacity < 1) {
            return 0;
        }
        clear();

        //将取模(求余数%)转换为 算术与&
        //计算大于等于capacity的最小 2的N次方整数
        auto log2x = std::log2(capacity);
        capacity   = static_cast<uint_t>(std::pow(2, std::ceil(log2x)));

        //将对event_type_len乘法转换为 移位掩码<<
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
        read_index_.store(0, std::memory_order_relaxed);
        write_index_ = 0;
    }

    inline uint64_t capacity() const
    {
        return capacity_;
    }

    inline uint64_t size() const
    {
        return write_index_ - read_index_.load(std::memory_order_relaxed);
    }

    inline uint64_t free_size() const
    {
        return capacity_ - (write_index_ - read_index_.load(std::memory_order_relaxed));
    }

    inline bool empty() const
    {
        return read_index_.load(std::memory_order_relaxed) == write_index_;
    }

    inline int push(const EventType *event)
    {
        uint64_t capacity    = capacity_;
        uint64_t write_index = write_index_;
        if (write_index - min_write_index_.load(std::memory_order_relaxed) < capacity) {
            uint64_t offset = (write_index & (capacity - 1)) << type_len_mask_;
            zrsocket_memcpy((buffer_ + offset), event->event_ptr(), event->event_len());
            write_index_ = write_index + 1;
            return 1;
        }
        return 0;
    }

    template <typename THandler>
    EventType * pop(THandler &handler)
    {
        char *buffer = buffer_;
        uint64_t capacity = capacity_;
        uint16_t type_len_mask = type_len_mask_;
        uint64_t read_index;

        for (int i = 0; i < SPIN_LOOP_TIMES; ++i) {
            read_index = read_index_.load(std::memory_order_relaxed);
            if (write_index_ > read_index) {
                if (read_index_.compare_exchange_weak(read_index, read_index + 1, std::memory_order_relaxed, std::memory_order_relaxed)) {

                    //在read_index(不能用read_index_)位置读数据
                    uint64_t offset  = (read_index & (capacity - 1)) << type_len_mask;
                    EventType *event = (EventType *)(buffer + offset);

                    //处理事件
                    handler.handle_event(event);

                    //发布刚读数据的位置
                    while (!min_write_index_.compare_exchange_weak(read_index, read_index + 1, std::memory_order_relaxed));

                    return event;
                }
            }
            else {
                return nullptr;
            }
        }

        return nullptr;
    }

    inline bool swap_buffer() const
    {
        return false;
    }

private:
    static constexpr const int PADDING_SIZE1 = (CACHE_LINE_SIZE - sizeof(AtomicUInt64));
    static constexpr const int PADDING_SIZE2 = (CACHE_LINE_SIZE - sizeof(uint64_t));

    AtomicUInt64        read_index_  = { 0 };   //读位置(单调递增:只增不减)
    char padding1_[PADDING_SIZE1];
    volatile uint64_t   write_index_ = 0;       //写位置(单调递增:只增不减)
    char padding2_[PADDING_SIZE2];

    AtomicUInt64 min_write_index_    = { 0 };   //最小写位置(单调递增:只增不减)
    char padding3_[PADDING_SIZE1];

    char               *buffer_ = nullptr;      //事件类型缓冲
    uint64_t            capacity_ = 0;          //队列容量
    uint16_t            type_len_mask_ = 0;     //类型长度掩码
};

using EventTypeQueue = SPSCNormalEventTypeQueue;

ZRSOCKET_NAMESPACE_END

#endif
