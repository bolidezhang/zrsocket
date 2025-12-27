// Some compilers (e.g. VC++) benefit significantly from using this. 
// We've measured 3-4% build speed improvements in apps as a result 
#pragma once

#ifndef ZRSOCKET_LOCKFREE_QUEUE_H
#define ZRSOCKET_LOCKFREE_QUEUE_H
#include <deque>
#include <vector>
#include <array>
#include <cmath>
#include "atomic.h"
#include "lockfree.h"

ZRSOCKET_NAMESPACE_BEGIN

// 单生产者单消费者 数组无锁队列
//  signle producer single consumer array lockfree queue
template <typename T, uint_t N>
class SPSCArrayLockfreeQueue
{
public:
    SPSCArrayLockfreeQueue()
    {
        //将取模(求余数%)转换为 算术位运算与(&)
        //计算大于等于capacity的最小 2的N次方整数
        auto log2x = std::log2(N);
        auto size  = static_cast<uint_t>(std::pow(2, std::ceil(log2x)));
        array_.resize(size);
    }

    ~SPSCArrayLockfreeQueue() = default;
    
    inline uint64_t capacity() const
    {
        return array_.size();
    }

    inline uint64_t size()
    {
        return write_index_ - read_index_;
    }

    inline uint64_t free_size()
    {
        return array_.size() - size();
    }

    inline bool empty() const
    {
        return read_index_ == write_index_;
    }

    inline int push(const T &value)
    {
        uint64_t size = array_.size();
        uint64_t write_index = write_index_;
        if (write_index - read_index_ < size) {
            uint64_t offset = write_index & (size - 1);
            array_[offset]  = value;
            write_index_    = write_index + 1;
            return 1;
        }
        return 0;
    }

    inline int push(T &&value)
    {
        uint64_t size = array_.size();
        uint64_t write_index = write_index_;
        if (write_index - read_index_ < size) {
            uint64_t offset = write_index & (size - 1);
            array_[offset]  = std::move(value);
            write_index_    = write_index + 1;
            return 1;
        }
        return 0;
    }

    //TfnHandler format: fnHandler(T &t)/fnHandler(const T &t)
    template <typename TfnHandler>
    int pop(TfnHandler handler)
    {
        uint64_t read_index = read_index_;
        if (write_index_ > read_index) {
            uint64_t offset = read_index & (array_.size() - 1);

            //处理事件
            handler(array_[offset]);

            read_index_ = read_index + 1;
            return 1;
        }
        return 0;
    }

protected:
    static constexpr int PADDING_SIZE = (CACHE_LINE_SIZE - sizeof(uint64_t));

    volatile uint64_t read_index_  = 0;      //读位置(单调递增:只增不减)
    char padding1_[PADDING_SIZE];

    volatile uint64_t write_index_ = 0;      //写位置(单调递增:只增不减)
    char padding2_[PADDING_SIZE];

    std::vector<T> array_;
};

// 多生产者多消费者 数组无锁队列
//  multiple producer multiple consumer array lockfree queue
template <typename T, uint_t N>
class MPMCArrayLockfreeQueue
{
public:
    MPMCArrayLockfreeQueue()
    {
        //将取模(求余数%)转换为 算术位运算与(&)
        //计算大于等于capacity的最小 2的N次方整数
        auto log2x = std::log2(N);
        auto size  = static_cast<uint_t>(std::pow(2, std::ceil(log2x)));
        array_.resize(size);
    }

    ~MPMCArrayLockfreeQueue() = default;

    inline uint64_t capacity() const
    {
        return array_.size();
    }

    inline uint64_t size()
    {
        return (write_index_.load(std::memory_order_relaxed) - 
            read_index_.load(std::memory_order_relaxed));
    }

    inline uint64_t free_size()
    {
        return array_.size() - size();
    }

    inline bool empty() const
    {
        return (write_index_.load(std::memory_order_relaxed) == 
            read_index_.load(std::memory_order_relaxed));
    }

