// Some compilers (e.g. VC++) benefit significantly from using this. 
// We've measured 3-4% build speed improvements in apps as a result 
#pragma once

#ifndef ZRSOCKET_SEDA_EVENT_H
#define ZRSOCKET_SEDA_EVENT_H
#include "config.h"
#include "base_type.h"
#include "byte_buffer.h"
#include "event_handler.h"
#include "inet_addr.h"
#include "seda_timer.h"

ZRSOCKET_NAMESPACE_BEGIN

class SedaEvent
{
public:
    inline uint8_t * event_ptr() const
    {
        return ((uint8_t *)this);
    }

    inline int event_len() const
    {
        return len_;
    }

    inline int type() const
    {
        return type_;
    }

    inline int64_t timestamp() const
    {
        return timestamp_;
    }

    inline void timestamp(int64_t timestamp)
    {
        timestamp_ = timestamp;
    }

public:
    inline SedaEvent(int len, int type)
        : len_(len)
        , type_(type)
        , timestamp_(0)
    {
        //std::condition_variable cv;
        //std::condition_variable_any cvy;
    }

    inline ~SedaEvent()
    {
    }

    int         len_;       //len of data, include sizeof(SedaEvent)
    int         type_;      //type of SedaEvent
    uint64_t    timestamp_; //事件发生时间(建议时间单位:us, 可以自行定义) 用于控制SEDA线程处理太慢,事件在队列等待时间过长时,无需再处理此事件)
};

////////////////////////////////////////////////////////////////////////////////
//template of signal events (events without data)
//  type_value: the number of event type

template <int type_value>
class SedaSignalEvent : public SedaEvent
{
public:
    inline SedaSignalEvent(int type = type_value)
        : SedaEvent(sizeof(SedaSignalEvent), type)
    {
    }

    inline ~SedaSignalEvent()
    {
    }
};

////////////////////////////////////////////////////////////////////////////////
//template of fixed-size events
//  TEvent: the final class name of the event
//  type_value: the number of event type

template <class TEvent, int type_value>
class SedaFixedSizeEventBase : public SedaEvent
{
protected:
    inline SedaFixedSizeEventBase(int type = type_value)
        : SedaEvent(sizeof(TEvent), type)
    {
    }

    inline ~SedaFixedSizeEventBase()
    {
    }
};

////////////////////////////////////////////////////////////////////////////////
//  base event types
struct SedaEventType
{
    enum
    {
        QUIT_EVENT = 0,
        THREAD_IDLE = 1,
        TIMER_EXPIRE = 2,

        TCP_LISTEN = 3,
        TCP_SOCKET_OPEN = 4,
        TCP_SOCKET_CLOSE = 5,
        TCP_READ_DATA = 6,
        TCP_READ_MESSAGE = 7,
        TCP_WRITE_DATA = 8,

        UDP_SOCKET_OPEN = 9,
        UDP_SOCKET_CLOSE = 10,
        UDP_READ_DATA = 11,
        UDP_WRITE_DATA = 12,

        RPC_MESSAGE = 13,              //rpc(支持thrift和protobuf等消息格式)
        RPC_MESSAGE2 = 14,             //rpc(支持thrift和protobuf等消息格式)

        USER_START = 1000,
        USER_START_NUMBER = 1000,
    };
};

typedef SedaSignalEvent<SedaEventType::QUIT_EVENT>  SedaQuitEvent;
typedef SedaSignalEvent<SedaEventType::THREAD_IDLE> SedaThreadIdleEvent;

struct SedaTimerExpireEvent : public SedaEvent
{
    SedaTimer *timer;
    int slot;

    inline SedaTimerExpireEvent()
        : SedaEvent(sizeof(SedaTimerExpireEvent), SedaEventType::TIMER_EXPIRE)
    {
    }

    inline ~SedaTimerExpireEvent()
    {
    }
};

struct SedaTcpSocketOpenEvent : SedaEvent
{
    EventHandler *handler;

    inline SedaTcpSocketOpenEvent()
        : SedaEvent(sizeof(SedaTcpSocketOpenEvent), SedaEventType::TCP_SOCKET_OPEN)
    {
    }

    inline ~SedaTcpSocketOpenEvent()
    {
    }
};

struct SedaTcpSocketCloseEvent : SedaEvent
{
    EventHandler *handler;
    int64_t attach_id1;
    int64_t attach_id2;
    void *attach_object1;
    void *attach_object2;

    inline SedaTcpSocketCloseEvent()
        : SedaEvent(sizeof(SedaTcpSocketCloseEvent), SedaEventType::TCP_SOCKET_CLOSE)
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
        : SedaEvent(sizeof(SedaTcpReadDataEvent), SedaEventType::TCP_READ_DATA)
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
        : SedaEvent(sizeof(SedaTcpReadDataEvent), SedaEventType::TCP_READ_MESSAGE)
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
        : SedaEvent(sizeof(SedaTcpWriteDataEvent), SedaEventType::TCP_WRITE_DATA)
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
        : SedaEvent(sizeof(SedaRpcMessageEvent), SedaEventType::RPC_MESSAGE)
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
        : SedaEvent(sizeof(SedaRpcMessageEvent2), SedaEventType::RPC_MESSAGE2)
    {
    }

    inline ~SedaRpcMessageEvent2()
    {
    }
};

ZRSOCKET_NAMESPACE_END

#endif
