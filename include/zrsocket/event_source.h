#ifndef ZRSOCKET_EVENT_SOURCE_H_
#define ZRSOCKET_EVENT_SOURCE_H_
#include "config.h"
#include "base_type.h"
#include "atomic.h"
#include "inet_addr.h"
#include "event_handler.h"
#include "event_reactor.h"

ZRSOCKET_BEGIN

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
        : get_msglen_proc_(get_msglen_int16_host)
        , set_msglen_proc_(set_msglen_int16_host)
        , handler_(NULL)
        , reactor_(NULL)
        , recvbuffer_size_(4096)
        , msgbuffer_size_(4096)
        , msglen_bytes_(2)
        , msg_byteorder_(0)
    {
    }

    virtual ~EventSource()
    {
    }

    virtual InetAddr* get_local_addr()
    {
        return NULL;
    }

    inline EventHandler* get_handler()
    {
        return handler_;
    }

    inline EventReactor* get_reactor()
    {
        return reactor_;
    }

    inline uint_t get_recvbuffer_size() const
    {
        return recvbuffer_size_;
    }

    inline uint_t get_msgbuffer_size() const
    {
        return msgbuffer_size_;
    }

    inline uint8_t get_msglen_bytes() const
    {
        return msglen_bytes_;
    }

    inline uint8_t get_msg_byteorder() const
    {
        return msg_byteorder_;
    }

    static int get_msglen_int8(const char *msg, int len)
    {
        return *msg;
    }

    static int get_msglen_int16_host(const char *msg, int len)
    {
        return  *((int16_t *)msg);
    }

    static int get_msglen_int16_network(const char *msg, int len)
    {
        return ntohs(*((int16_t *)msg));
    }

    static int get_msglen_int32_host(const char *msg, int len)
    {
         return  *((int32_t *)msg);
    }

    static int get_msglen_int32_network(const char *msg, int len)
    {
        return ntohl(*((int32_t *)msg));
    }

    static void set_msglen_int8(char *msg, int len)
    {
        *msg = len;
    }

    static void set_msglen_int16_host(char *msg, int len)
    {
        *((int16_t *)msg) = len;
    }

    static void set_msglen_int16_network(char *msg, int len)
    {
        *((int16_t *)msg) = htons(len);
    }

    static void set_msglen_int32_host(char *msg, int len)
    {
        *((int32_t *)msg) = len;
    }

    static void set_msglen_int32_network(char *msg, int len)
    {
        *((int32_t *)msg) = htonl(len);
    }

    static uint64_t generate_id()
    {
        static AtomicUInt64 id(0);
        return ++id;
    }

public:
    virtual EventHandler* alloc_handler()
    {
        return NULL;
    }

    virtual int free_handler(EventHandler *handler)
    {
        return 0;
    }

    virtual SOURCE_TYPE source_type()
    {
        return TYPE_UNKNOW;
    }

    virtual SOURCE_STATE source_state()
    {
        return STATE_CLOSED;
    }

    virtual void source_state(SOURCE_STATE state)
    {
    }

    virtual int connect()
    {
        return -1;
    }

    typedef int (* get_msglen_proc)(const char *msg, int len);
    typedef void (* set_msglen_proc)(char *msg, int len);

    get_msglen_proc     get_msglen_proc_;       //取得消息长度函数指针
    set_msglen_proc     set_msglen_proc_;       //设置消息长度函数指针

protected:
    EventHandler *handler_;
    EventReactor *reactor_;

    uint_t  recvbuffer_size_;
    uint_t  msgbuffer_size_;
    uint8_t msglen_bytes_;          //消息长度域所占字节数(一般为1,2,4)
    uint8_t msg_byteorder_;         //消息的字节顺序(0:host-byte,1:big endian(network-byte) 2:little endian)

};

ZRSOCKET_END

#endif