    inline int push(const T &t) 
    {
        uint64_t capacity = array_.size();
        uint64_t write_index;

        for (int i = 0; i < SPIN_LOOP_TIMES; ++i) {
            write_index = write_index_.load(std::memory_order_relaxed);
            if (write_index - min_write_index_.load(std::memory_order_relaxed) < capacity) {
                if (write_index_.compare_exchange_weak(write_index, write_index + 1, 
                    std::memory_order_relaxed, std::memory_order_relaxed)) {

                    //在write_index(不能用write_index_)位置写入数据
                    uint64_t offset = (write_index & (capacity - 1));
                    array_[offset]  = t;

                    //发布刚写入数据的位置(更新最大读位置)
                    while (!max_read_index_.compare_exchange_weak(write_index, write_index + 1, 
                        std::memory_order_relaxed, std::memory_order_relaxed));

                    return 1;
                }
            }
        }

        return 0;
    }

    inline int push(T &&t)
    {
        uint64_t capacity = array_.size();
        uint64_t write_index;

        for (int i = 0; i < SPIN_LOOP_TIMES; ++i) {
            write_index = write_index_.load(std::memory_order_relaxed);
            if (write_index - min_write_index_.load(std::memory_order_relaxed) < capacity) {
                if (write_index_.compare_exchange_weak(write_index, write_index + 1, 
                    std::memory_order_relaxed, std::memory_order_relaxed)) {

                    //在write_index(不能用write_index_)位置写入数据
                    uint64_t offset = (write_index & (capacity - 1));
                    array_[offset]  = std::move(t);

                    //发布刚写入数据的位置(更新最大读位置)
                    while (!max_read_index_.compare_exchange_weak(write_index, write_index + 1, 
                        std::memory_order_relaxed, std::memory_order_relaxed));

                    return 1;
                }
            }
        }

        return 0;
    }

    //TfnHandler format: fnHandler(T &t)/fnHandler(const T &t)
    template <typename TfnHandler>
    int pop(TfnHandler handler)
    {
        uint64_t capacity = array_.size();
        uint64_t read_index;

        for (int i = 0; i < SPIN_LOOP_TIMES; ++i) {
            read_index = read_index_.load(std::memory_order_relaxed);
            if (max_read_index_.load(std::memory_order_relaxed) > read_index) {
                if (read_index_.compare_exchange_weak(read_index, read_index + 1, 
                    std::memory_order_relaxed, std::memory_order_relaxed)) {

                    //在read_index(不能用read_index_)位置读数据
                    uint64_t offset = read_index & (capacity - 1);

                    //处理事件
                    handler(array_[offset]);

                    //发布刚读数据的位置(更新最小写位置)                                             
                    while (!min_write_index_.compare_exchange_weak(read_index, read_index + 1, 
                        std::memory_order_relaxed));

                    return 1;
                }
            }
            else {
                return 0;
            }
        }

        return 0;
    }

protected:
    static constexpr int PADDING_SIZE = (CACHE_LINE_SIZE - sizeof(AtomicUInt64));

    AtomicUInt64 read_index_      = { 0 };      //读位置(单调递增:只增不减)
    char padding1_[PADDING_SIZE];

    AtomicUInt64 write_index_     = { 0 };      //写位置(单调递增:只增不减)
    char padding2_[PADDING_SIZE];

    //更新write_index_和真实的写入数据是两个步骤
    //所以引入max_read_index_，写入完成后更新max_read_index_
    AtomicUInt64 max_read_index_  = { 0 };      //最大读位置(单调递增:只增不减)
    char padding3_[PADDING_SIZE];

    //与max_read_index_同理
    AtomicUInt64 min_write_index_ = { 0 };      //最小写位置(单调递增:只增不减)
    char padding4_[PADDING_SIZE];

    std::vector<T> array_;
};

// 多生产者单消费者 数组无锁队列
//  multiple producer single consumer array lockfree queue
template <typename T, uint_t N>
class MPSCArrayLockfreeQueue
{
public:
    MPSCArrayLockfreeQueue()
    {
        //将取模(求余数%)转换为 算术位运算与(&)
        //计算大于等于capacity的最小 2的N次方整数
        auto log2x = std::log2(N);
        auto size  = static_cast<uint_t>(std::pow(2, std::ceil(log2x)));
        array_.resize(size);
    }

