//1)SedaStage2解决的问题:
//  因SedaStage直接将event推至每个线程的本地队列,
//  若线程处理比较费时, 即使此时其他线程空闲,也不能从本线程的队列取出event进行处理
//2)SedaStage2的实现
//  多线程间共享一个全局队列,使每个线程都有event处理,提高整体的处理效率

#ifndef ZRSOCKET_SEDA_STAGE2_H
#define ZRSOCKET_SEDA_STAGE2_H
#include <vector>
#include "base_type.h"
#include "os_api.h"
#include "thread.h"
#include "mutex.h"
#include "seda_event_queue.h"
#include "seda_stage2_thread.h"

ZRSOCKET_NAMESPACE_BEGIN

template <typename TSedaStageHandler, typename TMutex = SpinlockMutex>
class SedaStage2 : public ISedaStage
{
public:
    SedaStage2()
        : timedwait_interval_us_(10000)
        , timedwait_signal_(true)
        , type_(0)
        , batch_size_(2)
        , is_priority_(false)
    {
    }

    virtual ~SedaStage2()
    {
        close();
    }

    int open(uint_t thread_number,
             uint_t queue_max_size,
             uint_t event_len = 8,
             uint_t timedwait_interval_us = 10000,
             bool   timedwait_signal = true,
             int    type = 0,
             uint_t batch_size = 2,
             bool   is_priority = false,
             bool   is_shared_queue = true)
    {
        if (thread_number < 1) {
            thread_number = 1;
        }
        if (batch_size < 1) {
            batch_size = 1;
        }

        timedwait_interval_us_  = timedwait_interval_us;
        timedwait_signal_       = timedwait_signal;
        type_                   = type;
        batch_size_             = batch_size;
        is_priority_            = is_priority;

        low_priority_queue_.init(queue_max_size, event_len);
        if (is_priority) {
            high_priority_queue_.init(queue_max_size, event_len);
        }

        stage_threads_.reserve(thread_number);
        StageThread *stage_thread;
        for (uint_t i=0; i<thread_number; ++i) {
            stage_thread = new StageThread(this, i, batch_size+1, event_len);
            stage_thread->start();
            stage_threads_.push_back(stage_thread);
        }
        return 0;
    }

    int close()
    {
        StageThread *stage_thread;
        auto iter = stage_threads_.begin();
        auto iter_end = stage_threads_.end();
        for (; iter != iter_end; ++iter) {
            stage_thread = *iter;
            stage_thread->stop();
            stage_thread->join();
            delete stage_thread;
        }
        stage_threads_.clear();
        return 0;
    }

    int join()
    {
        auto iter = stage_threads_.begin();
        auto iter_end = stage_threads_.end();
        for (; iter != iter_end; ++iter) {
            (*iter)->join();
        }
        return 0;
    }

    inline int push_event(const SedaEvent *event, int thread_index = -1, int priority = SedaPriority::UNKNOWN_PRIOITY)
    {
        int ret;
        if (is_priority_ && (SedaPriority::HIGH_PRIORITY == priority)) {
            high_priority_mutex_.lock();
            ret = high_priority_queue_.push(event);
            high_priority_mutex_.unlock();
        }
        else {
            low_priority_mutex_.lock();
            ret = low_priority_queue_.push(event);
            low_priority_mutex_.unlock();
        }
        if (timedwait_signal_ && (1 == ret)) {
            timedwait_condition_.notify_one();
        }
        return ret;
    }

    inline int pop_event(void *push_queue, uint_t batch_size)
    {
        SedaEventQueue *push_queue_tmp = (SedaEventQueue *)push_queue;
        if (is_priority_) {
            high_priority_mutex_.lock();
            high_priority_queue_.pop(push_queue_tmp, batch_size);
            high_priority_mutex_.unlock();
        }
        low_priority_mutex_.lock();
        low_priority_queue_.pop(push_queue_tmp, batch_size);
        low_priority_mutex_.unlock();

        if (push_queue_tmp->empty()) {
            std::unique_lock<std::mutex> lock(timedwait_mutex_);
            timedwait_condition_.wait_for(lock, std::chrono::microseconds(timedwait_interval_us_));
        }
        return 0;
    }

    inline int type() const 
    {
        return type_;
    }

    inline int event_count() const
    {
        return high_priority_queue_.size() + low_priority_queue_.size();
    }

    inline uint_t batch_size() const
    {
        return batch_size_;
    }

protected:
    typedef SedaStage2Thread<TSedaStageHandler> StageThread;
    std::vector<StageThread * > stage_threads_;

    SedaEventQueue  high_priority_queue_;
    TMutex          high_priority_mutex_;
    SedaEventQueue  low_priority_queue_;
    TMutex          low_priority_mutex_;

    ThreadMutex     timedwait_mutex_;
    Condition       timedwait_condition_;
    uint_t          timedwait_interval_us_;
    bool            timedwait_signal_;

    //区分多个实例标识
    int     type_;

    //批处理大小
    uint_t  batch_size_;

    //是否启用优先级
    bool    is_priority_;
};

ZRSOCKET_NAMESPACE_END

#endif
