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

template <typename TSedaStageHandler>
class SedaStageThread : public ISedaStageThread
{
public:
    SedaStageThread(ISedaStage *stage, uint_t thread_index, uint_t queue_max_size, uint_t event_len, uint_t timedwait_interval_us, bool timedwait_signal)
        : stage_(stage)
        , thread_index_(thread_index)
        , idle_event_flag_(false)
        , idle_interval_ms_(ZRSOCKET_SEDA_DEFAULT_IDLE_INTERVAL)
        , timer_event_flag_(false)
        , timer_min_interval_ms_(ZRSOCKET_SEDA_DEFAULT_IDLE_INTERVAL)
        , timedwait_interval_us_(timedwait_interval_us)
        , timedwait_flag_(0)
        , timedwait_signal_(timedwait_signal)
    {
        stage_handler_.set_stage_thread(this);
        event_queue1_.init(queue_max_size, event_len);
        event_queue2_.init(queue_max_size, event_len);
        event_queue_active_  = &event_queue1_;
        event_queue_standby_ = &event_queue2_;
    }

    virtual ~SedaStageThread()
    {
    }

    inline int start()
    {
        return thread_.start(thread_proc, this);
    }

    inline int stop()
    {
        SedaQuitEvent quit_event;
        event_queue_standby_->push(&quit_event);
        timedwait_condition_.notify_one();
        return thread_.stop();
    }

    inline int join()
    {
        return thread_.join();
    }

