// Some compilers (e.g. VC++) benefit significantly from using this. 
// We've measured 3-4% build speed improvements in apps as a result 
#pragma once

#ifndef ZRSOCKET_EVENT_LOOP_H
#define ZRSOCKET_EVENT_LOOP_H
#include "config.h"
#include "event_handler.h"
#include "timer_interface.h"
#include "byte_buffer.h"
#include "event_type.h"

ZRSOCKET_NAMESPACE_BEGIN

class ZRSOCKET_EXPORT EventLoop
{
public:
    EventLoop()
    {
    }

    virtual ~EventLoop()
    {
    }
    
    virtual int init(uint_t num, uint_t max_events, int event_mode, uint_t event_queue_max_size, uint_t event_type_len)
    {
        return 0;
    }

    virtual int buffer_size(int recv_buffer_size, int send_buffer_size) 
    {
        return 0;
    }

    virtual ByteBuffer * get_recv_buffer()
    {
        return nullptr;
    }

    virtual ByteBuffer * get_send_buffer()
    {
        return nullptr;
    }

    virtual ZRSOCKET_IOVEC * iovecs(int &iovecs_count)
    {
        return nullptr;
    }

    virtual void * get_loop_data()
    {
        return nullptr;
    }

    virtual int open(uint_t max_size, int iovec_count, int64_t max_timeout_us, int flags)
    { 
        return 0;
    }

    virtual int close()
    { 
        return 0;
    }

    virtual int add_handler(EventHandler *handler, int event_mask) = 0;
    //删除: 从event_loop中删除,且关闭和回收handler
    virtual int delete_handler(EventHandler *handler, int event_mask) = 0;
    //移除: 从event_loop中移除,但不关闭和回收handler
    //handler可以一个event_loop中移除, 随后可再加入新的event_loop中
    virtual int remove_handler(EventHandler *handler, int event_mask) = 0;

    virtual int add_event(EventHandler *handler, int event_mask) = 0;
    virtual int delete_event(EventHandler *handler, int event_mask) = 0;
    virtual int set_event(EventHandler *handler, int event_mask) = 0;

    virtual int add_timer(ITimer *timer) = 0;
    virtual int delete_timer(ITimer *timer) = 0;
    virtual int push_event(const EventType *event) = 0;

    virtual int loop(int64_t timeout_us) = 0;
    virtual int loop_wakeup() = 0;
    virtual int loop_thread_start(int64_t timeout_us) = 0;
    virtual int loop_thread_join() = 0;
    virtual int loop_thread_stop() = 0;

    virtual size_t handler_size() = 0;

};

ZRSOCKET_NAMESPACE_END

#endif
