#ifndef ZRSOCKET_EPOLL_REACTOR_H_
#define ZRSOCKET_EPOLL_REACTOR_H_
#include "config.h"
#include "byte_buffer.h"
#include "event_reactor.h"
#include "mutex.h"
#include "thread.h"

#ifdef ZRSOCKET_OS_LINUX
#include <sys/epoll.h>

ZRSOCKET_BEGIN

template <class TMutex>
class EpollReactor : public EventReactor
{
public:
    EpollReactor()
        : max_events_(10000)
        , current_handle_size_(0)
        , epoll_mode_(LT_MODE)
        , events_(NULL)
        , iovec_(NULL)
        , iovec_count_(0)
    {
        set_buffer_size();
    }

    virtual ~EpollReactor()
    {
        close();
    }

    virtual int init(uint_t num = 1, uint_t max_events = 10000, int event_mode = LT_MODE)
    {
        max_events_ = max_events;
        epoll_mode_ = event_mode;
        return 0;
    }

    inline int set_buffer_size(int recv_buffer_size = 32768, int send_buffer_size = 32768)
    {
        recv_buffer_.reserve(recv_buffer_size);
        return 0;
    }

    inline int get_recv_buffer_size() const
    {
        return recv_buffer_.buffer_size();
    }

    inline char* get_recv_buffer()
    {
        return recv_buffer_.buffer();
    }

    inline ZRSOCKET_IOVEC* get_iovec()
    {
        return iovec_;
    }

    inline int get_iovec_count()
    {
        return iovec_count_;
    }

    int open(uint_t max_size = 100000, int iovec_count = 1024, int max_timeout_ms = 10, int flags = 0)
    {
        if (max_size <= 0) {
            return -1;
        }
        if (iovec_count <= 0) {
            return -2;
        }
        if (iovec_count > ZRSOCKET_MAX_IOVCNT) {
            iovec_count = ZRSOCKET_MAX_IOVCNT;
        }
        if (max_events_ > max_size) {
            max_events_ = max_size;
        }

        max_size_ = max_size;
        max_timeout_ms_ = max_timeout_ms;
        iovec_count_ = iovec_count;

        events_ = new epoll_event[max_events_];
        if (NULL == events_) {
            return -3;
        }
        iovec_ = new ZRSOCKET_IOVEC[iovec_count_];
        if (NULL == iovec_) {
            return -4;
        }
        epoll_fd_ = epoll_create(max_size);
        if (epoll_fd_ < 0) {
            return -5;
        }

        return 0;
    }

    int close()
    {
        thread_.stop();
        thread_.join();
        current_handle_size_ = 0;
        if (NULL != events_) {
            delete[]events_;
            events_ = NULL;
        }
        if (NULL != iovec_) {
            delete[]iovec_;
            iovec_ = NULL;
        }
        ::close(epoll_fd_);
        return 0;
    }

    inline int add_handler(EventHandler *handler, int event_mask)
    {
        mutex_.lock();
        if (handler->in_reactor_) {
            mutex_.unlock();
            return -1;
        }

        struct epoll_event ee = { 0 };
        ee.data.ptr = handler;
        if (epoll_mode_ == ET_MODE) {
            ee.events = EPOLLET;
        }
        int add_event_mask = EventHandler::NULL_EVENT_MASK;
        if (event_mask & EventHandler::READ_EVENT_MASK) {
            ee.events |= EPOLLIN;
            add_event_mask |= EventHandler::READ_EVENT_MASK;
        }
        if (event_mask & EventHandler::WRITE_EVENT_MASK) {
            ee.events |= EPOLLOUT;
            add_event_mask |= EventHandler::WRITE_EVENT_MASK;
        }
        if (epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, handler->socket_, &ee) < 0) {
            mutex_.unlock();
            return -1;
        }

        ++current_handle_size_;
        handler->in_reactor_ = true;
        handler->event_mask_ = add_event_mask;
        mutex_.unlock();

