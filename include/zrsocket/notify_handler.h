#pragma once

#ifndef ZRSOCKET_NOTIFY_HANDLER_H
#define ZRSOCKET_NOTIFY_HANDLER_H
#include "config.h"
#include "event_handler.h"
#include "global.h"

ZRSOCKET_NAMESPACE_BEGIN

#ifdef ZRSOCKET_OS_LINUX
// os eventfd api
#include <sys/eventfd.h>

class NotifyHandler : public EventHandler
{
public:
    NotifyHandler()
    {
        in_event_loop_ = false;
        source_ = &Global::instance().null_event_source_;
    }

    virtual ~NotifyHandler()
    {
        close();
    }

    int open(unsigned int initval = 0, int flags = EFD_NONBLOCK)
    {
        close();

        fd_ = eventfd(initval, flags);
        if (fd_ < 0) {
            in_event_loop_ = true;
            return -OSApi::socket_get_lasterror();
        }

        return 0;
    }

    void close()
    {
        if (fd_ != ZRSOCKET_INVALID_SOCKET) {
            ::close(fd_);
        }
    }

    int wait()
    {
        uint64_t value = 0;
        return wait(value);
    }

    int notify()
    {
        static uint64_t value = 1;
        return notify(value);
    }

    int handle_read()
    {
#ifdef ZRSOCKET_DEBUG
        printf("NotifyHanlder::handle_read fd:%d\n", fd_);
#endif
        uint64_t value;
        wait(value);
        return 0;
    }

    inline int wait(uint64_t &value)
    {
        return ::read(fd_, &value, sizeof(uint64_t));
    }

    inline int notify(uint64_t &value)
    {
        return ::write(fd_, &value, sizeof(uint64_t));
    }
};
#else

class NotifyHandler : public EventHandler
{
public:
    NotifyHandler()
    {
        in_event_loop_ = true;
    }

    virtual ~NotifyHandler()
    {
        close();
    }

    int open(unsigned int initval = 0, int flags = 0)
    {
        return 0;
    }

    void close()
    {
    }

    int notify()
    {
        return 0;
    }

    int wait()
    {
        return 0;
    }

    int handle_read()
    {
        return 0;
    }
};

#endif

ZRSOCKET_NAMESPACE_END

#endif
