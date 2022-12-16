#ifndef HTTP_CLIENT_H
#define HTTP_CLIENT_H
#include "zrsocket/zrsocket.h"

using namespace zrsocket;
class ClientHttpHandler : public HttpResponseHandler<ByteBuffer, NullMutex>
{
public:
    int do_open()
    {
        //printf("tcp do_open:%d, recv_buffer_size:%d, send_buffer_size:%d\n",
        //    fd_,
        //    OSApi::socket_get_recv_buffer_size(fd_),
        //    OSApi::socket_get_send_buffer_size(fd_));

        printf("http tcp do_open:%d\n", fd_);
        return 0;
    }

    int do_close()
    {
        printf("http tcp do_close:%d\n", fd_);
        return 0;
    }

    int do_message()
    {
        printf("http version:%d, status_code:%d desc:%s\n", 
            response_.version_id_, response_.status_code_, response_.reason_phrase_.c_str());
        for (auto &iter : response_.headers_) {
            printf("%s:%s\n", iter.first.c_str(), iter.second.c_str());
        }
        //printf("body:%s\n", response_.body_ptr_);
        
        //std::string body;
        //body.append(response_.body_ptr_, response_.content_length_);
        //printf("body:%s\n", body.c_str());

        return 0;
    }

};

class HttpAppTimer : public zrsocket::Timer
{
public:
    int handle_timeout();
};

class ClientHttpApp : public Application<ClientHttpApp, SpinMutex>
{
public:
    int init();

    int do_fini()
    {
        http_client_.close();
        printf("do_fini");
        return 0;
    }

    int do_signal(int signum)
    {
        printf("do_signal signum:%d\n", signum);
        return 0;
    }

public:
    typedef zrsocket_default_event_loop<zrsocket::SpinMutex> BASE_EVENT_LOOP;
    zrsocket::TcpClient<ClientHttpHandler> http_client_;

    HttpAppTimer       timer1_;
    std::string        server_;
    zrsocket::ushort_t port_ = 8080;
    AtomicUInt64       recv_messge_count_;
    HttpDecoderConfig  decoder_config_;
};

#endif
