#ifndef ECHO_SERVER_H
#define ECHO_SERVER_H
#include "zrsocket/zrsocket.h"
#include "message.h"

using namespace zrsocket;
//class TcpHandler : public MessageHandler<ByteBuffer, NullMutex>
//class TcpHandler : public FixedLengthMessageHandler<ByteBuffer, NullMutex>
class TcpHandler : public LengthFieldMessageHandler<ByteBuffer, NullMutex>
{
public:
    int do_open()
    {
        //printf("tcp do_open:%d, recv_buffer_size:%d, send_buffer_size:%d\n",
        //    socket_,
        //    OSApi::socket_get_recv_buffer_size(socket_),
        //    OSApi::socket_get_send_buffer_size(socket_));

        printf("tcp do_open:%d\n", socket_);
        return 0;
    }

    int do_close()
    {
        printf("tcp do_close:%d\n", socket_);
        return 0;
    }

    int do_message(const char *message, uint_t len);
};

class LoopData
{
public:
    int a = 1;
    int b = 2;
};

class UdpHandler : public UdpSourceHandler<ByteBuffer, NullMutex>
{
public:
    int handle_open()
    {
        printf("udp handle_open:%d\n", socket_);
        return 0;
    }

    int handle_close()
    {
        printf("udp handle_close:%d\n", socket_);
        return 0;
    }

    int handle_read(const char *data, int len, bool is_alloc_buffer, InetAddr &from_addr)
    {
        char data_end = *(data + len);
        *((char *)data + len) = '\0';
        printf("udp handle_read data:%s, len:%d\n", data, len);
        *((char *)data + len) = data_end;
        send(data, len, from_addr, true);
        send(data, len, from_addr, true);
        send("123", 3, from_addr, true);

        if (nullptr != event_loop_) {
            LoopData *loop_data = static_cast<LoopData *>(event_loop_->get_loop_data());
            loop_data->a = 999;
            loop_data->b = 666;
        }

        std::list<InetAddr> to_addrs;
        to_addrs.push_back(from_addr);
        to_addrs.push_back(from_addr);
        std::vector<ZRSOCKET_IOVEC> iovecs(2);
        iovecs[0].iov_base = (char *)data;
        iovecs[0].iov_len  = len;
        iovecs[1].iov_base = (char *)data;
        iovecs[1].iov_len  = len;

        int ret = 0;
        ret = send(iovecs.data(), iovecs.size(), from_addr, true, 0, 0);
        printf("sendv1 ret:%d\n", ret);
        //ret = send(iovecs.data(), iovecs.size(), from_addr, false, 0, 0);
        //printf("sendv2 ret:%d\n", ret);

        ret = send(iovecs.data(), iovecs.size(), to_addrs.begin(), to_addrs.end(), [](InetAddr &addr){return &addr;}, true, 0, 0);
        printf("sendvv1 ret:%d\n", ret);
        //ret = send(iovecs.data(), iovecs.size(), to_addrs.begin(), to_addrs.end(), [](InetAddr &addr){return &addr;}, false, 0, 0);
        //printf("sendvv2 ret:%d\n", ret);

        return 0;
    }

};

class BizStageHandler : public zrsocket::SedaStageHandler
{
public:
    BizStageHandler()
    {
    }

    ~BizStageHandler()
    {
    }

    int handle_open()
    {
        SedaStageThreadLRUSlotInfo slot[1];
        slot[TIMER_STAT].capacity = 10;
        slot[TIMER_STAT].interval_ms = 100;
        stage_thread_->enable_lru_timers(slot, 1);
        //stage_thread_->set_lru_timer(TIMER_STAT, 0);
        return 0;
    }

    int handle_event(const SedaEvent *event)
    {
        switch (event->type()) {
            case SedaEventType::TIMER_EXPIRE: 
            {
                stage_thread_->set_lru_timer(TIMER_STAT, 0);

                printf("current_timestamp:%lld\n", OSApi::timestamp_ms());                
            }
            break;
        default:
            break;
        }
        return 0;
    }

    int handle_close()
    {
        return 0;
    }

    static const int TIMER_STAT = 0;
};

class TestTimer : public zrsocket::Timer
{
public:
    int handle_timeout();
};

class TestAppServer : public Application<TestAppServer, SpinMutex>
{
public:
    int init();

    int do_fini()
    {
        tcp_server_.close();
        udp_source_.close();
        stage_.close();
        stage2_.close();
        sub_event_loop_.close();
        printf("do_fini\n");
        return 0;
    }

    int do_signal(int signum)
    {
        udp_source_.close();
        sub_event_loop_.loop_thread_stop();
        printf("do_signal signum:%d\n", signum);
        return 0;
    }

public:
    typedef zrsocket_default_event_loop<zrsocket::SpinMutex, LoopData> BASE_EVENT_LOOP;
    typedef zrsocket::ZRSocketObjectPool<TcpHandler, zrsocket::SpinMutex> TCP_OBJECT_POOL;

    TCP_OBJECT_POOL handler_object_pool_;
    zrsocket::TcpServer<TcpHandler, TCP_OBJECT_POOL, zrsocket::TcpServerHandler> tcp_server_;
    zrsocket::UdpSource<UdpHandler> udp_source_;
    EventLoopGroup<BASE_EVENT_LOOP> sub_event_loop_;

    TestTimer timer1_;
    TestTimer timer2_;
    TestTimer timer3_;
    TestTimer timer4_;

    zrsocket::ushort_t port_ = 6611;
    zrsocket::ushort_t event_loop_num_ = 2;
    AtomicUInt64       recv_messge_count_;

    //FixedLengthMessageDecoderConfig decoder_config_;
    LengthFieldMessageDecoderConfig decoder_config_;

    //stage
    SedaStage<BizStageHandler>  stage_;

    //stage2
    SedaStage2<BizStageHandler> stage2_;
};

#endif
