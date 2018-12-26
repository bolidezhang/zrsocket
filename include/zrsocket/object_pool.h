//**********************************************************************
//
// Copyright (C) 2005-2007 Bolide Zhang(bolidezhang@gmail.com)
// All rights reserved.
//
// This copy of zrsoket is licensed to you under the terms described 
// in the LICENSE.txt file included in this distribution.
//
//**********************************************************************

#ifndef ZRSOCKET_OBJECT_POOL_H_
#define ZRSOCKET_OBJECT_POOL_H_
#include <list>
#include "config.h"
#include "base_type.h"

ZRSOCKET_BEGIN

template <typename T, typename TMutex>
class ZRSocketObjectPool
{
public:
    ZRSocketObjectPool()
        : max_chunk_size_(10000)
        , per_chunk_size_(10)
        , current_chunk_size_(0)
    {
    }

    ~ZRSocketObjectPool()
    {
        clear();
    }

    inline int clear()
    {
        mutex_.lock();
        if (!object_chunks_.empty()) {
            typename std::list<T*>::iterator iter = object_chunks_.begin();
            typename std::list<T*>::iterator iter_end = object_chunks_.end();
            for (; iter != iter_end; ++iter) {
                delete [](*iter);
            }
            object_chunks_.clear();
        }
        free_objects_.clear();
        current_chunk_size_ = 0;
        mutex_.unlock();

        return 0;
    }

    inline int init(uint_t max_chunk_size = 10000, uint_t init_chunk_size = 1, uint_t per_chunk_size = 10)
    {
        clear();

        if (max_chunk_size == 0) {
            max_chunk_size = 1;
        }
        if (init_chunk_size > max_chunk_size) {
            init_chunk_size = max_chunk_size;
        }
        if (per_chunk_size == 0) {
            per_chunk_size = 1;
        }

        mutex_.lock();
        max_chunk_size_ = max_chunk_size;
        per_chunk_size_ = per_chunk_size;
        for (uint_t i = 0; i < init_chunk_size; ++i) {
            alloc_i();
        }
        mutex_.unlock();

        return 0;
    }

    inline T* pop()
    {
        T *object;
        mutex_.lock();
        if (!free_objects_.empty()) {
            object = free_objects_.back();
            free_objects_.pop_back();
            mutex_.unlock();
            return object;
        }

        if (current_chunk_size_ >= max_chunk_size_) {
            mutex_.unlock();
            return NULL;
        }

        alloc_i();
        object = free_objects_.back();
        free_objects_.pop_back();
        mutex_.unlock();

        return object;
    }

    inline void push(T *object)
    {
        mutex_.lock();
        free_objects_.emplace_back(object);
        mutex_.unlock();
    }

private:
    ZRSocketObjectPool(const ZRSocketObjectPool &) = delete;
    void operator= (const ZRSocketObjectPool &) = delete;

    inline bool alloc_i()
    {
        T *chunk = new T[per_chunk_size_];
        for (uint_t i = 0; i < per_chunk_size_; ++i) {
            free_objects_.emplace_back(&chunk[i]);
        }
        object_chunks_.emplace_back(chunk);
        ++current_chunk_size_;
        return true;
    }

    uint_t  max_chunk_size_;      //�����������
    uint_t  per_chunk_size_;      //ÿ���ж�����
    uint_t  current_chunk_size_;

    TMutex  mutex_;
    std::list<T*> free_objects_;
    std::list<T*> object_chunks_;
};

template <typename T, typename TMutex>
class SimpleObjectPool
{
public:
    SimpleObjectPool()
        : max_size_(100000)
        , grow_size_(10)
        , free_size_(0)
    {
    }

    ~SimpleObjectPool()
    {
        clear();
    }

    inline int clear()
    {
        mutex_.lock();
        typename std::list<T*>::iterator iter = free_objects_.begin();
        typename std::list<T*>::iterator iter_end = free_objects_.end();
        for (; iter != iter_end; ++iter) {
            delete *iter;
        }
        free_objects_.clear();
        free_size_ = 0;
        mutex_.unlock();
        return 0;
    }

    inline int init(size_t max_size = 100000, uint_t init_size = 1, uint_t grow_size = 10)
    {
        clear();

        if (max_size == 0) {
            max_size = 1;
        }
        if (grow_size < 1) {
            grow_size = 1;
        }
        if (init_size > max_size) {
            init_size = max_size;
        }
        if (grow_size > max_size) {
            grow_size = max_size;
        }

        mutex_.lock();
        max_size_   = max_size;
        grow_size_  = grow_size;
        alloc_i(init_size);
        mutex_.unlock();

        return 0;
    }

    inline T* pop()
    {
        T *object;
        mutex_.lock();
        if (!free_objects_.empty()) {
            object = free_objects_.back();
            free_objects_.pop_back();
            --free_size_;
            mutex_.unlock();
        }
        else
        {
            alloc_i(grow_size_);
            object = free_objects_.back();
            free_objects_.pop_back();
            --free_size_;
            mutex_.unlock();
        }

        return object;
    }

    inline void push(T *object)
    {
        mutex_.lock();
        if (free_size_ < max_size_) {
            free_objects_.emplace_back(object);
            ++free_size_;
            mutex_.unlock();
        }
        else {
            mutex_.unlock();
            delete object;
        }
    }

private:
    SimpleObjectPool(const SimpleObjectPool &) = delete;
    void operator= (const SimpleObjectPool &) = delete;

    inline bool alloc_i(uint_t size)
    {
        T *object;
        for (uint_t i = 0; i < size; ++i) {
            object = new T();
            free_objects_.emplace_back(object);
        }
        free_size_ += size;
        return true;
    }

private:
    size_t  max_size_;   //������������
    uint_t  grow_size_;  //��������
    uint_t  free_size_;

    TMutex  mutex_;
    std::list<T*> free_objects_;
};

ZRSOCKET_END

#endif
