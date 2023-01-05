// Some compilers (e.g. VC++) benefit significantly from using this. 
// We've measured 3-4% build speed improvements in apps as a result 
#pragma once

#ifndef ZRSOCKET_SEDA_EVENT_H
#define ZRSOCKET_SEDA_EVENT_H
#include "event_type.h"
#include "event_handler.h"
#include "seda_timer.h"

ZRSOCKET_NAMESPACE_BEGIN

typedef EventType       SedaEvent;
typedef EventTypeId     SedaEventTypeId;
typedef QuitEvent       SedaQuitEvent;
typedef ThreadIdleEvent SedaThreadIdleEvent;

struct SedaTimerExpireEvent : public SedaEvent
{
    SedaTimer *timer;
    int slot;

    inline SedaTimerExpireEvent()
        : SedaEvent(sizeof(SedaTimerExpireEvent), SedaEventTypeId::TIMER_EXPIRE)
    {
    }

    inline ~SedaTimerExpireEvent()
    {
    }
};

struct SedaTcpSocketOpenEvent : public SedaEvent
{
    EventHandler *handler;

    inline SedaTcpSocketOpenEvent()
        : SedaEvent(sizeof(SedaTcpSocketOpenEvent), SedaEventTypeId::TCP_SOCKET_OPEN)
    {
    }

    inline ~SedaTcpSocketOpenEvent()
    {
    }
};

struct SedaTcpSocketCloseEvent : public SedaEvent
{
    EventHandler *handler;
    int64_t attach_id1;
    int64_t attach_id2;
    void *attach_object1;
    void *attach_object2;

    inline SedaTcpSocketCloseEvent()
        : SedaEvent(sizeof(SedaTcpSocketCloseEvent), SedaEventTypeId::TCP_SOCKET_CLOSE)
    {
    }

    inline ~SedaTcpSocketCloseEvent()
    {
    }
};

struct SedaTcpReadDataEvent : public SedaEvent
{
    EventHandler *handler;
    ByteBuffer   *data;
    int64_t attach_id1;
    int64_t attach_id2;
    void *attach_object1;
    void *attach_object2;

    inline SedaTcpReadDataEvent()
        : SedaEvent(sizeof(SedaTcpReadDataEvent), SedaEventTypeId::TCP_READ_DATA)
    {
    }

    inline ~SedaTcpReadDataEvent()
    {
    }
};

struct SedaTcpReadMessageEvent : public SedaEvent
{
    EventHandler *handler;
    ByteBuffer   *message;
    int64_t attach_id1;
    int64_t attach_id2;
    void *attach_object1;
    void *attach_object2;

    inline SedaTcpReadMessageEvent()
        : SedaEvent(sizeof(SedaTcpReadDataEvent), SedaEventTypeId::TCP_READ_MESSAGE)
    {
    }

    inline ~SedaTcpReadMessageEvent()
    {
    }
};

struct SedaTcpWriteDataEvent : public SedaEvent
{
    EventHandler *handler;
    int64_t attach_id1;
    int64_t attach_id2;
    void *attach_object1;
    void *attach_object2;

    inline SedaTcpWriteDataEvent()
        : SedaEvent(sizeof(SedaTcpWriteDataEvent), SedaEventTypeId::TCP_WRITE_DATA)
    {
    }

    inline ~SedaTcpWriteDataEvent()
    {
    }
};

typedef int (*SedaRpcMessageProc)(int handler_id, EventHandler *handler, void *seda_stagehandler, void *rpc_message, InetAddr *remote_addr);
struct SedaRpcMessageEvent : public SedaEvent
{
    EventHandler *handler;
    SedaRpcMessageProc rpc_proc;
    void *rpc_message;
    InetAddr *remote_addr;
    int handler_id;

    inline SedaRpcMessageEvent()
        : SedaEvent(sizeof(SedaRpcMessageEvent), SedaEventTypeId::RPC_MESSAGE)
    {
    }

    inline ~SedaRpcMessageEvent()
    {
    }
};

struct SedaRpcMessageEvent2;
typedef int (*SedaRpcMessageProc2)(SedaRpcMessageEvent2 *rpc_event, void *seda_stagehandler);
struct SedaRpcMessageEvent2 : public SedaEvent
{
    EventHandler *handler;
    SedaRpcMessageProc2 rpc_proc;
    void *rpc_message;
    InetAddr *remote_addr;
    int64_t handler_id;

    inline SedaRpcMessageEvent2()
        : SedaEvent(sizeof(SedaRpcMessageEvent2), SedaEventTypeId::RPC_MESSAGE2)
    {
    }

    inline ~SedaRpcMessageEvent2()
    {
    }
};

ZRSOCKET_NAMESPACE_END

#endif
