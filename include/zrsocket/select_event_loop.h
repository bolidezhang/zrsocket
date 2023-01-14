// Some compilers (e.g. VC++) benefit significantly from using this. 
// We've measured 3-4% build speed improvements in apps as a result 
#pragma once

#ifndef ZRSOCKET_SELECT_EVENT_LOOP_H
#define ZRSOCKET_SELECT_EVENT_LOOP_H
#include <algorithm>
#include <unordered_map>
#include <unordered_set>
#include "config.h"
#include "base_type.h"
#include "memory.h"
#include "byte_buffer.h"
#include "mutex.h"
#include "thread.h"
#include "os_api.h"
#include "event_loop.h"
#include "time.h"
#include "timer_queue.h"
#include "event_loop_queue.h"
#include "atomic.h"

#ifndef ZRSOCKET_OS_WINDOWS
    #include <sys/select.h>
#endif

ZRSOCKET_NAMESPACE_BEGIN

template <class TMutex, class TLoopData = nullptr_t, class TEventTypeHandler = EventTypeHandler>
class SelectEventLoop : public EventLoop
{
public:
    SelectEventLoop()
    {
        temp_handlers_active_   = &temp1_handlers_;
        temp_handlers_standby_  = &temp2_handlers_;
        buffer_size();
        timer_queue_.event_loop(this);
#ifndef ZRSOCKET_OS_WINDOWS
        wakeup_handler_.open();
        wakeup_flag_.store(true, std::memory_order_relaxed);
#endif
    }

    virtual ~SelectEventLoop()
    {
        close();
    }

    int init(uint_t num = 1, uint_t max_events = 10000, int event_mode = 0, uint_t event_queue_max_size = 100000, uint_t event_type_len = 8)
    {
        return event_queue_.init(event_queue_max_size, event_type_len);
    }

    int buffer_size(int recv_buffer_size = 65536, int send_buffer_size = 65536)
    {
        recv_buffer_.reserve(recv_buffer_size);
        send_buffer_.reserve(send_buffer_size);
        return 0;
    }

    inline ByteBuffer * get_recv_buffer()
    {
        return &recv_buffer_;
    }

    inline ByteBuffer * get_send_buffer()
    {
        return &send_buffer_;
    }

    inline ZRSOCKET_IOVEC * iovecs(int &iovecs_count)
    {
        iovecs_count = iovecs_count_;
        return iovecs_;
    }

    inline void * get_loop_data()
    {
        return &loop_data_;
    }

    int open(uint_t max_size = 1024, int iovecs_count = 1024, int64_t max_timeout_us = -1, int flags = 0)
    {
        if (max_size <= 0) {
            return -1;
        }
        if (iovecs_count <= 0) {
            return -2;
        }

        if (max_size > FD_SETSIZE) {
            max_size = FD_SETSIZE;
        }
        if (iovecs_count > ZRSOCKET_MAX_IOVCNT) {
            iovecs_count = ZRSOCKET_MAX_IOVCNT;
        }
        max_size_       = max_size;
        max_timeout_us_ = max_timeout_us;
        iovecs_count_   = iovecs_count;
        iovecs_         = new ZRSOCKET_IOVEC[iovecs_count];

        temp1_handlers_.reserve(max_size_);
        temp2_handlers_.reserve(max_size_);
        handlers_.reserve(max_size_);
        temp1_handlers_.clear();
        temp2_handlers_.clear();
        handlers_.clear();

        FD_ZERO(&fd_set_in_.read);
        FD_ZERO(&fd_set_in_.write);
        FD_ZERO(&fd_set_out_.read);
        FD_ZERO(&fd_set_out_.write);
#ifdef ZRSOCKET_OS_WINDOWS
        FD_ZERO(&fd_set_in_.except);
        FD_ZERO(&fd_set_out_.except);
#else
        add_handler(&wakeup_handler_, EventHandler::READ_EVENT_MASK);
#endif
        return 0;
    }