    inline ~MPSCArrayLockfreeQueue()
    {
    }

    inline uint64_t capacity() const
    {
        return array_.size();
    }

    inline uint64_t size() const
    {
        return write_index_.load(std::memory_order_relaxed) - read_index_;
    }

    inline uint64_t free_size() const
    {
        return array_.size() - (write_index_.load(std::memory_order_relaxed) - read_index_);
    }

    inline bool empty() const
    {
        return read_index_ == write_index_.load(std::memory_order_relaxed);
    }

    inline int push(const T &t)
    {
        uint64_t capacity = array_.size();
        uint64_t write_index;

        for (int i = 0; i < SPIN_LOOP_TIMES; ++i) {
            write_index = write_index_.load(std::memory_order_relaxed);
            if (write_index - read_index_ < capacity) {
                if (write_index_.compare_exchange_weak(write_index, write_index + 1, 
                    std::memory_order_relaxed, std::memory_order_relaxed)) {

                    //在write_index(不能用write_index_)位置写入数据
                    uint64_t offset = write_index & (capacity - 1);
                    array_[offset]  = t;

                    //发布刚写入数据的位置(更新最大读位置)
                    while (!max_read_index_.compare_exchange_weak(write_index, write_index + 1, 
                        std::memory_order_relaxed, std::memory_order_relaxed));

                    return 1;
                }
            }
        }

        return 0;
    }

    inline int push(T &&t)
    {
        uint64_t capacity = array_.size();
        uint64_t write_index;

        for (int i = 0; i < SPIN_LOOP_TIMES; ++i) {
            write_index = write_index_.load(std::memory_order_relaxed);
            if (write_index - read_index_ < capacity) {
                if (write_index_.compare_exchange_weak(write_index, write_index + 1, 
                    std::memory_order_relaxed, std::memory_order_relaxed)) {

                    //在write_index(不能用write_index_)位置写入数据
                    uint64_t offset = (write_index & (capacity - 1));
                    array_[offset]  = std::move(t);

                    //发布刚写入数据的位置(更新最大读位置)
                    while (!max_read_index_.compare_exchange_weak(write_index, write_index + 1, 
                        std::memory_order_relaxed, std::memory_order_relaxed));

                    return 1;
                }
            }
        }

        return 0;
    }

    //TfnHandler format: fnHandler(T &t)/fnHandler(const T &t)
    template <typename TfnHandler>
    int pop(TfnHandler handler)
    {
        uint64_t read_index = read_index_;
        if (max_read_index_.load(std::memory_order_relaxed) > read_index) {
            uint64_t offset = read_index & (array_.size() - 1);

            //处理事件
            handler(array_[offset]);

            read_index_ = read_index + 1;
            return 1;
        }

        return 0;
    }

private:
    static constexpr int PADDING_SIZE1 = (CACHE_LINE_SIZE - sizeof(uint64_t));
    static constexpr int PADDING_SIZE2 = (CACHE_LINE_SIZE - sizeof(AtomicUInt64));

    volatile uint64_t read_index_ = 0;      //读位置(单调递增:只增不减)
    char padding1_[PADDING_SIZE1];

    AtomicUInt64 write_index_     = { 0 };  //写位置(单调递增:只增不减)
    char padding2_[PADDING_SIZE2];

    //更新write_index_和真实的写入数据是两个步骤
    //所以引入max_read_index_，写入完成后更新max_read_index_
    AtomicUInt64 max_read_index_  = { 0 };   //最大读位置(单调递增:只增不减)
    char padding3_[PADDING_SIZE2];

    std::vector<T> array_;
};

// 单生产者多消费者 数组无锁队列
//  multiple producer multiple consumer array lockfree queue
template <typename T, uint_t N>
class SPMCArrayLockfreeQueue
{
public:
    SPMCArrayLockfreeQueue()
    {
        read_index_.store(0, std::memory_order_relaxed);
        min_write_index_.store(0, std::memory_order_relaxed);

        //将取模(求余数%)转换为 算术位运算与(&)
        //计算大于等于capacity的最小 2的N次方整数
        auto log2x = std::log2(N);
        auto size = static_cast<uint_t>(std::pow(2, std::ceil(log2x)));
        array_.resize(size);
    }

