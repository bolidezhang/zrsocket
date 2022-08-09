#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H
#include "zrsocket/zrsocket.h"

using namespace zrsocket;
class ServerHttpHandler : public HttpRequestHandler<ByteBuffer, NullMutex>
{
public:
    int do_open()
    {
        //printf("tcp do_open:%d, recv_buffer_size:%d, send_buffer_size:%d\n",
        //    socket_,
        //    OSApi::socket_get_recv_buffer_size(socket_),
        //    OSApi::socket_get_send_buffer_size(socket_));

        printf("http tcp do_open:%d\n", socket_);
        return 0;
    }

    int do_close()
    {
        printf("http tcp do_close:%d\n", socket_);
        return 0;
    }

    int do_message();
};

class HttpAppTimer : public zrsocket::Timer
{
public:
    int handle_timeout();
};

class ServerHttpApp : public Application<ServerHttpApp, SpinMutex>
{
public:
    int init();

    int do_fini()
    {
        http_server_.close();
        printf("do_fini\n");
        return 0;
    }

    int do_signal(int signum)
    {
        printf("do_signal signum:%d\n", signum);
        return 0;
    }

public:
    typedef zrsocket_default_event_loop<zrsocket::SpinMutex> BASE_EVENT_LOOP;
    typedef zrsocket::ZRSocketObjectPool<ServerHttpHandler, zrsocket::SpinMutex> ECHO_HTTP_OBJECT_POOL;

    ECHO_HTTP_OBJECT_POOL handler_object_pool_;
    zrsocket::TcpServer<ServerHttpHandler, ECHO_HTTP_OBJECT_POOL, zrsocket::TcpServerHandler> http_server_;
    EventLoopGroup<BASE_EVENT_LOOP> sub_event_loop_;

    HttpAppTimer timer1_;
    HttpAppTimer timer2_;
    HttpAppTimer timer3_;
    HttpAppTimer timer4_;

    zrsocket::ushort_t port_ = 8080;
    zrsocket::ushort_t event_loops_num_ = 2;
    AtomicUInt64       recv_messge_count_;
    HttpDecoderConfig  decoder_config_;
};

#endif
