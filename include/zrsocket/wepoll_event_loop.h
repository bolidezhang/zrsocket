#pragma once

#ifndef ZRSOCKET_WEPOLL_EVENT_LOOP_H
#define ZRSOCKET_WEPOLL_EVENT_LOOP_H

#include <algorithm>
#include "config.h"
#include "byte_buffer.h"
#include "event_loop.h"
#include "mutex.h"
#include "thread.h"
#include "time.h"
#include "timer_queue.h"
#include "notify_handler.h"
#include "event_loop_queue.h"

#ifdef ZRSOCKET_OS_WINDOWS
#include "..\dependency\wepoll\wepoll.h"

ZRSOCKET_NAMESPACE_BEGIN

enum class WEPOLL_MODE
{
    LT = 1,
    ET = 2,
};

template <class TMutex,
    class TLoopData = nullptr_t,
    class TEventTypeHandler = EventTypeHandler,
    class TQueue = DoubleBufferEventTypeQueue<TMutex> >
    class WEpollEventLoop : public EventLoop
{
public:
    WEpollEventLoop()
    {
        buffer_size();
        timer_queue_.event_loop(this);
    }

    virtual ~WEpollEventLoop()
    {
        close();
    }

    int init(uint_t num = 1, uint_t max_events = 10000, int event_mode = WEPOLL_MODE::LT,
        uint_t event_queue_max_size = 100000, uint_t event_type_len = 8)
    {
        max_events_ = max_events;
        return event_queue_.init(event_queue_max_size, event_type_len);
    }

    int buffer_size(int recv_buffer_size = 65536, int send_buffer_size = 65536)
    {
        recv_buffer_.reserve(recv_buffer_size);
        send_buffer_.reserve(send_buffer_size);
        return 0;
    }

    inline ByteBuffer* get_recv_buffer()
    {
        return &recv_buffer_;
    }

    inline ByteBuffer* get_send_buffer()
    {
        return &send_buffer_;
    }

    inline ZRSOCKET_IOVEC* iovecs(int& iovecs_count)
    {
        iovecs_count = iovecs_count_;
        return iovecs_;
    }

    inline void* get_loop_data()
    {
        return &loop_data_;
    }

    int open(uint_t max_size = 100000, int iovecs_count = 1024, int64_t max_timeout_us = -1, int flags = 0)
    {
        if (max_size <= 0) {
            return -1;
        }
        if (iovecs_count <= 0) {
            return -2;
        }
        if (iovecs_count > ZRSOCKET_MAX_IOVCNT) {
            iovecs_count = ZRSOCKET_MAX_IOVCNT;
        }
        if (max_events_ > max_size) {
            max_events_ = max_size;
        }

        max_size_ = max_size;
        max_timeout_us_ = max_timeout_us;
        iovecs_count_ = iovecs_count;

        events_ = new epoll_event[max_events_];
        if (nullptr == events_) {
            return -3;
        }
        iovecs_ = new ZRSOCKET_IOVEC[iovecs_count_];
        if (nullptr == iovecs_) {
            return -4;
        }
        epoll_handle_ = epoll_create(max_size);
        if (nullptr == epoll_handle_) {
            return -5;
        }

        return 0;
    }

    int close()
    {
        thread_.stop();
        thread_.join();
        current_handle_size_ = 0;
        if (nullptr != events_) {
            delete[]events_;
            events_ = nullptr;
        }
        if (nullptr != iovecs_) {
            delete[]iovecs_;
            iovecs_ = nullptr;
        }
        if (epoll_handle_) {
            epoll_close(epoll_handle_);
            epoll_handle_ = nullptr;
        }
        return 0;
    }

    int add_handler(EventHandler *handler, int event_mask)
    {
        mutex_.lock();
        if (handler->in_event_loop_) {
            mutex_.unlock();
            return -1;
        }

        struct epoll_event ee = { 0 };
        ee.data.ptr = handler;
        int add_event_mask = EventHandler::NULL_EVENT_MASK;
        if (event_mask & EventHandler::READ_EVENT_MASK) {
            ee.events |= EPOLLIN;
            add_event_mask |= EventHandler::READ_EVENT_MASK;
        }
        if (event_mask & EventHandler::WRITE_EVENT_MASK) {
            ee.events |= EPOLLOUT;
            add_event_mask |= EventHandler::WRITE_EVENT_MASK;
        }

        if (epoll_ctl(epoll_handle_, EPOLL_CTL_ADD, handler->fd_, &ee) < 0) {
            mutex_.unlock();
            return -2;
        }

        ++current_handle_size_;
        handler->in_event_loop_ = true;
        handler->event_loop_ = this;
        handler->event_mask_ = add_event_mask;
        mutex_.unlock();

        return 0;
    }

