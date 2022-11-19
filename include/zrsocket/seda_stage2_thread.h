#ifndef ZRSOCKET_SEDA_STAGE_THREAD2_H
#define ZRSOCKET_SEDA_STAGE_THREAD2_H

#include <algorithm>
#include <vector>
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

template <typename TSedaStageHandler>
class SedaStage2Thread : public ISedaStageThread
{
public:
    SedaStage2Thread(ISedaStage *stage, uint_t thread_index, uint_t queue_max_size, uint_t event_len)
        : stage_(stage)
        , thread_index_(thread_index)
        , idle_event_flag_(false)
        , idle_interval_ms_(ZRSOCKET_SEDA_DEFAULT_IDLE_INTERVAL)
        , timer_event_flag_(false)
        , timer_min_interval_ms_(ZRSOCKET_SEDA_DEFAULT_IDLE_INTERVAL)
    {
        stage_handler_.set_stage_thread(this);
        event_queue_.init(queue_max_size, event_len);
    }

    virtual ~SedaStage2Thread()
    {
    }

    inline int start()
    {
        return thread_.start(thread_proc, this);
    }

    inline int stop()
    {
        SedaQuitEvent quit_event;
        event_queue_.push(&quit_event);
        return thread_.stop();
    }

    inline int join()
    {
        return thread_.join();
    }

    inline int push_event(const SedaEvent *event)
    {
        return 0;
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
        if ((slot < 0) || (slot >= (int)lru_timer_managers_.size()))
        {
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
        if ( (slot_count > 0) && lru_timer_managers_.empty() ) {
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

    inline int enable_idle_event(bool enabled)
    {
        idle_event_flag_ = enabled;
        return 0;
    }

    inline int set_idle_interval(uint_t idle_interval_ms)
    {
        idle_interval_ms_ = idle_interval_ms;
        return 0;
    }

    inline int set_sleep_interval(uint_t sleep_interval_us)
    {
        return 0;
    }

    inline int set_timedwait_interval(uint_t timedwait_interval_us)
    {
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
    }

    static unsigned int thread_proc(void *arg)
    {
        SedaStage2Thread<TSedaStageHandler> *stage_thread = (SedaStage2Thread<TSedaStageHandler> *)arg;
        stage_thread->stage_handler_.handle_open();

        uint64_t    current_clock_ms    = OSApi::timestamp_ms();
        uint64_t    idle_last_clock_ms  = current_clock_ms;
        int         timer_event_count   = 0;
        uint_t      idle_interval_ms    = stage_thread->idle_interval_ms_;
        bool        idle_event_flag     = stage_thread->idle_event_flag_;
        bool        timer_event_flag    = stage_thread->timer_event_flag_;

        SedaThreadIdleEvent  idle_event;
        SedaTimerExpireEvent timer_expire_event;
        SedaEvent *event = nullptr;

        if (!timer_event_flag) {
            if (!idle_event_flag) { //timer_event_flag = false and idle_event_flag = false
                for (;;) {
                    event = stage_thread->event_queue_.pop();
                    if (nullptr != event) {
                        if (SedaEventType::QUIT_EVENT != event->type_) {
                            stage_thread->stage_handler_.handle_event(event);
                        }
                        else {
                            goto QUIT_THREAD_PROC;
                        }
                    }
                    else {
                        stage_thread->stage_->pop_event(&stage_thread->event_queue_, stage_thread->stage_->batch_size());
                    }
                }
            }
            else {  //timer_event_flag = false and idle_event_flag = true
                for (;;) {
                    event = stage_thread->event_queue_.pop();
                    if (nullptr != event) {
                        if (SedaEventType::QUIT_EVENT != event->type_) {
                            stage_thread->stage_handler_.handle_event(event);
                        }
                        else {
                            goto QUIT_THREAD_PROC;
                        }
                    }
                    else {
                        stage_thread->stage_->pop_event(&stage_thread->event_queue_, stage_thread->stage_->batch_size());
                        if (stage_thread->event_queue_.empty()) {
                            current_clock_ms = OSApi::timestamp_ms();
                            if (current_clock_ms - idle_last_clock_ms >= idle_interval_ms) {
                                stage_thread->stage_handler_.handle_event(&idle_event);
                                idle_last_clock_ms = current_clock_ms;
                            }
                        }
                    }
                }
            }
        }
        else
        {
            if (!idle_event_flag) { //timer_event_flag = true and idle_event_flag = false
                for (;;) {
                    event = stage_thread->event_queue_.pop();
                    if (nullptr != event) {
                        if (SedaEventType::QUIT_EVENT != event->type_) {
                            stage_thread->stage_handler_.handle_event(event);

                            ++timer_event_count;
                            if (ZRSOCKET_SEDA_TIMER_EVENT_FACTOR == timer_event_count) {
                                timer_event_count = 0;
                                stage_thread->check_timers(OSApi::timestamp_ms(), &timer_expire_event);
                            }
                        }
                        else
                        {
                            goto QUIT_THREAD_PROC;
                        }
                    }
                    else
                    {
                        current_clock_ms = OSApi::timestamp_ms();
                        stage_thread->check_timers(current_clock_ms, &timer_expire_event);
                        timer_event_count = 0;
                        stage_thread->stage_->pop_event(&stage_thread->event_queue_, stage_thread->stage_->batch_size());
                    }
                }
            }
            else {  //timer_event_flag = true and idle_event_flag = true
                for (;;) {
                    event = stage_thread->event_queue_.pop();
                    if (nullptr != event) {
                        if (SedaEventType::QUIT_EVENT != event->type_) {
                            stage_thread->stage_handler_.handle_event(event);

                            ++timer_event_count;
                            if (ZRSOCKET_SEDA_TIMER_EVENT_FACTOR == timer_event_count) {
                                timer_event_count = 0;
                                stage_thread->check_timers(OSApi::timestamp_ms(), &timer_expire_event);
                            }
                        }
                        else {
                            goto QUIT_THREAD_PROC;
                        }
                    }
                    else {
                        stage_thread->stage_->pop_event(&stage_thread->event_queue_, stage_thread->stage_->batch_size());
                        if (stage_thread->event_queue_.empty()) {
                            current_clock_ms = OSApi::timestamp_ms();
                            stage_thread->check_timers(current_clock_ms, &timer_expire_event);
                            timer_event_count = 0;
                            if (current_clock_ms - idle_last_clock_ms >= idle_interval_ms) {
                                stage_thread->stage_handler_.handle_event(&idle_event);
                                idle_last_clock_ms = current_clock_ms;
                            }
                        }
                    }
                }
            }
        }

QUIT_THREAD_PROC:
        stage_thread->stage_handler_.handle_close();
        return 0;
    }

private:
    ISedaStage                     *stage_;
    TSedaStageHandler               stage_handler_;
    uint_t                          thread_index_;              //线程在线程集合中索引
    Thread                          thread_;

    bool                            idle_event_flag_;
    uint_t                          idle_interval_ms_;
    bool                            timer_event_flag_;          //定时器事件触发标志
    uint_t                          timer_min_interval_ms_;     //定时器最小间隔(ms)
    SedaTimerQueue                  timer_queue_;
    std::vector<SedaLRUTimerQueue>  lru_timer_managers_;

    //线程本地队列
    SedaEventQueue                  event_queue_;
};

ZRSOCKET_NAMESPACE_END

#endif
