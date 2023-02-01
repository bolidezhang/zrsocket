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

//#pragma pack(push)  //±£´æ¶ÔÆë×´Ì¬
//#pragma pack(1)

struct SedaTimerExpireEvent : public SedaEvent
{
    int slot = 0;
    SedaTimer *timer = nullptr;

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
    EventHandler *handler = nullptr;

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
    EventHandler *handler = nullptr;
    int64_t attach_id1 = 0;
    int64_t attach_id2 = 0;
    void *attach_object1 = nullptr;
    void *attach_object2 = nullptr;

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
    EventHandler *handler = nullptr;
    ByteBuffer   *data = nullptr;
    int64_t attach_id1 = 0;
    int64_t attach_id2 = 0;
    void *attach_object1 = nullptr;
    void *attach_object2 = nullptr;

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
    EventHandler *handler = nullptr;
    ByteBuffer   *message = nullptr;
    int64_t attach_id1 = 0;
    int64_t attach_id2 = 0;
    void *attach_object1 = nullptr;
    void *attach_object2 = nullptr;

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
    EventHandler *handler = nullptr;;
    int64_t attach_id1 = 0;
    int64_t attach_id2 = 0;
    void *attach_object1 = nullptr;
    void *attach_object2 = nullptr;

    inline SedaTcpWriteDataEvent()
        : SedaEvent(sizeof(SedaTcpWriteDataEvent), SedaEventTypeId::TCP_WRITE_DATA)
    {
    }

    inline ~SedaTcpWriteDataEvent()
    {
    }
};

typedef int (* SedaRpcMessageProc)(int handler_id, EventHandler *handler, void *seda_stagehandler, void *rpc_message, InetAddr *remote_addr);
struct SedaRpcMessageEvent : public SedaEvent
{
    EventHandler *handler = nullptr;;
    SedaRpcMessageProc rpc_proc = nullptr;
    void *rpc_message = nullptr;
    InetAddr *remote_addr = nullptr;
    int handler_id = 0;

    inline SedaRpcMessageEvent()
        : SedaEvent(sizeof(SedaRpcMessageEvent), SedaEventTypeId::RPC_MESSAGE)
    {
    }

    inline ~SedaRpcMessageEvent()
    {
    }
};

struct SedaRpcMessageEvent2;
typedef int (* SedaRpcMessageProc2)(SedaRpcMessageEvent2 *rpc_event, void *seda_stagehandler);
struct SedaRpcMessageEvent2 : public SedaEvent
{
    EventHandler *handler = nullptr;
    SedaRpcMessageProc2 rpc_proc = nullptr;
    void *rpc_message = nullptr;
    InetAddr *remote_addr = nullptr;
    int64_t handler_id = 0;

    inline SedaRpcMessageEvent2()
        : SedaEvent(sizeof(SedaRpcMessageEvent2), SedaEventTypeId::RPC_MESSAGE2)
    {
    }

    inline ~SedaRpcMessageEvent2()
    {
    }
};

//#pragma pack(pop)   //»Ö¸´¶ÔÆë×´Ì¬

ZRSOCKET_NAMESPACE_END

#endif
