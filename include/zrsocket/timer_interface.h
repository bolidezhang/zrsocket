// Some compilers (e.g. VC++) benefit significantly from using this. 
// We've measured 3-4% build speed improvements in apps as a result 
#pragma once

#ifndef ZRSOCKET_TIMER_INTERFACE_H
#define ZRSOCKET_TIMER_INTERFACE_H
#include "base_type.h"
#include "event_loop.h"

ZRSOCKET_NAMESPACE_BEGIN

class ITimer
{
public:
    ITimer() = default;
    virtual ~ITimer() = default;

    virtual int handle_timeout()
    {
        return 0;
    }
};

class ITimerQueue
{
public:
    ITimerQueue() = default;
    virtual ~ITimerQueue() = default;

    virtual int     add_timer(ITimer *timer, uint64_t current_timestamp) = 0;
    virtual int     delete_timer(ITimer *timer) = 0;
    virtual int     loop(uint64_t current_timestamp) = 0;
    virtual size_t  size() = 0;
    virtual bool    empty() = 0;
    virtual int64_t min_interval() = 0;

    virtual EventLoop * event_loop() const = 0;
    virtual void event_loop(EventLoop *e) = 0;
};

ZRSOCKET_NAMESPACE_END

#endif
