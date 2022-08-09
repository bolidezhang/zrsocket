// Some compilers (e.g. VC++) benefit significantly from using this. 
// We've measured 3-4% build speed improvements in apps as a result 
#pragma once

#ifndef ZRSOCKET_TIMER_H
#define ZRSOCKET_TIMER_H
#include "timer_interface.h"

ZRSOCKET_NAMESPACE_BEGIN

class ITimerQueue;
class Timer : public ITimer
{
public:
    Timer()
        : timer_type_(TimerType::CYCLE)
        , interval_(0)
        , enabled_(true)
        , timer_queue_(nullptr)
        , expire_time_(0)
    {
        data_.ptr = nullptr;
        prev_ = this;
        next_ = this;
    }

    ~Timer()
    {
        cancel_timer();
    }

    inline int init()
    {
        if (nullptr == timer_queue_) {
            prev_        = this;
            next_        = this;
            interval_    = 0;
            enabled_     = true;
            expire_time_ = 0;
            return 1;
        }
        return 0;
    }

    inline Timer * prev() const
    {
        return prev_;
    }

    inline Timer * next() const
    {
        return next_;
    }

    inline bool alone() const
    {
        return next_ == this;
    }

    inline int64_t interval() const
    {
        return interval_;
    }

    inline void interval(int64_t interval)
    {
        if (nullptr == timer_queue_) {
            interval_ = interval;
        }
    }

    //是否激活
    inline bool enabled() const
    {
        return enabled_;
    }

    inline void enabled(bool enabled)
    {
        enabled_ = enabled;
    }

    void cancel_timer()
    {
        if (nullptr != timer_queue_) {
            auto event_loop = timer_queue_->event_loop();
            if (nullptr != event_loop) {
                event_loop->delete_timer(this);
            }
            else {
                timer_queue_->delete_timer(this);
            }
        }
    }

    virtual int handle_timeout()
    {
        return 0;
    }

public:
    union TimerData
    {
        int64_t id;
        void   *ptr;
    } data_;

    enum class TimerType
    {
        ONCE = 0,
        CYCLE
    } timer_type_;

protected:
    int64_t     interval_;      //定时间隔
    bool        enabled_;

private:
    Timer       *next_;          //初始指向其自身this,防止因nullptr而出现异常
    Timer       *prev_;          //初始指向其自身this,防止因nullptr而出现异常
    ITimerQueue *timer_queue_;   //定时器队列
    uint64_t     expire_time_;   //定时器触发时间点

    friend class TimerList;
    template <class TMutex> friend class TimerQueue;
};

class TimerList
{
public:
    TimerList() = default;
    ~TimerList() = default;

    TimerList(int64_t interval)
        : interval_(interval)
    {
    }

    TimerList(const TimerList &tl)
        : interval_(tl.interval_)
    {
    }

    TimerList(TimerList && tl)
        : interval_(tl.interval_)
    {
    }

    inline int init()
    {
        head_.init();
        size_ = 0;
        interval_ = 0;
        return 0;
    }

    inline void push_front(Timer *timer)
    {
        timer->prev_ = timer;
        if (head_.alone()) {
            timer->next_ = timer;
            head_.prev_ = timer;
        }
        else {
            timer->next_ = head_.next_;
            head_.next_->prev_ = timer;
        }
        head_.next_ = timer;
        ++size_;
    }

    inline void push_back(Timer *timer)
    {
        timer->next_ = timer;
        if (head_.alone()) {
            timer->prev_ = timer;
            head_.next_ = timer;
        }
        else {
            timer->prev_ = head_.prev_;
            head_.prev_->next_ = timer;
        }
        head_.prev_ = timer;
        ++size_;
    }

    inline void pop_front()
    {
        if (!head_.alone()) {
            head_.next_ = head_.next_->next_;
            head_.next_->prev_ = head_.next_;
            if (--size_ == 0) {
                head_.init();
            }
        }
    }

    inline void pop_back()
    {
        if (!head_.alone()) {
            head_.prev_ = head_.prev_->prev_;
            head_.prev_->next_ = head_.prev_;
            if (--size_ == 0)
            {
                head_.init();
            }
        }
    }

    //insert pos before
    inline void insert_before(Timer *pos, Timer *timer)
    {
        timer->next_ = pos;
        timer->prev_ = pos->prev_;
        if (pos != pos->prev_) {
            pos->prev_->next_ = timer;
        }
        pos->prev_ = timer;
        if (pos == head_.next_) {
            head_.next_ = timer;
            timer->prev_ = timer;
        }
        ++size_;
    }

    //insert pos after
    inline void insert_after(Timer *pos, Timer *timer)
    {
        timer->next_ = pos->next_;
        timer->prev_ = pos;
        if (pos != pos->next_) {
            pos->next_->prev_ = timer;
        }
        pos->next_ = timer;
        if (pos == head_.prev_) {
            head_.prev_ = timer;
            timer->next_ = timer;
        }
        ++size_;
    }

    inline void remove(Timer *timer)
    {
        if (0 == size_) {
            return;
        }

        timer->prev_->next_ = timer->next_;
        timer->next_->prev_ = timer->prev_;
        if (timer == head_.next_) {
            head_.next_ = timer->next_;
            head_.next_->prev_ = head_.next_;
        }
        if (timer == head_.prev_) {
            head_.prev_ = timer->prev_;
            head_.prev_->next_ = head_.prev_;
        }
        if (--size_ == 0) {
            head_.init();
        }
        timer->init();
    }

    inline Timer * front() const
    {
        return head_.next_;
    }

    inline Timer * back() const
    {
        return head_.prev_;
    }

    inline size_t size() const
    {
        return size_;
    }

    inline bool empty() const
    {
        return (0 == size_);
    }

    inline int64_t interval() const
    {
        return interval_;
    }

    inline void interval(int64_t interval)
    {
        interval_ = interval;
    }

protected:
    Timer   head_;          //此节点的next_指向第一个节点，prev_指向最后一个节点
    size_t  size_ = 0;
    int64_t interval_ = 0;  //定时间隔
};

ZRSOCKET_NAMESPACE_END

#endif
