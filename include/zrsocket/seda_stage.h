#ifndef ZRSOCKET_SEDA_STAGE_H
#define ZRSOCKET_SEDA_STAGE_H

#include <vector>
#include "base_type.h"
#include "os_api.h"
#include "mutex.h"
#include "seda_stage_thread.h"

ZRSOCKET_NAMESPACE_BEGIN

template <typename TSedaStageHandler, typename TQueue = DoubleBufferEventTypeQueue<SpinlockMutex> >
class SedaStage : public ISedaStage
{
public:
    SedaStage()
        : timedwait_interval_us_(10000)
        , next_thread_index_(0)
        , type_(0)
    {
    }

    virtual ~SedaStage()
    {
        close();
    }

    int open(uint_t thread_number,
             uint_t queue_max_size,
             uint_t event_len = 8,
             uint_t timedwait_interval_us = 10000,
             bool   timedwait_signal = true,
             int    type = 0,
             uint_t batch_size = 0,
             bool   is_priority = false,
             bool   is_shared_queue = false)
    {
        if (thread_number < 1) {
            thread_number = 1;
        }
        type_ = type;
        timedwait_interval_us_ = timedwait_interval_us;
        stage_threads_.reserve(thread_number);

        StageThread *stage_thread;
        for (uint_t i=0; i<thread_number; ++i) {
            stage_thread = new StageThread(this, i, queue_max_size, event_len, timedwait_interval_us, timedwait_signal);
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
        int thread_size = static_cast<int>(stage_threads_.size());
        if (thread_size < 2) {
            thread_index = 0;
        }
        else if ((thread_index < 0) || (thread_index >= thread_size)) {
            thread_index = next_thread_index_;
            next_thread_index_ = (thread_index + 1) % thread_size;
        }

        if (stage_threads_[thread_index]->push_event(event) > 0) {
            return thread_index;
        }
        return -1;
    }

    inline int type() const
    {
        return type_;
    }

    inline int event_count() const
    {
        return 0;
    }

    inline uint_t batch_size() const
    {
        return 0;
    }

protected:
    using StageThread = SedaStageThread<TSedaStageHandler, TQueue>;
    std::vector<StageThread * > stage_threads_;

    uint_t  timedwait_interval_us_;
    int     next_thread_index_;

    //区分多个实例标识
    int     type_;
};

ZRSOCKET_NAMESPACE_END

#endif
