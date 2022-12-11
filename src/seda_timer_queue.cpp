#include "zrsocket/os_api.h"
#include "zrsocket/seda_timer_queue.h"
#include "zrsocket/seda_event.h"

ZRSOCKET_NAMESPACE_BEGIN

SedaTimerQueue::SedaTimerQueue()
    : stage_handler_(nullptr)
    , pool_(nullptr)
    , last_clock_ms_(0)
    , queue_max_size_(0)
{
}

SedaTimerQueue::~SedaTimerQueue()
{
    clear();
}

int SedaTimerQueue::init(uint_t queue_max_size, uint_t interval_ms, ISedaStageHandler *stage_handler, int slot)
{
    clear();

    pool_ = new SedaTimer[queue_max_size];
    if (nullptr == pool_) {
        return -1;
    }

    for (uint_t i = 0; i < queue_max_size; ++i) {
#ifdef ZRSOCKET_DEBUG
        pool_[i].param_ = i;
#endif
        free_list_.push_back(&pool_[i]);
    }
    stage_handler_ = stage_handler;
    return 0;
}

int SedaTimerQueue::clear()
{
    if (nullptr != pool_) {
        delete []pool_;
        pool_ = nullptr;
    }
    free_list_.init();
    active_list_.init();
    last_clock_ms_  = 0;
    queue_max_size_ = 0;
    stage_handler_  = nullptr;
    return 0;
}

SedaTimer * SedaTimerQueue::set_timer(uint_t interval_ms, SedaTimer::TimerParam param)
{
    if (!free_list_.empty()) {
        SedaTimer *timer = free_list_.front();
        free_list_.pop_front();
        timer->init();
        timer->param_        = param;
        timer->interval_ms_  = interval_ms;
        timer->end_clock_ms_ = OSApi::timestamp_ms() + interval_ms;
        timer->is_active_    = true;

        //insert to active_list_
        if (active_list_.empty()) {
            active_list_.push_back(timer);
        }
        else {
            SedaTimer *pos = active_list_.back();
            uint_t active_list_size = active_list_.size();
            for (uint_t i=0; i<active_list_size; ++i) {
                if (timer->end_clock_ms_ > pos->end_clock_ms_) {
                    active_list_.insert_after(pos, timer);
                    return timer;
                }
                pos = pos->prev_;
            }
            active_list_.push_front(timer);
        }
        return timer;
    }
    return nullptr;
}

int SedaTimerQueue::cancel_timer(SedaTimer *timer)
{
    if (nullptr != timer) {
        if (!timer->is_active_) {
            return -1;
        }
        active_list_.remove(timer);
        free_list_.push_front(timer);
    }
    return 0;
}

int SedaTimerQueue::update_timer(SedaTimer *timer)
{
    if (nullptr != timer) {
        if (!timer->is_active_) {
            return -1;
        }
        active_list_.remove(timer);
        timer->end_clock_ms_ = OSApi::timestamp_ms() + timer->interval_ms_;
        timer->is_active_    = true;

        //insert to active_list_
        if (active_list_.empty()) {
            active_list_.push_back(timer);
        }
        else {
            SedaTimer *pos = active_list_.back();
            uint_t active_list_size = active_list_.size();
            for (uint_t i=0; i<active_list_size; ++i) {
                if (timer->end_clock_ms_ > pos->end_clock_ms_) {
                    active_list_.insert_after(pos, timer);
                    return 0;
                }
                pos = pos->prev_;
            }
            active_list_.push_front(timer);
        }
    }
    return 0;
}

int SedaTimerQueue::expire(uint64_t current_clock_ms, SedaTimerExpireEvent *event)
{
    if (current_clock_ms > last_clock_ms_) {
        last_clock_ms_ = current_clock_ms;
    }
    else {
        return 0;
    }

    if (nullptr != stage_handler_) {
        while (!active_list_.empty()) {
            SedaTimer *timer = active_list_.front();
            if (current_clock_ms >= timer->end_clock_ms_) {
                active_list_.pop_front();
                free_list_.push_front(timer);
                event->slot  = -1;
                event->timer = timer;
                stage_handler_->handle_event(event);
            }
            else {
                return 0;
            }
        }
    }
    else {
        while (!active_list_.empty()) {
            SedaTimer *timer = active_list_.front();
            if (current_clock_ms >= timer->end_clock_ms_) {
                active_list_.pop_front();
                free_list_.push_front(timer);
                timer->timeout();
            }
            else {
                return 0;
            }
        }
    }
    return 0;
}