    int delete_handler(EventHandler *handler, int event_mask)
    {
        mutex_.lock();
        if (!handler->in_event_loop_) {
            mutex_.unlock();
            return -1;
        }

        if (epoll_ctl(epoll_handle_, EPOLL_CTL_DEL, handler->fd_, nullptr) >= 0) {
            handler->in_event_loop_ = false;
            handler->event_mask_ = EventHandler::NULL_EVENT_MASK;
            --current_handle_size_;
            mutex_.unlock();

            handler->handle_close();
            handler->source_->free_handler(handler);
        }
        else {
            mutex_.unlock();
            return -2;
        }

        return 0;
    }

    int remove_handler(EventHandler *handler, int event_mask)
    {
        mutex_.lock();
        if (!handler->in_event_loop_) {
            mutex_.unlock();
            return -1;
        }

        if (epoll_ctl(epoll_handle_, EPOLL_CTL_DEL, handler->fd_, nullptr) >= 0) {
            handler->in_event_loop_ = false;
            handler->event_loop_ = nullptr;
            handler->event_mask_ = EventHandler::NULL_EVENT_MASK;
            --current_handle_size_;
            mutex_.unlock();
        }
        else {
            mutex_.unlock();
            return -2;
        }

        return 0;
    }

    int add_event(EventHandler* handler, int event_mask)
    {
        if (!handler->in_event_loop_) {
            return -1;
        }

        struct epoll_event ee = { 0 };
        ee.data.ptr = handler;
        int add_event_mask = EventHandler::NULL_EVENT_MASK;
        if ((event_mask & EventHandler::READ_EVENT_MASK) &&
            !(handler->event_mask_ & EventHandler::READ_EVENT_MASK)) {
            add_event_mask |= EventHandler::READ_EVENT_MASK;
        }
        if ((event_mask & EventHandler::WRITE_EVENT_MASK) &&
            !(handler->event_mask_ & EventHandler::WRITE_EVENT_MASK)) {
            add_event_mask |= EventHandler::WRITE_EVENT_MASK;
        }
        if (add_event_mask != EventHandler::NULL_EVENT_MASK) {
            event_mask = handler->event_mask_ | add_event_mask;
            if (event_mask & EventHandler::READ_EVENT_MASK) {
                ee.events |= EPOLLIN;
            }
            if (event_mask & EventHandler::WRITE_EVENT_MASK) {
                ee.events |= EPOLLOUT;
            }
            if (epoll_ctl(epoll_handle_, EPOLL_CTL_MOD, handler->fd_, &ee) < 0) {
                return -2;
            }
            handler->event_mask_ = event_mask;
        }

        return 0;
    }

    int delete_event(EventHandler* handler, int event_mask)
    {
        if (!handler->in_event_loop_) {
            return -1;
        }

        struct epoll_event ee = { 0 };
        ee.data.ptr = handler;
        int del_event_mask = EventHandler::NULL_EVENT_MASK;
        if (handler->event_mask_ & event_mask & EventHandler::READ_EVENT_MASK) {
            del_event_mask |= EventHandler::READ_EVENT_MASK;
        }
        if (handler->event_mask_ & event_mask & EventHandler::WRITE_EVENT_MASK) {
            del_event_mask |= EventHandler::WRITE_EVENT_MASK;
        }

        if (del_event_mask != EventHandler::NULL_EVENT_MASK) {
            event_mask = handler->event_mask_ & ~del_event_mask;
            if (event_mask & EventHandler::READ_EVENT_MASK) {
                ee.events |= EPOLLIN;
            }
            if (event_mask & EventHandler::WRITE_EVENT_MASK) {
                ee.events |= EPOLLOUT;
            }
            if (epoll_ctl(epoll_handle_, EPOLL_CTL_MOD, handler->fd_, &ee) < 0) {
                return -2;
            }
            handler->event_mask_ = event_mask;
        }

        return 0;
    }

