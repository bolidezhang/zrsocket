// Some compilers (e.g. VC++) benefit significantly from using this. 
// We've measured 3-4% build speed improvements in apps as a result 
#pragma once

#ifndef ZRSOCKET_EVENT_TYPE_H
#define ZRSOCKET_EVENT_TYPE_H
#include "config.h"
#include "base_type.h"

ZRSOCKET_NAMESPACE_BEGIN

//#pragma pack(push)  //保存对齐状态
//#pragma pack(1)

template <typename TDataType>
class IEventType
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

    inline void type(int type)
    {
        type_ = type;
    }

public:
    inline IEventType(TDataType len, TDataType type)
        : len_(len)
        , type_(type)
    {
    }
    inline ~IEventType()
    {
    }

    TDataType len_;     //len of data, include sizeof(EventType)
    TDataType type_;    //type of EventType
};

#ifdef ZRSOCKET_BIG_EVENT_TYPE
using EventType = IEventType<uint16_t>;
#else
using EventType = IEventType<uint8_t>;
#endif

////////////////////////////////////////////////////////////////////////////////
//template of signal events (events without data)
//  type_value: the number of event type

template <int type_value>
class SignalEvent : public EventType
{
public:
    inline SignalEvent(int type = type_value)
        : EventType(sizeof(SignalEvent), type)
    {
    }

    inline ~SignalEvent() = default;
};

////////////////////////////////////////////////////////////////////////////////
//template of fixed-size events
//  TEvent: the final class name of the event
//  type_value: the number of event type

template <class TEvent, int type_value>
class FixedSizeEventBase : public EventType
{
protected:
    inline FixedSizeEventBase(int type = type_value)
        : EventType(sizeof(TEvent), type)
    {
    }

    inline ~FixedSizeEventBase() = default;
};

struct TimestampEventType : public EventType
{
public:
    TimestampEventType(int type)
        : EventType(sizeof(TimestampEventType), type)
        , timestamp_(OSApi::timestamp_ms())
    {
    }

    ~TimestampEventType() = default;

    inline int64_t timestamp() const
    {
        return timestamp_;
    }

    inline void timestamp(int64_t timestamp)
    {
    }

protected:

    //事件发生时间(建议时间单位:us, 可以自行定义) 用于控制线程处理太慢,事件在队列等待时间过长时,无需再处理此事件)
    uint64_t timestamp_;
};

//#pragma pack(pop)   //恢复对齐状态

////////////////////////////////////////////////////////////////////////////////
// event type ids
struct EventTypeId
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

        RPC_MESSAGE  = 13,             //rpc(支持thrift和protobuf等消息格式)
        RPC_MESSAGE2 = 14,             //rpc(支持thrift和protobuf等消息格式)

        USER_START = 32,
        USER_START_NUMBER = 32,
    };
};

using QuitEvent = SignalEvent<EventTypeId::QUIT_EVENT>;
using ThreadIdleEvent = SignalEvent<EventTypeId::THREAD_IDLE>;

ZRSOCKET_NAMESPACE_END

#endif
