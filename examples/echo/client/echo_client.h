#ifndef ECHO_CLIENT_H
#define ECHO_CLIENT_H
#include "zrsocket/zrsocket.h"
#include "message.h"

using namespace zrsocket;
//class ClientTcpHandler : public MessageHandler<ByteBuffer, SpinMutex>
//class ClientTcpHandler : public MessageHandler<SharedBuffer, SpinMutex>
//class ClientTcpHandler : public FixedLengthMessageHandler<ByteBuffer, SpinMutex>
class ClientTcpHandler : public LengthFieldMessageHandler<ByteBuffer, SpinMutex>
{
public:
    ClientTcpHandler()
    {
    }

    virtual ~ClientTcpHandler()
    {
    }

    int do_open()
    {
        //printf("client tcp do_open:%d, recv_buffer_size:%d, send_buffer_size:%d\n",
        //    fd_,
        //    OSApi::socket_get_recv_buffer_size(fd_),
        //    OSApi::socket_get_send_buffer_size(fd_));

        printf("client tcp do_open:%d\n", fd_);
        return 0;
    }

    int do_close()
    {
        printf("client do_close:%d\n", fd_);
        return 0;
    }

    int do_connect()
    {
        printf("client do_connect:%d\n", fd_);
        return 0;
    }

    int do_message(const char *message, uint_t len);
};


class ClientUdpHandler : public UdpSourceHandler<ByteBuffer, NullMutex>
{
public:
    int handle_open()
    {
        printf("udp handle_open:%d\n", fd_);
        return 0;
    }

    int handle_close()
    {
        printf("udp handle_close:%d\n", fd_);
        return 0;
    }

    int handle_read(const char* data, int len, bool is_alloc_buffer, InetAddr& from_addr)
    {
        char data_end = *(data + len);
        *((char*)data + len) = '\0';
        printf("udp handle_read data:%s, len:%d\n", data, len);
        *((char*)data + len) = data_end;

        //send(data, len, from_addr, false);
        //send(data, len, from_addr, false);
        //send("123", 3, from_addr, false);

        return 0;
    }
};

class TestClientApp : public Application<TestClientApp, SpinMutex>
{
public:
    int init();

    int do_fini()
    {
        for (auto &client : tcp_clients_) {
            client->close();
            delete client;
        }
        udp_source_.close();
        printf("do_fini\n");
        return 0;
    }

    int do_signal(int signum)
    {
        printf("do_signal signum:%d\n", signum);
        return 0;
    }

    static int send_proc(void *arg);

public:
    typedef zrsocket::TcpClient<ClientTcpHandler> Client;
    std::vector<Client *> tcp_clients_;

    std::string         server_;
    zrsocket::ushort_t  port_;
    zrsocket::uint_t    connect_count_;
    zrsocket::uint_t    send_count_;
    zrsocket::Thread    send_thread_;
    SteadyClockCounter  scc_;
    zrsocket::uint_t    recv_count_;
    zrsocket::uint_t    message_size_ = 12;

    //FixedLengthMessageDecoderConfig decoder_config_;
    LengthFieldMessageDecoderConfig decoder_config_;

    zrsocket::UdpSource<ClientUdpHandler> udp_source_;

    bool send_flag_ = false;    
};

#endif