        return 0;
    }

    inline int del_handler(EventHandler *handler)
    {
        mutex_.lock();
        if (!handler->in_reactor_) {
            mutex_.unlock();
            return -1;
        }

        if (epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, handler->socket_, NULL) >= 0) {
            handler->in_reactor_ = false;
            handler->event_mask_ = EventHandler::NULL_EVENT_MASK;
            --current_handle_size_;
            mutex_.unlock();

            handler->handle_close();
            handler->source_->free_handler(handler);
        }
        else {
            mutex_.unlock();
            return -1;
        }

        return 0;
    }

    int add_event(EventHandler *handler, int event_mask)
    {
        if (!handler->in_reactor_) {
            return -1;
        }

        struct epoll_event ee = { 0 };
        ee.data.ptr = handler;
        if (epoll_mode_ == ET_MODE) {
            ee.events = EPOLLET;
        }

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
            if (epoll_ctl(epoll_fd_, EPOLL_CTL_MOD, handler->socket_, &ee) < 0) {
                return -1;
            }
            handler->event_mask_ = event_mask;
        }

        return 0;
    }

    int del_event(EventHandler *handler, int event_mask)
    {
        if (!handler->in_reactor_) {
            return -1;
        }

        struct epoll_event ee = { 0 };
        ee.data.ptr = handler;
        if (epoll_mode_ == ET_MODE) {
            ee.events = EPOLLET;
        }

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
            if (epoll_ctl(epoll_fd_, EPOLL_CTL_MOD, handler->socket_, &ee) < 0) {
                return -1;
            }
            handler->event_mask_ = event_mask;
        }

        return 0;
    }

    int set_event(EventHandler *handler, int event_mask)
    {
        if (!handler->in_reactor_) {
            return -1;
        }

        struct epoll_event ee = { 0 };
        ee.data.ptr = handler;
        if (epoll_mode_ == ET_MODE) {
            ee.events = EPOLLET;
        }

        int set_event_mask = EventHandler::NULL_EVENT_MASK;
        if (event_mask & EventHandler::READ_EVENT_MASK) {
            set_event_mask |= EventHandler::READ_EVENT_MASK;
        }
        if (event_mask & EventHandler::WRITE_EVENT_MASK) {
            set_event_mask |= EventHandler::WRITE_EVENT_MASK;
        }

        if (set_event_mask != EventHandler::NULL_EVENT_MASK) {
            if (set_event_mask & EventHandler::READ_EVENT_MASK) {
                ee.events |= EPOLLIN;
            }
            if (set_event_mask & EventHandler::WRITE_EVENT_MASK) {
                ee.events |= EPOLLOUT;
            }
            if (epoll_ctl(epoll_fd_, EPOLL_CTL_MOD, handler->socket_, &ee) < 0) {
                return -1;
            }
            handler->event_mask_ = set_event_mask;
        }

        return 0;
    }

    inline size_t get_handler_size() const
    {
        return current_handle_size_;
    }

    inline int event_loop(int timeout_ms = 1)
    {
        int ret = epoll_wait(epoll_fd_, events_, max_events_, timeout_ms);
        if (ret > 0) {
            EventHandler *handler;
            EventSource  *source;
            for (int i = 0; i < ret; ++i) {
                handler = (EventHandler *)events_[i].data.ptr;
                if (handler) {
                    if (events_[i].events & EPOLLIN) {
                        if (handler->handle_read() < 0) {
                            del_handler(handler);
                        }
                    }
                    if (events_[i].events & EPOLLOUT) {
                        source = handler->source_;
                        if (source->source_state() != EventSource::STATE_CONNECTING) {
                            if (handler->handle_write() < 0) {
                                handler->reactor_->del_handler(handler, 0);
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
        }
        return ret;
    }

    inline int thread_start(int timeout_ms = 1)
    {
        max_timeout_ms_ = timeout_ms;
        return thread_.start(thread_process, this);
    }

    inline int thread_join()
    {
        thread_.join();
        return 0;
    }

    inline int thread_stop()
    {
        thread_.stop();
        return 0;
    }

    static int thread_process(void *arg)
    {
        EpollReactor<TMutex> *reactor = (EpollReactor<TMutex> *)arg;
        Thread &thread = reactor->thread_;
        while (thread.state() == Thread::RUNNING) {
            reactor->event_loop(reactor->max_timeout_ms_);
        }
        return 0;
    }

private:
    enum EPOLL_MODE
    {
        LT_MODE = 1,
        ET_MODE = 2,
    };

    uint_t  max_size_;
    uint_t  max_events_;
    uint_t  max_timeout_ms_;
    uint_t  current_handle_size_;

    int     epoll_mode_;
    int     epoll_fd_;
    struct  epoll_event *events_;

    TMutex      mutex_;
    Thread      thread_;
    ByteBuffer  recv_buffer_;

    ZRSOCKET_IOVEC *iovec_;
    int             iovec_count_;
};

ZRSOCKET_END

#endif

#endif
