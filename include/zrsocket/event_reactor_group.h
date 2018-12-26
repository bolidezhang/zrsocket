#ifndef ZRSOCKET_EVENT_REACTOR_GROUP_H_
#define ZRSOCKET_EVENT_REACTOR_GROUP_H_
#include <vector>
#include "config.h"
#include "event_handler.h"
#include "event_reactor.h"

ZRSOCKET_BEGIN

template <typename TEventReactor>
class EventReactorGroup : public EventReactor
{
public:
    EventReactorGroup()
        : next_index_(0)
    {
    }

    virtual ~EventReactorGroup()
    {
        close();
        clear();
    }

    virtual TEventReactor* assign_reactor()
    {
        int reactors_size = (int)event_reactors_.size();
        if (reactors_size > 1) {
            int reactor_index = next_index_++;
            if (reactor_index >= reactors_size) {
                reactor_index -= reactors_size;
                next_index_    = reactor_index + 1;
            }
            return event_reactors_[reactor_index];
        }
        else {
            return event_reactors_[0];
        }
        return NULL;
    }

    int init(uint_t num = 2, uint_t max_events = 10000, int event_mode = 1)
    {
        if (num < 2) {
            num = 2;
        }

        event_reactors_.reserve(num);
        TEventReactor *reactor;
        for (uint_t i = 0; i < num; ++i) {
            reactor = new TEventReactor();
            reactor->init(num, max_events, event_mode);
            event_reactors_.emplace_back(reactor);
        }
        return 0;
    }

    int clear()
    {
        auto iter = event_reactors_.begin();
        auto iter_end = event_reactors_.end();
        for (; iter != iter_end; ++iter) {
            delete (*iter);
        }
        event_reactors_.clear();
        return 0; 
    }

    inline int set_buffer_size(int recv_buffer_size=32768, int send_buffer_size=32768) 
    { 
        TEventReactor *reactor;
        auto iter = event_reactors_.begin();
        auto iter_end = event_reactors_.end();
        for (; iter != iter_end; ++iter) {
            reactor = *iter;
            reactor->set_buffer_size(recv_buffer_size, send_buffer_size);
        }
        return 0; 
    }

    int open(uint_t max_size = 1024, int iovec_count = 1024, int max_timeout_ms = 1, int flags = 0)
    { 
        TEventReactor *reactor;
        auto iter = event_reactors_.begin();
        auto iter_end = event_reactors_.end();
        for (; iter != iter_end; ++iter) {
            reactor = *iter;
            reactor->open(max_size, iovec_count, max_timeout_ms, flags);
        }
        return 0;
    }

    int close() 
    { 
        TEventReactor *reactor;
        auto iter = event_reactors_.begin();
        auto iter_end = event_reactors_.end();
        for (; iter != iter_end; ++iter) {
            reactor = *iter;
            reactor->close();
        }
        return 0; 
    }

    inline int add_handler(EventHandler *handler, int event_mask)
    {
        TEventReactor *reactor;
        size_t size = event_reactors_.size();
        for (size_t i = 0; i < size; ++i) {
            reactor = assign_reactor();
            if (NULL != reactor) {
                if (reactor->add_handler(handler, event_mask) >= 0) {
                    handler->reactor_ = reactor;
                    return 0;
                }
            }
        }
        return -1;
    }

    inline int del_handler(EventHandler *handler, int event_mask)
    {
        return handler->reactor_->del_handler(handler, event_mask);
    }

    inline size_t get_handler_size()
    {
        size_t handler_size = 0;
        TEventReactor *reactor;
        auto iter = event_reactors_.begin();
        auto iter_end = event_reactors_.end();
        for (; iter != iter_end; ++iter) {
            reactor = *iter;
            handler_size += reactor->get_handler_size();
        }
        return handler_size;
    }

    inline int event_loop(int timeout_ms)
    {
        TEventReactor *reactor;
        auto iter = event_reactors_.begin();
        auto iter_end = event_reactors_.end();
        for (; iter != iter_end; ++iter) {
            reactor = *iter;
            reactor->event_loop(timeout_ms);
        }
        return 0; 
    }

    inline int thread_start(int timeout_ms)
    {
        TEventReactor *reactor;
        auto iter = event_reactors_.begin();
        auto iter_end = event_reactors_.end();
        for (; iter != iter_end; ++iter) {
            reactor = *iter;
            reactor->thread_start(timeout_ms);
        }
        return 0;
    }

    inline int thread_join()
    { 
        TEventReactor *reactor;
        auto iter = event_reactors_.begin();
        auto iter_end = event_reactors_.end();
        for (; iter != iter_end; ++iter) {
            reactor = *iter;
            reactor->thread_join();
        }
        return 0; 
    }

    inline int thread_stop()
    { 
        TEventReactor *reactor;
        auto iter = event_reactors_.begin();
        auto iter_end = event_reactors_.end();
        for (; iter != iter_end; ++iter) {
            reactor = *iter;
            reactor->thread_stop();
        }
        return 0; 
    }

protected:
    int next_index_;
    std::vector<TEventReactor * > event_reactors_;
};

ZRSOCKET_END

#endif
