#ifndef ZRSOCKET_SEDA_STAGE_THREAD_H
#define ZRSOCKET_SEDA_STAGE_THREAD_H

#include <algorithm>
#include <vector>
#include "config.h"
#include "base_type.h"
#include "os_api.h"
#include "thread.h"
#include "mutex.h"
#include "atomic.h"
#include "seda_interface.h"
#include "seda_event.h"
#include "seda_event_queue.h"
#include "seda_timer_queue.h"

ZRSOCKET_NAMESPACE_BEGIN

template <typename TSedaStageHandler, typename TQueue>
class SedaStageThread : public ISedaStageThread
{
public:
    SedaStageThread(ISedaStage *stage, uint_t thread_index, uint_t queue_max_size, uint_t event_len, uint_t timedwait_interval_us, bool timedwait_signal)
        : stage_(stage)
        , thread_index_(thread_index)
        , timer_event_flag_(false)
        , timer_min_interval_ms_(ZRSOCKET_SEDA_DEFAULT_IDLE_INTERVAL)
        , timedwait_interval_us_(timedwait_interval_us)
        , timedwait_flag_(false)
        , timedwait_signal_(timedwait_signal)
    {
        stage_handler_.set_stage_thread(this);
        event_queue_.init(queue_max_size, event_len);
    }

    virtual ~SedaStageThread() = default;

    inline int start()
    {
        return thread_.start(thread_proc, this);
    }

    inline int stop()
    {
        SedaQuitEvent quit_event;
        event_queue_.push(&quit_event);
        timedwait_condition_.notify_one();
        return thread_.stop();
    }

    inline int join()
    {
        return thread_.join();
    }

    inline int push_event(const SedaEvent *event)
    {
        int ret = event_queue_.push(event);
        if ((ret > 0) && timedwait_signal_) {
            if (timedwait_flag_.load()) {
                timedwait_flag_.store(false);
                timedwait_condition_.notify_one();
            }
        }
        return ret;
    }

    inline SedaTimer * set_timer(uint_t interval_ms, SedaTimer::TimerParam param)
    {
        if ( (interval_ms < timer_min_interval_ms_) || (0 == timer_min_interval_ms_) ) {
            timer_min_interval_ms_ = interval_ms;
        }
        return timer_queue_.set_timer(interval_ms, param);
    }

    inline int cancel_timer(SedaTimer *timer)
    {
        return timer_queue_.cancel_timer(timer);
    }

    inline int update_timer(SedaTimer *timer)
    {
        return timer_queue_.update_timer(timer);
    }

    inline int enable_timer_event(uint_t capacity)
    {
        return timer_queue_.init(capacity, 0, &stage_handler_, -1);
    }

    inline SedaTimer * set_lru_timer(int slot, SedaTimer::TimerParam param)
    {
        if ((slot < 0) || (slot >= (int)lru_timer_managers_.size())) {
            return nullptr;
        }
        return lru_timer_managers_[slot].set_timer(0, param);
    }

    inline int cancel_lru_timer(int slot, SedaTimer *timer)
    {
        if ((slot < 0) || (slot >= (int)lru_timer_managers_.size())) {
            return -1;
        }
        return lru_timer_managers_[slot].cancel_timer(timer);
    }

    inline int update_lru_timer(int slot, SedaTimer *timer)
    {
        if ((slot < 0) || (slot >= (int)lru_timer_managers_.size())) {
            return -1;
        }
        return lru_timer_managers_[slot].update_timer(timer);
    }

    inline int enable_lru_timers(SedaStageThreadLRUSlotInfo *slots, int slot_count)
    {
        if ((slot_count > 0) && lru_timer_managers_.empty()) {
            lru_timer_managers_.resize(slot_count);
            for (int i=0; i<slot_count; ++i) {
                int ret = lru_timer_managers_[i].init(slots[i].capacity, slots[i].interval_ms, &stage_handler_, i);
                if (ret < 0) {
                    return ret;
                }
                if ( (slots[i].interval_ms < timer_min_interval_ms_) || (0 == timer_min_interval_ms_) ) {
                    timer_min_interval_ms_ = slots[i].interval_ms;
                }
            }
            timer_event_flag_ = true;
            return 0;
        }
        return -1;
    }

    inline int set_sleep_interval(uint_t sleep_interval_us)
    {
        timedwait_interval_us_ = sleep_interval_us;
        return 0;
    }