    inline int push_event(const SedaEvent *event)
    {
        event_queue_mutex_.lock();
        int ret = event_queue_standby_->push(event);
        event_queue_mutex_.unlock();
        if ((1 == ret) && timedwait_signal_) {
            if (timedwait_flag_.load(std::memory_order_relaxed) > 0) {
                timedwait_flag_.store(0, std::memory_order_relaxed);
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
            return NULL;
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
        for (int i = 0; i < timer_size; ++i)
        {
            lru_timer_managers_[i].expire(current_clock_ms, event);
        }

        //普通定时器很少使用,故取消
        //timer_queue_.expire(current_clock_ms, event);
    }

    //交换event_queue的 active/standby 指针
    inline bool swap_event_queue()
    {
        if (!event_queue_standby_->empty()) {
            event_queue_mutex_.lock();
            std::swap(event_queue_standby_, event_queue_active_);
            event_queue_mutex_.unlock();
            return true;
        }
        return false;
    }

    static int thread_proc(void *arg)
    {
        SedaStageThread<TSedaStageHandler> *stage_thread = (SedaStageThread<TSedaStageHandler> *)arg;
        stage_thread->stage_handler_.handle_open();

        uint64_t current_clock_ms    = OSApi::timestamp_ms();
        uint64_t idle_last_clock_ms  = current_clock_ms;
        int      timer_event_count   = 0;
        uint_t   idle_interval_ms    = stage_thread->idle_interval_ms_;
        bool     idle_event_flag     = stage_thread->idle_event_flag_;
        bool     timer_event_flag    = stage_thread->timer_event_flag_;

        uint_t timedwait_interval_us  = stage_thread->timer_min_interval_ms_ * 1000;
        if (timedwait_interval_us > stage_thread->timedwait_interval_us_) {
            timedwait_interval_us = stage_thread->timedwait_interval_us_;
        }

        SedaThreadIdleEvent  idle_event;
        SedaTimerExpireEvent timer_expire_event;
        SedaEvent *event = nullptr;

        if (!timer_event_flag) {
            if (!idle_event_flag) { //timer_event_flag = false and idle_event_flag = false
                for (;;) {
                    event = stage_thread->event_queue_active_->pop();
                    if (nullptr != event) {
                        if (SedaEventType::QUIT_EVENT != event->type_) {
                            stage_thread->stage_handler_.handle_event(event);
                        }
                        else {
                            goto QUIT_THREAD_PROC;
                        }
                    }
                    else {
                        if (!stage_thread->swap_event_queue()) {
                            stage_thread->timedwait_flag_.store(1, std::memory_order_relaxed);
                            {
                                std::unique_lock<std::mutex> lock(stage_thread->timedwait_mutex_);
                                stage_thread->timedwait_condition_.wait_for(lock, std::chrono::microseconds(timedwait_interval_us));
                            }
                            stage_thread->timedwait_flag_.store(0, std::memory_order_relaxed);
                            stage_thread->swap_event_queue();
                        }
                    }
                }
            }
            else {  //timer_event_flag = false and idle_event_flag = true
                for (;;) {
                    event = stage_thread->event_queue_active_->pop();
                    if (nullptr != event) {
                        if (SedaEventType::QUIT_EVENT != event->type_) {
                            stage_thread->stage_handler_.handle_event(event);
                        }
                        else {
                            goto QUIT_THREAD_PROC;
                        }
                    }
                    else {
                        current_clock_ms = OSApi::timestamp_ms();
                        if (current_clock_ms - idle_last_clock_ms >= idle_interval_ms) {
                            stage_thread->stage_handler_.handle_event(&idle_event);
                            idle_last_clock_ms = current_clock_ms;
                        }

                        if (!stage_thread->swap_event_queue()) {
                            stage_thread->timedwait_flag_.store(1, std::memory_order_relaxed);
                            {
                                std::unique_lock<std::mutex> lock(stage_thread->timedwait_mutex_);
                                stage_thread->timedwait_condition_.wait_for(lock, std::chrono::microseconds(timedwait_interval_us));
                            }
                            stage_thread->timedwait_flag_.store(0, std::memory_order_relaxed);
                            stage_thread->swap_event_queue();
                        }
                    }
                }
            }
        }
        else
        {
            if (!idle_event_flag) { //timer_event_flag = true and idle_event_flag = false
                for (;;) {
                    event = stage_thread->event_queue_active_->pop();
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
                    else {
                        current_clock_ms = OSApi::timestamp_ms();
                        stage_thread->check_timers(current_clock_ms, &timer_expire_event);
                        timer_event_count = 0;

                        if (!stage_thread->swap_event_queue()) {
                            stage_thread->timedwait_flag_.store(1, std::memory_order_relaxed);
                            {
                                std::unique_lock<std::mutex> lock(stage_thread->timedwait_mutex_);
                                stage_thread->timedwait_condition_.wait_for(lock, std::chrono::microseconds(timedwait_interval_us));
                            }
                            stage_thread->timedwait_flag_.store(0, std::memory_order_relaxed);
                            stage_thread->swap_event_queue();
                        }
                    }
                }
            }
            else {  //timer_event_flag = true and idle_event_flag = true
                for (;;) {
                    event = stage_thread->event_queue_active_->pop();
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
                        current_clock_ms = OSApi::timestamp_ms();
                        stage_thread->check_timers(current_clock_ms, &timer_expire_event);
                        timer_event_count = 0;
                        if (current_clock_ms - idle_last_clock_ms >= idle_interval_ms) {
                            stage_thread->stage_handler_.handle_event(&idle_event);
                            idle_last_clock_ms = current_clock_ms;
                        }

                        if (!stage_thread->swap_event_queue()) {
                            stage_thread->timedwait_flag_.store(1, std::memory_order_relaxed);
                            {
                                std::unique_lock<std::mutex> lock(stage_thread->timedwait_mutex_);
                                stage_thread->timedwait_condition_.wait_for(lock, std::chrono::microseconds(timedwait_interval_us));
                            }
                            stage_thread->timedwait_flag_.store(0, std::memory_order_relaxed);
                            stage_thread->swap_event_queue();
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

    //采用双队列提高性能
    SedaEventQueue                 *event_queue_active_;
    SedaEventQueue                 *event_queue_standby_;
    SedaEventQueue                  event_queue1_;
    SedaEventQueue                  event_queue2_;
    SpinMutex                       event_queue_mutex_;

    ThreadMutex                     timedwait_mutex_;
    Condition                       timedwait_condition_;
    uint_t                          timedwait_interval_us_;
    AtomicInt32                     timedwait_flag_;            //条件触发标识
    bool                            timedwait_signal_;          //用于控制是否触发条件信号(因触发条件信号比较耗时)
};

ZRSOCKET_NAMESPACE_END

#endif
