// Some compilers (e.g. VC++) benefit significantly from using this. 
// We've measured 3-4% build speed improvements in apps as a result 
#pragma once

#ifndef ZRSOCKET_EVENT_HANDLER_H
#define ZRSOCKET_EVENT_HANDLER_H
#include "config.h"
#include "base_type.h"
#include "byte_buffer.h"
#include "os_api.h"
#include "inet_addr.h"

ZRSOCKET_NAMESPACE_BEGIN

//消息发送结果
enum class SendResult
{
    FAILURE     = -100000,  // <0:发送失败
    PUSH_QUEUE  = 0,        //==0:入送队列
    SUCCESS     = 1,        //==1:发送成功
    END         = 2,        //==2:结束
};

class EventSource;
class EventLoop;
template <class TMutex, class TLoopData, class TEventTypeHandler> class SelectEventLoop;
template <class TMutex, class TLoopData, class TEventTypeHandler> class EpollEventLoop;
template <class TMutex, class TLoopData, class TEventTypeHandler> class EpollETEventLoop;
template <class TEventLoop> class EventLoopGroup;
template <class TClientHandler, class TObjectPool, class TServerHandler> class TcpServer;
template <class TUdpSourceHandler> class UdpSource;
template <class THandler> class TcpClient;

class ZRSOCKET_EXPORT EventHandler
{
public:
    enum EVENT_MASK
    {
        NULL_EVENT_MASK         = 0x00000000,
        READ_EVENT_MASK         = 0x00000001,
        WRITE_EVENT_MASK        = 0x00000002,
        EXCEPT_EVENT_MASK       = 0x00000004,
        ALL_EVENT_MASK          = READ_EVENT_MASK | WRITE_EVENT_MASK | EXCEPT_EVENT_MASK,
    };

    enum HANDLER_STATE
    {
        STATE_CLOSED            = 0x00,             //关闭
        STATE_OPENED            = 0x01,             //打开
        STATE_CONNECTED         = 0x02,             //连接成功,可收发数据
        STATE_CLOSE_WAIT        = 0x04,             //等待关闭(应用层主动要关闭socket,socket还未关闭)
        STATE_CLOSE_SEND        = 0x08,             //关闭发送通道(socket已关闭发送通道,但可接收数据)
        STATE_CLOSE_RECV        = 0x10,             //关闭接收通道(socket已关闭接收通道,但可发送数据)
    };

    enum HANDLE_ERROR
    {
        ERROR_START_NUMBER      = -1000000,

        ERROR_CLOSE_PASSIVE     = -1000001,         //被动关闭(远端关闭remote)
        ERROR_CLOSE_ACTIVE      = -1000002,         //主动关闭(本地关闭local)
        ERROR_CLOSE_RECV        = -1000003,         //接收时组包出错关闭
        ERROR_CLOSE_SEND        = -1000004,         //发送数据出错关闭
        ERROR_KEEPALIVE_TIMEOUT = -1000005,         //keepalive超时关闭

        ERROR_END_NUMBER        = -1009999,
    };

    enum WriteResult
    {
        WRITE_RESULT_NOT_INITIAL    = -3,           //没初始化
        WRITE_RESULT_STATUS_INVALID = -2,           //表示Socket状态失效
        WRITE_RESULT_FAILURE        = -1,           //表示发送失败(Socket已失效)
        WRITE_RESULT_NOT_DATA       = 0,            //表示没有数据可发送
        WRITE_RESULT_SUCCESS        = 1,            //表示发送成功
        WRITE_RESULT_PART           = 2             //表示发送出部分数据(包含0长度)
    };

    inline EventHandler()
        : source_(nullptr)
        , event_loop_(nullptr)
        , fd_(ZRSOCKET_INVALID_SOCKET)
        , last_update_time_(0)
        , last_errno_(0)
        , event_mask_(READ_EVENT_MASK)
        , state_(STATE_CLOSED)
        , in_event_loop_(false)
        , in_object_pool_(true)
    {
    }

    virtual ~EventHandler()
    {
    }

    inline ZRSOCKET_SOCKET socket() const
    {
        return fd_;
    }

    inline ZRSOCKET_FD fd() const
    {
        return fd_;
    }

    inline EventSource * source() const
    {
        return source_;
    }

    inline EventLoop * event_loop() const
    {
        return event_loop_;
    }
    
    inline int8_t state() const
    {
        return state_;
    }

    inline int event_mask() const
    {
        return event_mask_;
    }

    void init(ZRSOCKET_SOCKET fd, EventSource *source, EventLoop *event_loop, HANDLER_STATE state)
    {
        fd_ = fd;
        source_ = source;
        last_errno_ = 0;
        state_ = state;
#ifndef ZRSOCKET_HAVE_ACCEPT4
        OSApi::socket_set_block(fd, false);
#endif
    }

    virtual int handle_open()
    {
        return 0;
    }

    virtual int handle_close()
    {
        close();
        return 0;
    }

    virtual int handle_read()
    { 
        return 0;
    }

    virtual int handle_write()
    {
        return 0;
    }

    virtual int handle_connect()
    {
        return 0;
    }

    virtual void close()
    {
        if (ZRSOCKET_INVALID_SOCKET != fd_) {
            OSApi::socket_close(fd_);
            fd_ = ZRSOCKET_INVALID_SOCKET;
            state_ = STATE_CLOSED;
        }
    }

protected:
    EventSource    *source_;
    EventLoop      *event_loop_;
    ZRSOCKET_FD     fd_;

    uint64_t        last_update_time_;
    int             last_errno_;        //最后错误号

private:
    int             event_mask_;        //事件码(上层不能修改)
    int8_t          state_;             //当前状态

protected:
    bool            in_event_loop_;     //是否加入event_loop_中(上层不能修改)

private:
    bool            in_object_pool_;    //是否在object_pool中(上层不能修改)

    template <class TMutex, class TLoopData, class TEventTypeHandler> friend class SelectEventLoop;
    template <class TMutex, class TLoopData, class TEventTypeHandler> friend class EpollEventLoop;
    template <class TMutex, class TLoopData, class TEventTypeHandler> friend class EpollETEventLoop;
    template <class TEventLoop> friend class EventLoopGroup;
    template <class TClientHandler, class TObjectPool, class TServerHandler> friend class TcpServer;
    template <class TUdpSourceHandler> friend class UdpSource;
    template <class THandler> friend class TcpClient;
};

ZRSOCKET_NAMESPACE_END

#endif
