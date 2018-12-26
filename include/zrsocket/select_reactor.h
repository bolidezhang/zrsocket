#ifndef ZRSOCKET_REACTOR_H_
#define ZRSOCKET_REACTOR_H_
#include <algorithm>
#include <unordered_map>
#include <unordered_set>
#include "config.h"
#include "base_type.h"
#include "memory.h"
#include "byte_buffer.h"
#include "mutex.h"
#include "thread.h"
#include "system_api.h"
#include "event_handler.h"
#include "event_reactor.h"

#ifndef ZRSOCKET_OS_WINDOWS
    #include <sys/select.h>
#endif

ZRSOCKET_BEGIN

template <typename TMutex>
class SelectReactor : public EventReactor
{
public:
    SelectReactor()
        : read_size_(0)
        , write_size_(0)
#ifdef ZRSOCKET_OS_WINDOWS
        , except_size_(0)
#endif
        , max_timeout_ms_(1)
#ifndef ZRSOCKET_OS_WINDOWS
        , max_fd_(0)
#endif
        , iovec_(NULL)
        , iovec_count_(0)
    {
        temp_handlers_active_   = &temp1_handlers_;
        temp_handlers_standby_  = &temp2_handlers_;
        set_buffer_size();
    }

    virtual ~SelectReactor()
    {
        close();
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

    int open(uint_t max_size = 1024, int iovec_count = 1024, int max_timeout_ms = 10, int flags = 0)
    {
        if (max_size <= 0) {
            return -1;
        }
        if (iovec_count <= 0) {
            return -1;
        }

        if (max_size > FD_SETSIZE) {
            max_size = FD_SETSIZE;
        }
        if (iovec_count > ZRSOCKET_MAX_IOVCNT) {
            iovec_count = ZRSOCKET_MAX_IOVCNT;
        }
        max_size_       = max_size;
        max_timeout_ms_ = max_timeout_ms;
        iovec_count_    = iovec_count;
        iovec_          = new ZRSOCKET_IOVEC[iovec_count];

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
        if (NULL != iovec_) {
            delete []iovec_;
            iovec_ = NULL;
        }
        return 0;
    }

    int add_handler(EventHandler *handler, int event_mask)
    {
        mutex_.lock();
        if (handler->in_reactor_) {
            mutex_.unlock();
            return -1;
        }

        if ( (read_size_ >= max_size_) && (event_mask & EventHandler::READ_EVENT_MASK) ){
            mutex_.unlock();
            return -1;
        }
        if ((write_size_ >= max_size_) && (event_mask & EventHandler::WRITE_EVENT_MASK)) {
            mutex_.unlock();
            return -1;
        }
#ifdef ZRSOCKET_OS_WINDOWS
        if ((except_size_ >= max_size_) && (event_mask & EventHandler::EXCEPT_EVENT_MASK)) {
            mutex_.unlock();
            return -1;
        }
#endif

        int add_event_mask = EventHandler::NULL_EVENT_MASK;
        if (event_mask & EventHandler::READ_EVENT_MASK) {
            FD_SET(handler->socket_, &fd_set_in_.read);
            add_event_mask |= EventHandler::READ_EVENT_MASK;
            ++read_size_;
        }
        if (event_mask & EventHandler::WRITE_EVENT_MASK) {
            FD_SET(handler->socket_, &fd_set_in_.write);
            add_event_mask |= EventHandler::WRITE_EVENT_MASK;
            ++write_size_;
        }
#ifdef ZRSOCKET_OS_WINDOWS
        if (event_mask & EventHandler::EXCEPT_EVENT_MASK) {
            FD_SET(handler->socket_, &fd_set_in_.except);
            add_event_mask |= EventHandler::EXCEPT_EVENT_MASK;
            ++except_size_;
        }
#else
        if (max_fd_ < handler->socket_) {
            max_fd_ = handler->socket_;
        }
#endif
        handler->in_reactor_ = true;
        handler->event_mask_ = add_event_mask;
        temp_handlers_standby_->emplace(handler, ADD_HANDLER);
        mutex_.unlock();

        return 0;
    }

    int del_handler(EventHandler *handler, int event_mask)
    {
        mutex_.lock();
        if (!handler->in_reactor_) {
            mutex_.unlock();
            return -1;
        }

        event_mask = handler->event_mask_;
        if (event_mask & EventHandler::READ_EVENT_MASK) {
            FD_CLR(handler->socket_, &fd_set_in_.read);
            --read_size_;
        }
        if (event_mask & EventHandler::WRITE_EVENT_MASK) {
            FD_CLR(handler->socket_, &fd_set_in_.write);
            --write_size_;
        }
#ifdef ZRSOCKET_OS_WINDOWS
        if (event_mask & EventHandler::EXCEPT_EVENT_MASK) {
            FD_CLR(handler->socket_, &fd_set_in_.except);
            --except_size_;
        }
#else
        if (max_fd_ == handler->socket_) {
            --max_fd_;
        }
#endif
        handler->in_reactor_  = false;
        handler->event_mask_  = EventHandler::NULL_EVENT_MASK;
        temp_handlers_standby_->emplace(handler, DEL_HANDLER);
        mutex_.unlock();

        return 0;
    }

    int add_event(EventHandler *handler, int event_mask)
    {
        mutex_.lock();
        if (!handler->in_reactor_) {
            mutex_.unlock();
            return -1;
        }

        int add_event_mask = EventHandler::NULL_EVENT_MASK;
        if ( (event_mask & EventHandler::READ_EVENT_MASK) && 
             !(handler->event_mask_ & EventHandler::READ_EVENT_MASK) ) {
            FD_SET(handler->socket_, &fd_set_in_.read);
            add_event_mask |= EventHandler::READ_EVENT_MASK;
            ++read_size_;
        }
        if ( (event_mask & EventHandler::WRITE_EVENT_MASK) && 
            !(handler->event_mask_ & EventHandler::WRITE_EVENT_MASK) ) {
            FD_SET(handler->socket_, &fd_set_in_.write);
            add_event_mask |= EventHandler::WRITE_EVENT_MASK;
            ++write_size_;
        }
#ifdef ZRSOCKET_OS_WINDOWS
        if ( (event_mask & EventHandler::EXCEPT_EVENT_MASK) &&
             !(handler->event_mask_ & EventHandler::EXCEPT_EVENT_MASK) ) {
            FD_SET(handler->socket_, &fd_set_in_.except);
            add_event_mask |= EventHandler::EXCEPT_EVENT_MASK;
            ++except_size_;
        }
#endif
        handler->event_mask_ |= add_event_mask;
        mutex_.unlock();

        return 0;
    }

    int del_event(EventHandler *handler, int event_mask)
    {
        mutex_.lock();
        if (!handler->in_reactor_) {
            mutex_.unlock();
            return -1;
        }

        int del_event_mask = EventHandler::NULL_EVENT_MASK;
        if (handler->event_mask_ & event_mask & EventHandler::READ_EVENT_MASK) {
            FD_CLR(handler->socket_, &fd_set_in_.read);
            del_event_mask |= EventHandler::READ_EVENT_MASK;
            --read_size_;
        }
        if (handler->event_mask_ & event_mask & EventHandler::WRITE_EVENT_MASK) {
            FD_CLR(handler->socket_, &fd_set_in_.write);
            del_event_mask |= EventHandler::WRITE_EVENT_MASK;
            --write_size_;
        }
#ifdef ZRSOCKET_OS_WINDOWS
        if (handler->event_mask_ & event_mask & EventHandler::EXCEPT_EVENT_MASK) {
            FD_CLR(handler->socket_, &fd_set_in_.except);
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
        if (!handler->in_reactor_) {
            mutex_.unlock();
            return -1;
        }

        int set_event_mask = EventHandler::NULL_EVENT_MASK;
        if (event_mask & EventHandler::READ_EVENT_MASK) {
            if (!(handler->event_mask_ & EventHandler::READ_EVENT_MASK)) {
                FD_SET(handler->socket_, &fd_set_in_.read);
                set_event_mask |= EventHandler::READ_EVENT_MASK;
                ++read_size_;
            }
        }
        else {
            if (handler->event_mask_ & EventHandler::READ_EVENT_MASK) {
                FD_CLR(handler->socket_, &fd_set_in_.read);
                --read_size_;
            }
        }

        if (event_mask & EventHandler::WRITE_EVENT_MASK) {
            if (!(handler->event_mask_ & EventHandler::WRITE_EVENT_MASK)) {
                FD_SET(handler->socket_, &fd_set_in_.write);
                set_event_mask |= EventHandler::WRITE_EVENT_MASK;
                ++write_size_;
            }
        }
        else {
            if (handler->event_mask_ & EventHandler::WRITE_EVENT_MASK) {
                FD_CLR(handler->socket_, &fd_set_in_.write);
                --write_size_;
            }
        }

#ifdef ZRSOCKET_OS_WINDOWS
        if (event_mask & EventHandler::EXCEPT_EVENT_MASK) {
            if (!(handler->event_mask_ & EventHandler::EXCEPT_EVENT_MASK)) {
                FD_SET(handler->socket_, &fd_set_in_.except);
                set_event_mask |= EventHandler::EXCEPT_EVENT_MASK;
                ++except_size_;
            }
        }
        else {
            if (handler->event_mask_ & EventHandler::EXCEPT_EVENT_MASK) {
                FD_CLR(handler->socket_, &fd_set_in_.except);
                --except_size_;
            }
        }
#endif
        handler->event_mask_ = set_event_mask;
        mutex_.unlock();

        return 0;
    }
    inline size_t get_handler_size()
    {
        mutex_.lock();
        size_t handlers_size = ZRSOCKET_MAX(read_size_, write_size_);
#ifdef ZRSOCKET_OS_WINDOWS
        handlers_size = ZRSOCKET_MAX(handlers_size, except_size_);
#endif
        mutex_.unlock();
        return handlers_size;
    }

    int event_loop(int timeout_ms = 1)
    {
        static int fd_set_size = sizeof(fd_set);
        EventHandler *handler = NULL;
        int  ready;
#ifdef ZRSOCKET_OS_WINDOWS
        bool have_event = false;
#endif

        //½»»»active/standbyÖ¸Õë
        mutex_.lock();
#ifdef ZRSOCKET_OS_WINDOWS
        if (read_size_ || write_size_ || except_size_) {
            have_event = true;
        }
#endif
        if (!temp_handlers_standby_->empty()) {
            std::swap(temp_handlers_standby_, temp_handlers_active_);
            zrsocket_memcpy(&fd_set_out_.read,   &fd_set_in_.read,   fd_set_size);
            zrsocket_memcpy(&fd_set_out_.write,  &fd_set_in_.write,  fd_set_size);
#ifdef ZRSOCKET_OS_WINDOWS
            zrsocket_memcpy(&fd_set_out_.except, &fd_set_in_.except, fd_set_size);
#endif
            mutex_.unlock();

            auto iter = temp_handlers_active_->begin();
            auto iter_end = temp_handlers_active_->end();
            for (; iter != iter_end; ++iter) {
                handler = iter->first;
                if (iter->second == ADD_HANDLER) {
                    handlers_.emplace(handler);
                }
                else {
                    handlers_.erase(handler);
                    handler->handle_close();
                    handler->source_->free_handler(handler);
                }
            }
            temp_handlers_active_->clear();
        }
        else {
            zrsocket_memcpy(&fd_set_out_.read,   &fd_set_in_.read,   fd_set_size);
            zrsocket_memcpy(&fd_set_out_.write,  &fd_set_in_.write,  fd_set_size);
#ifdef ZRSOCKET_OS_WINDOWS
            zrsocket_memcpy(&fd_set_out_.except, &fd_set_in_.except, fd_set_size);
#endif
            mutex_.unlock();
        }

#ifdef ZRSOCKET_OS_WINDOWS
        if (have_event) {
            if (timeout_ms >= 0) {
                timeval tv;
                tv.tv_sec = timeout_ms / 1000;
                tv.tv_usec = (timeout_ms - tv.tv_sec * 1000) * 1000;
                ready = select(0, &fd_set_out_.read, &fd_set_out_.write, &fd_set_out_.except, &tv);
            }
            else {
                ready = select(0, &fd_set_out_.read, &fd_set_out_.write, &fd_set_out_.except, NULL);
            }
        }
        else {
            if (timeout_ms > 0) {
                SystemApi::util_sleep_us(timeout_ms);
            }
            return 0;
        }
#else
        if (timeout_ms >= 0) {
            timeval tv;
            tv.tv_sec = timeout_ms / 1000;
            tv.tv_usec = (timeout_ms - tv.tv_sec * 1000) * 1000;
            ready = select(max_fd_+1, &fd_set_out_.read, &fd_set_out_.write, NULL, &tv);
        }
        else {
            ready = select(max_fd_+1, &fd_set_out_.read, &fd_set_out_.write, NULL, NULL);
        }
#endif

        if (ready > 0) {
            EventSource  *source;
            EVENT_HANDLES::iterator iter = handlers_.begin();
            EVENT_HANDLES::iterator iter_end = handlers_.end();
            for (; iter != iter_end; ++iter) {
                handler = *iter;
                if (FD_ISSET(handler->socket_, &fd_set_out_.read)) {
                    if (handler->handle_read() < 0) {
                        handler->reactor_->del_handler(handler, 0);
                    }
                    if (--ready <= 0) {
                        break;
                    }
                }

#ifdef ZRSOCKET_OS_WINDOWS
                if (FD_ISSET(handler->socket_, &fd_set_out_.write)) {
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
                    if (--ready <= 0) {
                        break;
                    }
                }

                if (FD_ISSET(handler->socket_, &fd_set_out_.except)) {
                    handler->reactor_->del_handler(handler, 0);
                    if (--ready <= 0) {
                        break;
                    }
                }
#else
                if (FD_ISSET(handler->socket_, &fd_set_out_.write)) {
                    source = handler->source_;
                    if (source->source_state() != EventSource::STATE_CONNECTING) {
                        if (handler->handle_write() < 0) {
                            handler->reactor_->del_handler(handler, 0);
                        }
                    }
                    else { //nonblock connect                        
                        if (SystemApi::socket_get_error(handler->socket_) == 0) {
                            del_event(handler, EventHandler::WRITE_EVENT_MASK);
                            add_event(handler, EventHandler::READ_EVENT_MASK);
                            source->source_state(EventSource::STATE_CONNECTED);
                            handler->state_ = EventHandler::STATE_CONNECTED;
                            handler->handle_connect();
                        }
                        else {
                            handler->reactor_->del_handler(handler, 0);
                        }
                    }
                    if (--ready <= 0) {
                        break;
                    }
                }
#endif
            }
        }

        return ready;
    }

    inline int thread_start(int timeout_ms = 1)
    {
        max_timeout_ms_ = timeout_ms;
        return thread_.start(thread_process, this);
    }

    inline int thread_join()
    {
        return thread_.join();
    }

    inline int thread_stop()
    {
        return thread_.stop();
    }

private:
    static int thread_process(void *arg)
    {
        SelectReactor<TMutex> *reactor = (SelectReactor<TMutex> *)arg;
        Thread &thread = reactor->thread_;
        while (thread.state() == Thread::RUNNING) {
            reactor->event_loop(reactor->max_timeout_ms_);
        }
        return 0;
    }

    //set<handler *>
    typedef std::unordered_set<EventHandler *> EVENT_HANDLES;

    //map<handler *, ADD_HANDLER/del_HANDLER>
    typedef std::unordered_map<EventHandler *, int> TEMP_EVENT_HANDLES;

    enum
    {
        ADD_HANDLER = 1,
        DEL_HANDLER = 2,
    };

    struct ZRSOCKET_FD_SET
    {
        fd_set  read;
        fd_set  write;
#ifdef ZRSOCKET_OS_WINDOWS
        fd_set  except;
#endif
    };

    uint_t  max_size_;
    uint_t  read_size_;
    uint_t  write_size_;
#ifdef ZRSOCKET_OS_WINDOWS
    uint_t  except_size_;
#endif
    int     max_timeout_ms_;
#ifndef ZRSOCKET_OS_WINDOWS
    int     max_fd_;
#endif

    ZRSOCKET_FD_SET     fd_set_in_;
    ZRSOCKET_FD_SET     fd_set_out_;

    TMutex              mutex_;
    Thread              thread_;

    EVENT_HANDLES       handlers_;
    TEMP_EVENT_HANDLES  temp1_handlers_;
    TEMP_EVENT_HANDLES  temp2_handlers_;
    TEMP_EVENT_HANDLES *temp_handlers_active_;
    TEMP_EVENT_HANDLES *temp_handlers_standby_;

    ByteBuffer           recv_buffer_;

    ZRSOCKET_IOVEC      *iovec_;
    int                  iovec_count_;
};

ZRSOCKET_END

#endif