SedaLRUTimerQueue::SedaLRUTimerQueue()
    : stage_handler_(nullptr)
    , pool_(nullptr)
    , last_clock_ms_(0)
    , interval_ms_(0)
    , queue_max_size_(0)
    , slot_(-1)
{
}

SedaLRUTimerQueue::~SedaLRUTimerQueue()
{
    clear();
}

int SedaLRUTimerQueue::init(uint_t queue_max_size, uint_t interval_ms, ISedaStageHandler *stage_handler, int slot)
{
    clear();

    pool_ = new SedaTimer[queue_max_size];
    if (nullptr == pool_) {
        return -1;
    }

    for (uint_t i=0; i<queue_max_size; ++i) {
#ifdef ZRSOCKET_DEBUG
        pool_[i].param_ = i;
#endif
        free_list_.push_back(&pool_[i]);
    }
    interval_ms_    = interval_ms;
    stage_handler_  = stage_handler;
    slot_           = slot;
    return 0;
}

int SedaLRUTimerQueue::clear()
{
    if (nullptr != pool_) {
        delete []pool_;
        pool_ = nullptr;
    }
    free_list_.init();
    active_list_.init();
    last_clock_ms_  = 0;
    queue_max_size_ = 0;
    stage_handler_  = nullptr;
    return 0;
}

SedaTimer * SedaLRUTimerQueue::set_timer(uint_t interval_ms, SedaTimer::TimerParam param)
{
    if (!free_list_.empty()) {
        SedaTimer *timer = free_list_.front();
        free_list_.pop_front();
        timer->init();
        timer->param_        = param;
        timer->interval_ms_  = interval_ms_;
        timer->end_clock_ms_ = OSApi::timestamp_ms() + interval_ms_;
        timer->is_active_    = true;

        //insert to active list
        active_list_.push_back(timer);
        return timer;
    }
    return nullptr;
}

int SedaLRUTimerQueue::cancel_timer(SedaTimer *timer)
{
    if (nullptr != timer) {
        if (!timer->is_active_) {
            return -1;
        }
        active_list_.remove(timer);
        free_list_.push_front(timer);
    }
    return 0;
}

int SedaLRUTimerQueue::update_timer(SedaTimer *timer)
{
    if (nullptr != timer) {
        if (!timer->is_active_) {
            return -1;
        }
        active_list_.remove(timer);
        timer->end_clock_ms_ = OSApi::timestamp_ms() + timer->interval_ms_;
        timer->is_active_    = true;

        //insert to active list
        active_list_.push_back(timer);
    }
    return 0;
}

int SedaLRUTimerQueue::expire(uint64_t current_clock_ms, SedaTimerExpireEvent *event)
{
    if (current_clock_ms > last_clock_ms_) {
        last_clock_ms_ = current_clock_ms;
    }
    else {
        return 0;
    }

    if (nullptr != stage_handler_) {
        while (!active_list_.empty()) {
            SedaTimer *timer = active_list_.front();
            if (current_clock_ms >= timer->end_clock_ms_) {
                active_list_.pop_front();
                free_list_.push_front(timer);
                event->slot  = slot_;
                event->timer = timer;
                stage_handler_->handle_event(event);
            }
            else {
                return 0;
            }
        }
    }
    else {
        while (!active_list_.empty()) {
            SedaTimer *timer = active_list_.front();
            if (current_clock_ms >= timer->end_clock_ms_) {
                active_list_.pop_front();
                free_list_.push_front(timer);
                timer->timeout();
            }
            else {
                return 0;
            }
        }
    }
    return 0;
}

ZRSOCKET_NAMESPACE_END
