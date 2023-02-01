// Some compilers (e.g. VC++) benefit significantly from using this. 
// We've measured 3-4% build speed improvements in apps as a result 
#pragma once

#ifndef ZRSOCKET_EVENT_SOURCE_H
#define ZRSOCKET_EVENT_SOURCE_H
#include "config.h"
#include "base_type.h"
#include "atomic.h"
#include "inet_addr.h"
#include "event_handler.h"
#include "event_loop.h"
#include "message_common.h"

ZRSOCKET_NAMESPACE_BEGIN

class ZRSOCKET_EXPORT EventSource
{
public:
    enum SOURCE_STATE
    {
        STATE_CLOSED      = 0,    //关闭
        STATE_CONNECTING  = 1,    //连接中
        STATE_CONNECTED   = 2,    //连接成功
    };

    enum SOURCE_TYPE
    {
        TYPE_UNKNOW = 0,    //未知
        TYPE_CLIENT = 1,    //客户
        TYPE_SERVER = 2,    //服务
    };

    EventSource()
        : handler_(nullptr)
        , event_loop_(nullptr)
        , message_decoder_config_(nullptr)
        , recvbuffer_size_(4096)
    {
    }

    virtual ~EventSource() = default;

    virtual InetAddr * local_addr()
    {
        return nullptr;
    }

    inline EventHandler * handler()
    {
        return handler_;
    }

    inline EventLoop * event_loop()
    {
        return event_loop_;
    }

    inline MessageDecoderConfig * message_decoder_config()
    {
        return message_decoder_config_;
    }

    inline uint_t recvbuffer_size()
    {
        return recvbuffer_size_;
    }

public:
    virtual EventHandler * alloc_handler()
    {
        return nullptr;
    }

    virtual int free_handler(EventHandler *handler)
    {
        return 0;
    }

    virtual SOURCE_TYPE source_type()
    {
        return EventSource::SOURCE_TYPE::TYPE_UNKNOW;
    }

    virtual SOURCE_STATE source_state()
    {
        return EventSource::SOURCE_STATE::STATE_CLOSED;
    }

    virtual void source_state(SOURCE_STATE state)
    {
    }

    virtual int connect()
    {
        return -1;
    }

protected:
    EventHandler         *handler_;
    EventLoop            *event_loop_;
    MessageDecoderConfig *message_decoder_config_;
    uint_t                recvbuffer_size_;
};

ZRSOCKET_NAMESPACE_END

#endif