    int close()
    {
        thread_.stop();
        thread_.join();
        temp1_handlers_.clear();
        temp2_handlers_.clear();
        handlers_.clear();
        if (nullptr != iovecs_) {
            delete []iovecs_;
            iovecs_ = nullptr;
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

        if ( (read_size_ >= max_size_) && (event_mask & EventHandler::READ_EVENT_MASK) ){
            mutex_.unlock();
            return -2;
        }
        if ((write_size_ >= max_size_) && (event_mask & EventHandler::WRITE_EVENT_MASK)) {
            mutex_.unlock();
            return -3;
        }
#ifdef ZRSOCKET_OS_WINDOWS
        if ((except_size_ >= max_size_) && (event_mask & EventHandler::EXCEPT_EVENT_MASK)) {
            mutex_.unlock();
            return -4;
        }
#endif

        int add_event_mask = EventHandler::NULL_EVENT_MASK;
        if (event_mask & EventHandler::READ_EVENT_MASK) {
            FD_SET(handler->fd_, &fd_set_in_.read);
            add_event_mask |= EventHandler::READ_EVENT_MASK;
            ++read_size_;
        }
        if (event_mask & EventHandler::WRITE_EVENT_MASK) {
            FD_SET(handler->fd_, &fd_set_in_.write);
            add_event_mask |= EventHandler::WRITE_EVENT_MASK;
            ++write_size_;
        }
#ifdef ZRSOCKET_OS_WINDOWS
        if (event_mask & EventHandler::EXCEPT_EVENT_MASK) {
            FD_SET(handler->fd_, &fd_set_in_.except);
            add_event_mask |= EventHandler::EXCEPT_EVENT_MASK;
            ++except_size_;
        }
#else
        if (max_fd_ < handler->fd_) {
            max_fd_ = handler->fd_;
        }
#endif
        handler->in_event_loop_ = true;
        handler->event_loop_    = this;
        handler->event_mask_    = add_event_mask;
        temp_handlers_standby_->emplace(handler, ADD_HANDLER);
        mutex_.unlock();

        loop_wakeup();
        return 0;
    }

    int delete_handler(EventHandler *handler, int event_mask)
    {
        mutex_.lock();
        if (!handler->in_event_loop_) {
            mutex_.unlock();
            return -1;
        }

        event_mask = handler->event_mask_;
        if (event_mask & EventHandler::READ_EVENT_MASK) {
            FD_CLR(handler->fd_, &fd_set_in_.read);
            --read_size_;
        }
        if (event_mask & EventHandler::WRITE_EVENT_MASK) {
            FD_CLR(handler->fd_, &fd_set_in_.write);
            --write_size_;
        }
#ifdef ZRSOCKET_OS_WINDOWS
        if (event_mask & EventHandler::EXCEPT_EVENT_MASK) {
            FD_CLR(handler->fd_, &fd_set_in_.except);
            --except_size_;
        }
#else
        if (max_fd_ == handler->fd_) {
            --max_fd_;
        }
#endif
        handler->in_event_loop_ = false;
        handler->event_loop_    = nullptr;
        handler->event_mask_    = EventHandler::NULL_EVENT_MASK;
        temp_handlers_standby_->emplace(handler, DEL_HANDLER);
        mutex_.unlock();

        loop_wakeup();
        return 0;
    }

    int remove_handler(EventHandler *handler, int event_mask)
    {
        mutex_.lock();
        if (!handler->in_event_loop_) {
            mutex_.unlock();
            return -1;
        }

        event_mask = handler->event_mask_;
        if (event_mask & EventHandler::READ_EVENT_MASK) {
            FD_CLR(handler->fd_, &fd_set_in_.read);
            --read_size_;
        }
        if (event_mask & EventHandler::WRITE_EVENT_MASK) {
            FD_CLR(handler->fd_, &fd_set_in_.write);
            --write_size_;
        }
#ifdef ZRSOCKET_OS_WINDOWS
        if (event_mask & EventHandler::EXCEPT_EVENT_MASK) {
            FD_CLR(handler->fd_, &fd_set_in_.except);
            --except_size_;
        }
#else
        if (max_fd_ == handler->fd_) {
            --max_fd_;
        }
#endif
        handler->in_event_loop_ = false;
        handler->event_loop_    = nullptr;
        handler->event_mask_    = EventHandler::NULL_EVENT_MASK;
        temp_handlers_standby_->emplace(handler, REMOVE_HANDLER);
        mutex_.unlock();

        return 0;
    }

    int add_event(EventHandler *handler, int event_mask)
    {
        mutex_.lock();
        if (!handler->in_event_loop_) {
            mutex_.unlock();
            return -1;
        }

        int add_event_mask = EventHandler::NULL_EVENT_MASK;
        if ( (event_mask & EventHandler::READ_EVENT_MASK) && 
             !(handler->event_mask_ & EventHandler::READ_EVENT_MASK) ) {
            FD_SET(handler->fd_, &fd_set_in_.read);
            add_event_mask |= EventHandler::READ_EVENT_MASK;
            ++read_size_;
        }
        if ( (event_mask & EventHandler::WRITE_EVENT_MASK) && 
            !(handler->event_mask_ & EventHandler::WRITE_EVENT_MASK) ) {
            FD_SET(handler->fd_, &fd_set_in_.write);
            add_event_mask |= EventHandler::WRITE_EVENT_MASK;
            ++write_size_;
        }
#ifdef ZRSOCKET_OS_WINDOWS
        if ( (event_mask & EventHandler::EXCEPT_EVENT_MASK) &&
             !(handler->event_mask_ & EventHandler::EXCEPT_EVENT_MASK) ) {
            FD_SET(handler->fd_, &fd_set_in_.except);
            add_event_mask |= EventHandler::EXCEPT_EVENT_MASK;
            ++except_size_;
        }
#endif
        handler->event_mask_ |= add_event_mask;
        mutex_.unlock();

        return 0;
    }

    int delete_event(EventHandler *handler, int event_mask)
    {
        mutex_.lock();
        if (!handler->in_event_loop_) {
            mutex_.unlock();
            return -1;
        }

        int del_event_mask = EventHandler::NULL_EVENT_MASK;
        if (handler->event_mask_ & event_mask & EventHandler::READ_EVENT_MASK) {
            FD_CLR(handler->fd_, &fd_set_in_.read);
            del_event_mask |= EventHandler::READ_EVENT_MASK;
            --read_size_;
        }
        if (handler->event_mask_ & event_mask & EventHandler::WRITE_EVENT_MASK) {
            FD_CLR(handler->fd_, &fd_set_in_.write);
            del_event_mask |= EventHandler::WRITE_EVENT_MASK;
            --write_size_;
        }
#ifdef ZRSOCKET_OS_WINDOWS
        if (handler->event_mask_ & event_mask & EventHandler::EXCEPT_EVENT_MASK) {
            FD_CLR(handler->fd_, &fd_set_in_.except);
            del_event_mask |= EventHandler::EXCEPT_EVENT_MASK;
            --except_size_;
        }
#endif
        handler->event_mask_ &= ~del_event_mask;
        mutex_.unlock();

        return 0;
    }

    int set_event(EventHandler *handler, int event_mask)
    {
        mutex_.lock();
        if (!handler->in_event_loop_) {
            mutex_.unlock();
            return -1;
        }

        int set_event_mask = EventHandler::NULL_EVENT_MASK;
        if (event_mask & EventHandler::READ_EVENT_MASK) {
            if (!(handler->event_mask_ & EventHandler::READ_EVENT_MASK)) {
                FD_SET(handler->fd_, &fd_set_in_.read);
                set_event_mask |= EventHandler::READ_EVENT_MASK;
                ++read_size_;
            }
        }
        else {
            if (handler->event_mask_ & EventHandler::READ_EVENT_MASK) {
                FD_CLR(handler->fd_, &fd_set_in_.read);
                --read_size_;
            }
        }

        if (event_mask & EventHandler::WRITE_EVENT_MASK) {
            if (!(handler->event_mask_ & EventHandler::WRITE_EVENT_MASK)) {
                FD_SET(handler->fd_, &fd_set_in_.write);
                set_event_mask |= EventHandler::WRITE_EVENT_MASK;
                ++write_size_;
            }
        }
        else {
            if (handler->event_mask_ & EventHandler::WRITE_EVENT_MASK) {
                FD_CLR(handler->fd_, &fd_set_in_.write);
                --write_size_;
            }
        }

#ifdef ZRSOCKET_OS_WINDOWS
        if (event_mask & EventHandler::EXCEPT_EVENT_MASK) {
            if (!(handler->event_mask_ & EventHandler::EXCEPT_EVENT_MASK)) {
                FD_SET(handler->fd_, &fd_set_in_.except);
                set_event_mask |= EventHandler::EXCEPT_EVENT_MASK;
                ++except_size_;
            }
        }
        else {
            if (handler->event_mask_ & EventHandler::EXCEPT_EVENT_MASK) {
                FD_CLR(handler->fd_, &fd_set_in_.except);
                --except_size_;
            }
        }
#endif
        handler->event_mask_ = set_event_mask;
        mutex_.unlock();

        return 0;
    }

    int add_timer(ITimer *timer)
    {
        if (timer_queue_.add_timer(timer, Time::instance().current_timestamp_us())) {
            loop_wakeup();
            return 1;
        }
        return 0;
    }

    int delete_timer(ITimer *timer)
    {
        return timer_queue_.delete_timer(timer);
    }

    int push_event(const EventType *event)
    {
        if (event_queue_.push_event(event) > 0) {
            loop_wakeup();
            return 1;
        }
        return 0;
    }

    int loop(int64_t timeout_us = -1)
    {
        EventHandler *handler;
        int ready;
#ifdef ZRSOCKET_OS_WINDOWS
        bool have_event = false;
#endif

        //交换active/standby指针
        mutex_.lock();
        zrsocket_memcpy(&fd_set_out_.read, &fd_set_in_.read, sizeof(fd_set));
        zrsocket_memcpy(&fd_set_out_.write, &fd_set_in_.write, sizeof(fd_set));
#ifdef ZRSOCKET_OS_WINDOWS
        zrsocket_memcpy(&fd_set_out_.except, &fd_set_in_.except, sizeof(fd_set));
        if (read_size_ || write_size_ || except_size_) {
            have_event = true;
        }
#endif

        if (!temp_handlers_standby_->empty()) {
            std::swap(temp_handlers_standby_, temp_handlers_active_);
            mutex_.unlock();

            for (auto &iter : *temp_handlers_active_) {
                handler = iter.first;
                switch (iter.second) {
                case ADD_HANDLER:
                    handlers_.emplace(std::move(handler));
                    break;
                case DEL_HANDLER:
                    handlers_.erase(handler);
                    handler->handle_close();
                    handler->source_->free_handler(handler);
                    break;
                case REMOVE_HANDLER:
                    handlers_.erase(handler);
                    break;
                default:
                    break;
                }
            }
            temp_handlers_active_->clear();
        }
        else {
            mutex_.unlock();
        }

        int64_t min_interval = timer_queue_.min_interval();
        if (min_interval > 0) {
            timeout_us = std::min<int64_t>(min_interval, timeout_us);
        }

#ifdef ZRSOCKET_OS_WINDOWS
        if (timeout_us < ZRSOCKET_TIMER_MIN_INTERVAL) {
            timeout_us = ZRSOCKET_TIMER_MIN_INTERVAL;
        }
        if (have_event) {
            timeval tv;
            tv.tv_sec  = (long)(timeout_us / 1000000LL);
            tv.tv_usec = (long)(timeout_us - tv.tv_sec * 1000000LL);
            ready = select(0, &fd_set_out_.read, &fd_set_out_.write, &fd_set_out_.except, &tv);
        }
        else {
            OSApi::sleep_us(timeout_us);
            event_queue_.loop(event_queue_.capacity());
            timer_queue_.loop(Time::instance().current_timestamp_us());
            return 0;
        }
#else
        timeval tv, *tvp;
        if (timeout_us < 0) {
            tvp = nullptr;
        }
        else {
            tv.tv_sec = timeout_us / 1000000LL;
            tv.tv_usec = timeout_us - tv.tv_sec * 1000000LL;
            tvp = &tv;
        }
        ready = select(max_fd_+1, &fd_set_out_.read, &fd_set_out_.write, nullptr, tvp);
        wakeup_flag_.store(false, std::memory_order_relaxed);
#endif

        int num_events = 0;
        if (ready > 0) {
            num_events = ready;
            EventSource *source;
            for (auto &handler : handlers_) {
                if (FD_ISSET(handler->fd_, &fd_set_out_.read)) {
                    if (handler->handle_read() < 0) {
                        delete_handler(handler, 0);

                        if (--ready <= 0) {
                            break;
                        }
                        continue;
                    }

                    if (--ready <= 0) {
                        break;
                    }
                }

                if (FD_ISSET(handler->fd_, &fd_set_out_.write)) {
                    source = handler->source_;
                    if (source->source_state() != EventSource::STATE_CONNECTING) {
                        if (handler->handle_write() < 0) {
                            delete_handler(handler, 0);
                            if (--ready <= 0) {
                                break;
                            }
                            continue;
                        }
                    }
                    else {
                        //nonblock connect: tcpclient connect success
                        set_event(handler, EventHandler::READ_EVENT_MASK);
                        source->source_state(EventSource::STATE_CONNECTED);
                        handler->state_ = EventHandler::STATE_CONNECTED;
                        handler->handle_connect();
                    }

                    if (--ready <= 0) {
                        break;
                    }
                }

#ifdef ZRSOCKET_OS_WINDOWS
                if (FD_ISSET(handler->fd_, &fd_set_out_.except)) {
                    delete_handler(handler, 0);
                    if (--ready <= 0) {
                        break;
                    }
                }
#endif
            }
        }

        event_queue_.loop(event_queue_.capacity());
        timer_queue_.loop(Time::instance().current_timestamp_us());
#ifndef ZRSOCKET_OS_WINDOWS
        wakeup_flag_.store(true, std::memory_order_relaxed);
#endif

        return num_events;
    }

    int loop_wakeup()
    {
#ifndef ZRSOCKET_OS_WINDOWS
        if (wakeup_flag_.exchange(false, std::memory_order_relaxed)) {
            return wakeup_handler_.notify();
        }
#endif
        return 0;
    }

    int loop_thread_start(int64_t timeout_us = -1)
    {
        max_timeout_us_ = timeout_us;
        return thread_.start(loop_thread_proc, this);
    }

    int loop_thread_join()
    {
        return thread_.join();
    }

    int loop_thread_stop()
    {
        return thread_.stop();
    }

    size_t handler_size()
    {
        mutex_.lock();
        size_t handlers_size = std::max<uint_t>(read_size_, write_size_);
#ifdef ZRSOCKET_OS_WINDOWS
        handlers_size = std::max<uint_t>(handlers_size, except_size_);
#endif
        mutex_.unlock();
        return handlers_size;
    }

private:
    static int loop_thread_proc(void *arg)
    {
        SelectEventLoop<TMutex,TLoopData, TEventTypeHandler> *event_loop = static_cast<SelectEventLoop<TMutex, TLoopData, TEventTypeHandler> *>(arg);
        Thread &thread = event_loop->thread_;
        while (thread.state() == Thread::State::RUNNING) {
            event_loop->loop(event_loop->max_timeout_us_);
        }
        return 0;
    }

    //set<handler *>
    typedef std::unordered_set<EventHandler *> EVENT_HANDLES;

    //map<handler *, ADD_HANDLER/DELTE_HANDLER/REMOVE_HANDLER>
    typedef std::unordered_map<EventHandler *, int> TEMP_EVENT_HANDLES;

    enum OperateCode
    {
        ADD_HANDLER = 1,
        DEL_HANDLER = 2,
        REMOVE_HANDLER = 3,
    };

    struct ZRSOCKET_FD_SET
    {
        fd_set  read;
        fd_set  write;
#ifdef ZRSOCKET_OS_WINDOWS
        fd_set  except;
#endif
    };

    uint_t  max_size_   = 0;
    uint_t  read_size_  = 0;
    uint_t  write_size_ = 0;
#ifdef ZRSOCKET_OS_WINDOWS
    uint_t  except_size_ = 0;
#else 
    int     max_fd_ = 0;
#endif
    int64_t max_timeout_us_ = -1;

    ZRSOCKET_FD_SET     fd_set_in_;
    ZRSOCKET_FD_SET     fd_set_out_;

    EVENT_HANDLES       handlers_;
    TEMP_EVENT_HANDLES  temp1_handlers_;
    TEMP_EVENT_HANDLES  temp2_handlers_;
    TEMP_EVENT_HANDLES *temp_handlers_active_;
    TEMP_EVENT_HANDLES *temp_handlers_standby_;

    ByteBuffer          recv_buffer_;
    ByteBuffer          send_buffer_;
    ZRSOCKET_IOVEC     *iovecs_ = nullptr;
    int                 iovecs_count_ = 0;

    TMutex              mutex_;
    TimerQueue<TMutex>  timer_queue_;
    Thread              thread_;

#ifndef ZRSOCKET_OS_WINDOWS
    NotifyHandler       wakeup_handler_;
    AtomicBool          wakeup_flag_;
#endif

    EventLoopQueue<TMutex, EventTypeHandler> event_queue_;
    TLoopData loop_data_;
};

ZRSOCKET_NAMESPACE_END

#endif