    inline int set_timedwait_interval(uint_t timedwait_interval_us)
    {
        timedwait_interval_us_ = timedwait_interval_us;
        return 0;
    }

    inline ISedaStage * get_stage() const
    {
        return stage_;
    }

    inline uint_t get_thread_index() const
    {
        return thread_index_;
    }

private:
    inline void check_timers(uint64_t current_clock_ms, SedaTimerExpireEvent *event)
    {
        int timer_size = (int)lru_timer_managers_.size();
        for (int i = 0; i < timer_size; ++i) {
            lru_timer_managers_[i].expire(current_clock_ms, event);
        }

        //普通定时器很少使用,故取消
        //timer_queue_.expire(current_clock_ms, event);
    }

    static int thread_proc(void *arg)
    {
        SedaStageThread<TSedaStageHandler, TQueue> *stage_thread = static_cast<SedaStageThread<TSedaStageHandler, TQueue> *>(arg);
        TSedaStageHandler &stage_handler = stage_thread->stage_handler_;
        AtomicInt &timedwait_flag = stage_thread->timedwait_flag_;
        Mutex &timedwait_mutex = stage_thread->timedwait_mutex_;
        Condition &timedwait_condition = stage_thread->timedwait_condition_;
        TQueue &event_queue = stage_thread->event_queue_;
        stage_handler.handle_open();

        uint64_t current_clock_ms    = OSApi::timestamp_ms();
        int      timer_event_count   = 0;
        bool     timer_event_flag    = stage_thread->timer_event_flag_;

        uint_t timedwait_interval_us  = stage_thread->timer_min_interval_ms_ * 1000;
        if (timedwait_interval_us > stage_thread->timedwait_interval_us_) {
            timedwait_interval_us = stage_thread->timedwait_interval_us_;
        }

        SedaTimerExpireEvent timer_expire_event;
        SedaEvent *event = nullptr;

        if (timer_event_flag) {
            for (;;) {
                event = event_queue.pop(stage_handler);
                if (nullptr != event) {
                    if (SedaEventTypeId::QUIT_EVENT != event->type()) {
                        ++timer_event_count;
                        if (ZRSOCKET_SEDA_TIMER_EVENT_FACTOR == timer_event_count) {
                            stage_thread->check_timers(OSApi::timestamp_ms(), &timer_expire_event);
                            timer_event_count = 0;
                        }
                    }
                    else {
                        break;
                    }
                }
                else {
                    current_clock_ms = OSApi::timestamp_ms();
                    stage_thread->check_timers(current_clock_ms, &timer_expire_event);
                    timer_event_count = 0;

                    if (!event_queue.swap_buffer()) {
                        timedwait_flag.store(true);
                        {
                            std::unique_lock<std::mutex> lock(stage_thread->timedwait_mutex_);
                            timedwait_condition.wait_for(lock, std::chrono::microseconds(timedwait_interval_us));
                        }
                        timedwait_flag.store(false);
                        event_queue.swap_buffer();
                    }
                }
            }
        }
        else { // timer_event_flag == false
            for (;;) {
                event = event_queue.pop(stage_handler);
                if (nullptr != event) {
                    if (SedaEventTypeId::QUIT_EVENT == event->type()) {
                        break;
                    }
                }
                else if (!event_queue.swap_buffer()) {
                    timedwait_flag.store(true);
                    {
                        std::unique_lock<std::mutex> lock(timedwait_mutex);
                        timedwait_condition.wait_for(lock, std::chrono::microseconds(timedwait_interval_us));
                    }
                    timedwait_flag.store(false);
                    event_queue.swap_buffer();
                }
            }
        }

        stage_handler.handle_close();
        return 0;
    }

private:
    ISedaStage                     *stage_;
    TSedaStageHandler               stage_handler_;
    Thread                          thread_;
    uint_t                          thread_index_;          //线程在线程集合中索引

    bool                            timer_event_flag_;      //定时器事件触发标志
    uint_t                          timer_min_interval_ms_; //定时器最小间隔(ms)
    SedaTimerQueue                  timer_queue_;
    std::vector<SedaLRUTimerQueue>  lru_timer_managers_;

    Mutex                           timedwait_mutex_;
    Condition                       timedwait_condition_;
    uint_t                          timedwait_interval_us_;
    AtomicBool                      timedwait_flag_;        //条件触发标识
    bool                            timedwait_signal_;      //用于控制是否触发条件信号(因触发条件信号比较耗时)

    TQueue                          event_queue_;
};

ZRSOCKET_NAMESPACE_END

#endif
