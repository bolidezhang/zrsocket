// Some compilers (e.g. VC++) benefit significantly from using this. 
// We've measured 3-4% build speed improvements in apps as a result 
#pragma once

#ifndef ZRSOCKET_EVENT_LOOP_GROUP_H
#define ZRSOCKET_EVENT_LOOP_GROUP_H
#include <vector>
#include "config.h"
#include "event_handler.h"
#include "event_loop.h"

ZRSOCKET_NAMESPACE_BEGIN

template <class TEventLoop>
class EventLoopGroup : public EventLoop
{
public:
    EventLoopGroup()
        : next_index_(0)
    {
    }

    virtual ~EventLoopGroup()
    {
        close();
        clear();
    }

    virtual TEventLoop * assign_event_loop()
    {
        int loops_size = (int)event_loops_.size();
        if (loops_size > 1) {
            int index   = next_index_;
            next_index_ = (index + 1) % loops_size;
            return event_loops_[index];
        }
        else {
            return event_loops_[0];
        }
    }

    int init(uint_t num = 2, uint_t max_events = 10000, int event_mode = 1, uint_t event_queue_max_size = 100000, uint_t event_type_len = 8)
    {
        if (num < 1) {
            num = 1;
        }

        event_loops_.reserve(num);
        TEventLoop *loop;
        for (uint_t i = 0; i < num; ++i) {
            loop = new TEventLoop();
            loop->init(num, max_events, event_mode, event_queue_max_size, event_type_len);
            event_loops_.emplace_back(std::move(loop));
        }
        return 0;
    }

    int clear()
    {
        for (auto &loop : event_loops_) {
            delete loop;
        }
        event_loops_.clear();
        return 0; 
    }

    int buffer_size(int recv_buffer_size = 65536, int send_buffer_size = 65536)
    { 
        for (auto &loop : event_loops_) {
            loop->buffer_size(recv_buffer_size, send_buffer_size);
        }
        return 0; 
    }

    int open(uint_t max_size = 1024, uint_t iovec_count = 1024, int64_t max_timeout_us = -1, int flags = 0)
    { 
        for (auto &loop : event_loops_) {
            loop->open(max_size, iovec_count, max_timeout_us, flags);
        }
        return 0;
    }

    int close() 
    { 
        for (auto &loop : event_loops_) {
            loop->close();
        }
        return 0; 
    }

    int add_handler(EventHandler *handler, int event_mask)
    {
        TEventLoop *loop;
        size_t size = event_loops_.size();
        for (size_t i = 0; i < size; ++i) {
            loop = assign_event_loop();
            if (nullptr != loop) {
                if (loop->add_handler(handler, event_mask) >= 0) {
                    return 0;
                }
            }
        }
        return -1;
    }

    int delete_handler(EventHandler *handler, int event_mask)
    {
        return handler->event_loop_->delete_event(handler, event_mask);
    }

    int remove_handler(EventHandler *handler, int event_mask)
    {
        return handler->event_loop_->remove_handler(handler, event_mask);
    }

    int add_event(EventHandler *handler, int event_mask)
    {
        return handler->event_loop_->add_event(handler, event_mask);
    }

    int delete_event(EventHandler *handler, int event_mask)
    {
        return handler->event_loop_->delete_event(handler, event_mask);
    }

    int set_event(EventHandler *handler, int event_mask)
    {
        return handler->event_loop_->set_event(handler, event_mask);
    }

    int add_timer(ITimer *timer)
    {
        return 0;
    }

    int delete_timer(ITimer *timer)
    {
        return 0;
    }

    int push_event(const EventType *event)
    {
        return 0;
    }

    int loop(int64_t timeout_us)
    {
        return 0;
    }

    int loop_wakeup()
    {
        for (auto &loop : event_loops_) {
            loop->loop_wakeup();
        }
        return 0;
    }

    int loop_thread_start(int64_t timeout_us = -1)
    {
        for (auto &loop : event_loops_) {
            loop->loop_thread_start(timeout_us);
        }
        return 0;
    }

    int loop_thread_join()
    { 
        for (auto &loop : event_loops_) {
            loop->loop_thread_join();
        }
        return 0;
    }

    int loop_thread_stop()
    { 
        for (auto &loop : event_loops_) {
            loop->loop_thread_stop();
        }
        return 0;
    }

    size_t handler_size()
    {
        size_t handler_size = 0;
        for (auto &loop : event_loops_) {
            handler_size += loop->handler_size();
        }
        return handler_size;
    }

protected:
    std::vector<TEventLoop *> event_loops_;
    int next_index_;
};

ZRSOCKET_NAMESPACE_END

#endif
