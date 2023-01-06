#include <stdio.h>
#include <vector>
#include "echo_server.h"

int TcpHandler::do_message(const char *message, uint_t len)
{
    TestAppServer::instance().recv_messge_count_.fetch_add(1, std::memory_order_relaxed);
    Message *msg = (Message *)message;
    msg->head.type = MessageType::MT_ECHO_RES;
    send(message, len, false);
    return 0;
}

int TestTimer::handle_timeout()
{
    auto now = Time::instance().current_timestamp_ms();

    printf("timer id:%llu, current_timestamp:%llu, recv_messge_count:%llu\n", 
        data_.id, 
        now, 
        TestAppServer::instance().recv_messge_count_.load(std::memory_order_relaxed));

    ////
    //TestAppServer &app = TestAppServer::instance();
    //zrsocket::InetAddr remote_addr;
    //remote_addr.set("192.168.11.150", 6000);
    //app.udp_source_.send("0123456789", 10, remote_addr);

    return 0;
}

int TestAppServer::init()
{
    decoder_config_.length_field_offset_ = 0;
    decoder_config_.length_field_length_ = 2;
    decoder_config_.max_message_length_ = 16384;
    //decoder_config_.max_message_length_ = 4096;
    decoder_config_.update();
    main_event_loop_.buffer_size();
    main_event_loop_.open(1024, 1024, -1, 0);
    recv_messge_count_.store(0, std::memory_order_relaxed);
    sub_event_loop_.init(std::thread::hardware_concurrency(), 10000, 1, 1000, 64);
    sub_event_loop_.open(1024, 1024, -1);
    sub_event_loop_.loop_thread_start(-1);

    handler_object_pool_.init(10000, 100, 10);
    tcp_server_.set_config(std::thread::hardware_concurrency(), 1000000, 4096);
    tcp_server_.set_interface(&handler_object_pool_, &sub_event_loop_, &decoder_config_, &main_event_loop_);
    //tcp_server_.set_interface(&handler_object_pool_, &main_event_loop_, &decoder_config_, &main_event_loop_);
    tcp_server_.open(port_, 1024, nullptr);

    udp_source_.set_config();
    udp_source_.set_interface(&main_event_loop_);
    udp_source_.open(port_);
    //InetAddr dest_addr;
    //udp_source_.send("1", 1, dest_addr);

    timer1_.interval(1000000);
    timer1_.data_.id = 1;
    //main_event_loop_.add_timer(&timer1_);

    /*
    timer2_.interval(100000);
    timer2_.data_.id = 2;
    main_event_loop_.add_timer(&timer2_);

    timer3_.interval(10000);
    timer3_.data_.id = 3;
    main_event_loop_.add_timer(&timer3_);

    timer4_.interval(20000);
    timer4_.data_.id = 4;
    main_event_loop_.add_timer(&timer4_);
    */

    //stage_.open(1, 100000, 64);
    //stage2_.open(1, 100000, 64);
    init_flag_ = true;

    return 0;
}

int main(int argc, char *argv[])
{
#define TEST 0
#if TEST
    {
        //std::stable_sort()
        //std::vector<int> v1;
        //v1.erase()
        //std::sort()
        //std::for_each()
        //signal()
        //select()
    }
#endif

    TestAppServer &app = TestAppServer::instance();
    if (argc > 1) {
        app.port_ = atoi(argv[1]);
    }

    printf("zrsocket version:%s\n", ZRSOCKET_VERSION_STR);
    printf("please use the format: port (e.g.: 6611)\n");
    printf("server port:%d\n", app.port_);
    return app.run();
}
