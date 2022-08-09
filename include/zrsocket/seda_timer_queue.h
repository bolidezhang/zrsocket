#ifndef ZRSOCKET_SEDA_TIMER_QUEUE_H
#define ZRSOCKET_SEDA_TIMER_QUEUE_H
#include "seda_interface.h"
#include "seda_timer.h"

ZRSOCKET_NAMESPACE_BEGIN

class ZRSOCKET_EXPORT SedaTimerList
{
public:
    SedaTimerList()
        : size_(0)
    {
    }

    ~SedaTimerList()
    {
        init();
    }
    
    inline int init()
    {
        head_.init();
        size_ = 0;
        return 0;
    }

    inline void push_front(SedaTimer *timer)
    {
        timer->prev_ = timer;
        if (head_.alone()) {
            timer->next_ = timer;
            head_.prev_ = timer;
        }
        else {
            timer->next_ = head_.next_;
            head_.next_->prev_  = timer;
        }
        head_.next_ = timer;
        ++size_;
    }

    inline void push_back(SedaTimer *timer)
    {
        timer->next_ = timer;
        if (head_.alone()) {
            timer->prev_ = timer;
            head_.next_  = timer;
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
            if (--size_ == 0) {
                head_.init();
            }
        }
    }

    //insert pos before
    inline void insert_before(SedaTimer *pos, SedaTimer *timer)
    {
        timer->next_ = pos;
        timer->prev_ = pos->prev_;
        if (pos != pos->prev_) {
            pos->prev_->next_ = timer;
        }
        pos->prev_ = timer;
        if (pos == head_.next_) {
            head_.next_  = timer;
            timer->prev_ = timer;
        }
        ++size_;
    }
    
    //insert pos after
    inline void insert_after(SedaTimer *pos, SedaTimer *timer)
    {
        timer->next_ = pos->next_;
        timer->prev_ = pos;
        if (pos != pos->next_) {
            pos->next_->prev_ = timer;
        }
        pos->next_ = timer;
        if (pos == head_.prev_) {
            head_.prev_  = timer;
            timer->next_ = timer;
        }
        ++size_;
    }

    inline void remove(SedaTimer *timer)
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

    inline SedaTimer * front() const
    {
        return head_.next_;
    }

    inline SedaTimer * back() const
    {
        return head_.prev_;
    }

    inline uint_t size() const
    {
        return size_;
    }

    inline bool empty() const
    {
        return (0 == size_);
    }

protected:
    SedaTimer   head_;      //此节点的next_指向第一个节点，prev_指向最后一个节点
    uint_t      size_;
};

class ZRSOCKET_EXPORT SedaTimerQueue
{
public:
    SedaTimerQueue();
    ~SedaTimerQueue();

    int init(uint_t queue_max_size, uint_t interval_ms, ISedaStageHandler *stage_handler = nullptr, int slot = -1);
    int clear();

    SedaTimer * set_timer(uint_t interval_ms, SedaTimer::TimerParam param);
    int cancel_timer(SedaTimer *timer);
    int update_timer(SedaTimer *timer);
    int expire(uint64_t current_clock_ms, SedaTimerExpireEvent *event);

    inline uint_t free_timer_size() const
    {
        return free_list_.size();
    }

    inline uint_t active_timer_size() const
    {
        return active_list_.size();
    }

private:
    ISedaStageHandler  *stage_handler_;

    SedaTimer          *pool_;                 //SedaTimer类型数组池
    SedaTimerList       free_list_;
    SedaTimerList       active_list_;

    uint64_t            last_clock_ms_;
    uint_t              queue_max_size_;
};

//LRUTimerQueue中所有定时器的时间间隔是固定的
class ZRSOCKET_EXPORT SedaLRUTimerQueue
{
public:
    SedaLRUTimerQueue();
    ~SedaLRUTimerQueue();

    int init(uint_t queue_max_size, uint_t interval_ms, ISedaStageHandler *stage_handler = nullptr, int slot = -1);
    int clear();

    SedaTimer * set_timer(uint_t interval_ms, SedaTimer::TimerParam param);
    int cancel_timer(SedaTimer *timer);
    int update_timer(SedaTimer *timer);
    int expire(uint64_t current_clock_ms, SedaTimerExpireEvent *event);

    inline uint_t free_timer_size() const
    {
        return free_list_.size();
    }

    inline uint_t active_timer_size() const
    {
        return active_list_.size();
    }

private:
    ISedaStageHandler  *stage_handler_;

    SedaTimer          *pool_;              //SL_Seda_Timer类型数组池
    SedaTimerList       free_list_;
    SedaTimerList       active_list_;

    uint64_t            last_clock_ms_;
    uint_t              interval_ms_;       //定时间隔(时间单位为ms)
    uint_t              queue_max_size_;
    int                 slot_;
};

ZRSOCKET_NAMESPACE_END

#endif
