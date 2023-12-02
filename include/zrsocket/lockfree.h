// Some compilers (e.g. VC++) benefit significantly from using this. 
// We've measured 3-4% build speed improvements in apps as a result 
#pragma once

#ifndef ZRSOCKET_LOCKFREE_H
#define ZRSOCKET_LOCKFREE_H
#include "atomic.h"
#include "byte_buffer.h"

ZRSOCKET_NAMESPACE_BEGIN

static constexpr const int CACHE_LINE_SIZE = 64;          //缓存行大小 x86一般为64
static constexpr const int SPIN_LOOP_TIMES = 1000;        //旋转次数

// 双指针对象: double pointer object
//  用于动态更新配置信息的场景
template <typename TObject>
class DoublePointerObject
{
public:
    inline DoublePointerObject()
    {
        active_ptr_.store(&o1_);
        standby_ptr_.store(&o2_);
    }
    inline ~DoublePointerObject() = default;

    inline TObject * active_ptr() const
    {
        return active_ptr_.load(std::memory_order_relaxed);
    }

    inline TObject * standby_ptr() const
    {
        return standby_ptr_.load(std::memory_order_relaxed);
    }

    //交换active/standby指针
    //只能在standby线程调用
    inline bool swap_pointer()
    {
        TObject *tmp = active_ptr_.load(std::memory_order_relaxed);
        active_ptr_.store(standby_ptr_.load(std::memory_order_relaxed), std::memory_order_relaxed);
        standby_ptr_.store(tmp, std::memory_order_relaxed);
        return true;
    }

protected:
    std::atomic<TObject *> active_ptr_;
    std::atomic<TObject *> standby_ptr_;
    TObject o1_;
    TObject o2_;
};

// 双定长缓冲: double fixed length buffer
//  用于单消费者场景
template <typename TMutex>
class DoubleFixedLengthBuffer
{
public:
    inline DoubleFixedLengthBuffer()
    {
        active_ptr_  = &buf1_;
        standby_ptr_ = &buf2_;
    }

    inline ~DoubleFixedLengthBuffer()
    {
        buf1_.clear();
        buf2_.clear();
    }

    int set_max_size(uint_t max_size = 4096)
    {
        buf1_.reserve(max_size);
        buf2_.reserve(max_size);
        return 1;
    }

    inline uint_t write(const char *data, uint_t size, bool whole = true)
    {
        mutex_.lock();
        uint_t free_size = standby_ptr_->free_size();
        if (free_size >= size) {
            standby_ptr_->write(data, size);
            mutex_.unlock();
            return size;
        }
        else {
            if ((free_size > 0) && (!whole)) {
                standby_ptr_->write(data, free_size);
                mutex_.unlock();
                return free_size;
            }
        }
        mutex_.unlock();

        return 0;
    }

    inline ByteBuffer* active_ptr() const
    {
        return active_ptr_;
    }

    //交换active/standby指针
    //只能在active线程且active_ptr_->empty()==true调用
    inline bool swap_buffer()
    {
        mutex_.lock();
        if (!standby_ptr_->empty()) {
            active_ptr_->reset();
            std::swap(standby_ptr_, active_ptr_);
            mutex_.unlock();
            return true;
        }
        mutex_.unlock();

        return false;
    }

protected:
    ByteBuffer *active_ptr_;
    ByteBuffer *standby_ptr_;
    ByteBuffer  buf1_;
    ByteBuffer  buf2_;
    TMutex      mutex_;
};
ZRSOCKET_NAMESPACE_END

#endif