    ~SPMCArrayLockfreeQueue() = default;

    inline uint64_t capacity() const
    {
        return array_.size();
    }

    inline uint64_t size()
    {
        return write_index_ - read_index_.load(std::memory_order_relaxed);
    }

    inline uint64_t free_size()
    {
        return array_.size() - size();
    }

    inline bool empty() const
    {
        return write_index_ == read_index_.load(std::memory_order_relaxed);
    }

    inline int push(const T &t)
    {
        uint64_t capacity = array_.size();
        uint64_t write_index;

        for (int i = 0; i < SPIN_LOOP_TIMES; ++i) {
            write_index = write_index_;
            if (write_index - min_write_index_.load(std::memory_order_relaxed) < capacity) {
                uint64_t offset = write_index & (capacity - 1);
                array_[offset]  = t;
                write_index_    = write_index + 1;
                return 1;
            }
        }

        return 0;
    }

    inline int push(T &&t)
    {
        uint64_t capacity = array_.size();
        uint64_t write_index;

        for (int i = 0; i < SPIN_LOOP_TIMES; ++i) {
            write_index = write_index_;
            if (write_index - min_write_index_.load(std::memory_order_relaxed) < capacity) {
                uint64_t offset = write_index & (capacity - 1);
                array_[offset]  = std::move(t);
                write_index_    = write_index + 1;
                return 1;
            }
        }

        return 0;
    }

    //TfnHandler format: fnHandler(T &t)/fnHandler(const T &t)
    template <typename TfnHandler>
    int pop(TfnHandler handler)
    {
        uint64_t capacity = array_.size();
        uint64_t read_index;

        for (int i = 0; i < SPIN_LOOP_TIMES; ++i) {
            read_index = read_index_.load(std::memory_order_relaxed);
            if (write_index_ > read_index) {
                if (read_index_.compare_exchange_weak(read_index, read_index + 1, 
                    std::memory_order_relaxed, std::memory_order_relaxed)) {

                    //在read_index(不能用read_index_)位置读数据
                    uint64_t offset = read_index & (capacity - 1);

                    //处理事件
                    handler(array_[offset]);

                    //发布刚读数据的位置(更新最小写位置)                                             
                    while (!min_write_index_.compare_exchange_weak(read_index, 
                        read_index + 1, std::memory_order_relaxed));

                    return 1;
                }
            }
            else {
                return 0;
            }
        }

        return 0;
    }

protected:
    static constexpr int PADDING_SIZE1 = (CACHE_LINE_SIZE - sizeof(AtomicUInt64));
    static constexpr int PADDING_SIZE2 = (CACHE_LINE_SIZE - sizeof(uint64_t));

    AtomicUInt64 read_index_       = { 0 }; //读位置(单调递增:只增不减)
    char padding1_[PADDING_SIZE1];

    volatile uint64_t write_index_ = 0;     //写位置(单调递增:只增不减)
    char padding2_[PADDING_SIZE2];

    AtomicUInt64 min_write_index_  = { 0 }; //最小写位置(单调递增:只增不减)
    char padding3_[PADDING_SIZE1];

    std::vector<T> array_;
};

// 双指针交换队列: double pointer swap queue
//  用于单消费者场景
template <typename T, typename TMutex = SpinlockMutex, typename TContainer = std::deque<T> >
class DoublePointerSwapQueue
{
public:
    inline DoublePointerSwapQueue()
    {
        active_queue_ = &queue1_;
        standby_queue_ = &queue2_;
    }
    inline ~DoublePointerSwapQueue() = default;

    inline int push(const T& value)
    {
        mutex_.lock();
        standby_queue_->push_back(value);
        mutex_.unlock();

        return 1;
    }

    inline int push(T&& value)
    {
        mutex_.lock();
        standby_queue_->push_back(std::move(value));
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

    inline const TContainer *active_queue() const
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
