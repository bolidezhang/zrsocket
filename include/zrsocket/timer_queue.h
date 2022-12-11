// Some compilers (e.g. VC++) benefit significantly from using this. 
// We've measured 3-4% build speed improvements in apps as a result 
#pragma once

#ifndef ZRSOCKET_TIMER_QUEUE_H
#define ZRSOCKET_TIMER_QUEUE_H
#include <list>
#include <unordered_map>
#include <map>
#include "mutex.h"
#include "timer.h"
#include "event_loop.h"

ZRSOCKET_NAMESPACE_BEGIN

//相同间隔的定时器在同一TimerList中
template <class TMutex>
class TimerQueue : public ITimerQueue
{
public:
    TimerQueue()
    {
    }

    virtual ~TimerQueue()
    {
        interval_timers_.clear();
        timeout_timers_.clear();
    }

    int add_timer(ITimer *itimer, uint64_t current_timestamp)
    {
        Timer *timer = static_cast<Timer *>(itimer);
        mutex_.lock();
        if (timer->enabled_ && (nullptr == timer->timer_queue_)) {
            int64_t interval = timer->interval_;
            timer->expire_time_ = current_timestamp + interval;
            timer->timer_queue_ = this;
            auto pair = interval_timers_.emplace(interval, interval);
            TimerList *tl = &(pair.first->second);
            tl->push_back(timer);
            mutex_.unlock();
            return 1;
        }
        mutex_.unlock();

        return 0;
    }

    int delete_timer(ITimer *itimer)
    {
        Timer *timer = static_cast<Timer *>(itimer);
        mutex_.lock();
        if (nullptr != timer->timer_queue_) {
            timer->timer_queue_ = nullptr;
            TimerList *tl = find_timerlist_i(timer->interval_);
            if (nullptr != tl) {
                tl->remove(timer);
                mutex_.unlock();
                return 1;
            }
        }
        mutex_.unlock();

        return 0;
    }

    int loop(uint64_t current_timestamp)
    {
        mutex_.lock();
        for (auto &iter : interval_timers_) {
            TimerList &tl = iter.second;
            while (!tl.empty()) {
                Timer *timer = tl.front();
                int64_t difference = current_timestamp - timer->expire_time_;
                if (difference >= 0) {
                    timer->timer_queue_ = nullptr;
                    timeout_timers_.emplace_back(std::move(timer));
                    tl.pop_front();
                }
                else {
                    break;
                }
            }
        }
        mutex_.unlock();

        auto ret_size = timeout_timers_.size();
        if (!timeout_timers_.empty()) {
            for (auto &timer : timeout_timers_) {
                timer->handle_timeout();
                if (timer->timer_type_ == Timer::TimerType::CYCLE) {
                    add_timer(timer, current_timestamp);
                }
            }
            timeout_timers_.clear();
        }

        return ret_size;
    }

    int64_t min_interval()
    {
        int64_t min_value = -1;

        mutex_.lock();
        if (!interval_timers_.empty()) {
            auto iter = interval_timers_.begin();
            min_value = iter->first;
        }
        mutex_.unlock();

        return min_value;
    }

    size_t size()
    {
        mutex_.lock();
        size_t ret = size_i();
        mutex_.unlock();
        return ret;
    }

    bool empty()
    {
        return size() == 0;
    }

    EventLoop * event_loop() const
    {
        return event_loop_;
    }

    void event_loop(EventLoop *e)
    {
        event_loop_ = e;
    }

private:
    inline size_t size_i() const
    {
        size_t ret = 0;
        for (auto &iter : interval_timers_) {
            ret += iter.second.size();
        }
        return ret;
    }

    inline TimerList * find_timerlist_i(int64_t interval)
    {
        auto iter = interval_timers_.find(interval);
        if (iter != interval_timers_.end()) {
            return &iter->second;
        }
        return nullptr;
    }
    
private:
    TMutex mutex_;

    //定时器集合
    //interval越小,可能最先到期,需要按interval排序,所以选择sorted associative container的map
    //key:value <interval, TimerList>
    std::map<int64_t, TimerList> interval_timers_;
    
    //到期定时器列表
    std::list<Timer *> timeout_timers_;

    //所属EventLoop
    EventLoop *event_loop_ = nullptr;
};

//class TimeWheelTimerQueue : public ITimerQueue
//{
//};
//
//class MinHeapTimerQueue : public ITimerQueue
//{
//};
//
//class RbtreeTimerQueue : public ITimerQueue
//{
//};

ZRSOCKET_NAMESPACE_END

#endif
