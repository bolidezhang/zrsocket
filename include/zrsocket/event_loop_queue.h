// Some compilers (e.g. VC++) benefit significantly from using this. 
// We've measured 3-4% build speed improvements in apps as a result 
#pragma once

#ifndef ZRSOCKET_EVENT_LOOP_QUEUE_H
#define ZRSOCKET_EVENT_LOOP_QUEUE_H
#include "event_type.h"
#include "event_type_handler.h"
#include "event_type_queue.h"
#include "mutex.h"

ZRSOCKET_NAMESPACE_BEGIN

template <class TMutex, class TEventTypeHandler>
class EventLoopQueue
{
public:
    EventLoopQueue()
    {
        queue_active_  = &queue1_;
        queue_standby_ = &queue2_;
    }

    virtual ~EventLoopQueue()
    {
    }

    int init(uint_t queue_max_size, uint_t event_type_len = 64)
    {
        queue1_.init(queue_max_size, event_type_len);
        queue2_.init(queue_max_size, event_type_len);
        return 0;
    }

    inline int push_event(const EventType *event)
    {
        mutex_.lock();
        int ret = queue_standby_->push(event);
        mutex_.unlock();
        return ret;
    }

    inline int loop(int times = 10000)
    {
        EventType *event;
        for (int i = 0; i<times; ++i) {
            event = queue_active_->pop();
            if (nullptr != event) {
                handler_.handle_event(event);
                if (EventTypeId::QUIT_EVENT == event->type_) {
                    break;
                }
            }
            else if (!swap_queue_ptr()){
                break;
            }
        }

        return 0;
    }

    inline uint_t capacity() const
    {
        return queue1_.capacity();
    }

private:
    //交换queue的 active/standby 指针
    inline bool swap_queue_ptr()
    {
        bool ret = false;
        mutex_.lock();
        if (!queue_standby_->empty()) {
            std::swap(queue_standby_, queue_active_);
            ret = true;
        }
        mutex_.unlock();
        return ret;
    }

private:
    //采用双队列提高性能
    EventTypeQueue *queue_active_;
    EventTypeQueue *queue_standby_;
    EventTypeQueue  queue1_;
    EventTypeQueue  queue2_;
    TMutex          mutex_;

    TEventTypeHandler handler_;
};

ZRSOCKET_NAMESPACE_END

#endif
