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

template <class TQueue, class TEventTypeHandler>
class EventLoopQueue
{
public:
    EventLoopQueue()
    {
    }

    virtual ~EventLoopQueue()
    {
    }

    int init(uint_t queue_max_size, uint16_t event_type_len = 8)
    {
        queue_.init(queue_max_size, event_type_len);
        return 0;
    }

    inline int push_event(const EventType *event)
    {
        return queue_.push(event);
    }

    inline int loop(int times = 10000)
    {
        EventType *event;
        for (int i = 0; i<times; ++i) {
            event = queue_.pop();
            if (nullptr != event) {
                handler_.handle_event(event);
                if (EventTypeId::QUIT_EVENT == event->type()) {
                    break;
                }
            }
            else {
                if (!queue_.swap_buffer()) {
                    break;
                }
            }
        }

        return 0;
    }

    inline uint_t capacity() const
    {
        return static_cast<uint_t>(queue_.capacity());
    }

private:
    TQueue queue_;
    TEventTypeHandler handler_;
};

ZRSOCKET_NAMESPACE_END

#endif