    int set_event(EventHandler* handler, int event_mask)
    {
        if (!handler->in_event_loop_) {
            return -1;
        }

        struct epoll_event ee = { 0 };
        ee.data.ptr = handler;
        int set_event_mask = EventHandler::NULL_EVENT_MASK;
        if (event_mask & EventHandler::READ_EVENT_MASK) {
            ee.events |= EPOLLIN;
            set_event_mask |= EventHandler::READ_EVENT_MASK;
        }
        if (event_mask & EventHandler::WRITE_EVENT_MASK) {
            ee.events |= EPOLLOUT;
            set_event_mask |= EventHandler::WRITE_EVENT_MASK;
        }

        if (set_event_mask != EventHandler::NULL_EVENT_MASK) {
            if (epoll_ctl(epoll_handle_, EPOLL_CTL_MOD, handler->fd_, &ee) < 0) {
                return -2;
            }
            handler->event_mask_ = set_event_mask;
        }

        return 0;
    }

    int add_timer(ITimer* timer)
    {
        if (timer_queue_.add_timer(timer, Time::instance().current_timestamp_us())) {
            loop_wakeup();
            return 1;
        }
        return 0;
    }

    int delete_timer(ITimer* timer)
    {
        return timer_queue_.delete_timer(timer);
    }

    int push_event(const EventType* event)
    {
        if (event_queue_.push_event(event) > 0) {
            loop_wakeup();
            return 1;
        }
        return 0;
    }

    int loop(int64_t timeout_us = -1)
    {
        int64_t min_interval = timer_queue_.min_interval();
        if (min_interval > 0) {
            timeout_us = std::min<int64_t>(min_interval, timeout_us);
        }

        int timeout_ms = (timeout_us >= 0) ? (timeout_us / 1000) : (-1);
        int ready = epoll_wait(epoll_handle_, events_, max_events_, timeout_ms);
        if (ready > 0) {
            EventHandler *handler;
            EventSource *source;
            int events;
            for (int i = 0; i < ready; ++i) {
                handler = static_cast<EventHandler *>(events_[i].data.ptr);
                events = events_[i].events;
                if (events & EPOLLIN) {
                    if (handler->handle_read() < 0) {
                        delete_handler(handler, 0);
                        continue;
                    }
                }
                if (events & EPOLLOUT) {
                    source = handler->source_;
                    if (source->source_state() != EventSource::STATE_CONNECTING) {
                        if (handler->handle_write() < 0) {
                            delete_handler(handler, 0);
                        }
                    }
                    else {
                        //nonblock connect: tcpclient connect success
                        set_event(handler, EventHandler::READ_EVENT_MASK);
                        source->source_state(EventSource::STATE_CONNECTED);
                        handler->state_ = EventHandler::STATE_CONNECTED;
                        handler->handle_connect();
                    }
                }
            }
        }
        event_queue_.loop(event_queue_.capacity());
        timer_queue_.loop(Time::instance().current_timestamp_us());

        return ready;
    }

    int loop_wakeup()
    {
        return 0;
    }

    int loop_thread_start(int64_t timeout_us = -1)
    {
        max_timeout_us_ = timeout_us;
        return thread_.start(loop_thread_proc, this);
    }

    int loop_thread_join()
    {
        thread_.join();
        return 0;
    }

    int loop_thread_stop()
    {
        thread_.stop();
        return 0;
    }

    inline uint_t handler_size()
    {
        return current_handle_size_;
    }

private:
    static int loop_thread_proc(void* arg)
    {
        WEpollEventLoop<TMutex, TLoopData, TEventTypeHandler, TQueue>* event_loop =
            static_cast<WEpollEventLoop<TMutex, TLoopData, TEventTypeHandler, TQueue>*>(arg);

        Thread& thread = event_loop->thread_;
        while (thread.state() == Thread::State::RUNNING) {
            event_loop->loop(event_loop->max_timeout_us_);
        }
        return 0;
    }

private:
    uint_t  max_size_ = 100000;
    uint_t  max_events_ = 1000;
    uint_t  current_handle_size_ = 0;
    int64_t max_timeout_us_ = -1;

    HANDLE epoll_handle_ = nullptr;
    struct epoll_event *events_ = nullptr;

    ZRSOCKET_IOVEC* iovecs_ = nullptr;
    int             iovecs_count_ = 0;

    ByteBuffer          recv_buffer_;
    ByteBuffer          send_buffer_;
    TimerQueue<TMutex>  timer_queue_;
    Thread              thread_;
    TMutex              mutex_;

    EventLoopQueue<TQueue, EventTypeHandler> event_queue_;
    TLoopData loop_data_;
};

ZRSOCKET_NAMESPACE_END

#endif
#endif
