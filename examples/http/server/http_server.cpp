#include <stdio.h>
#include <iostream>
#include <memory>
#include "http_server.h"

int ServerHttpHandler::do_message()
{
    ServerHttpApp::instance().recv_messge_count_.fetch_add(1, std::memory_order_relaxed);

    //printf("http method:%d, uri:%s, version:%d\n", context_.request_.method_id_, context_.request_ .uri_.c_str(), context_.request_.version_id_);
    //for (auto &iter : context_.request_.headers_) {
    //    printf("%s:%s\n", iter.first.c_str(), iter.second.c_str());
    //}
    //context_.request_.body_.write('\0');
    //printf("body:%s\n", context_.request_.body_.data());

    //context_.response_.version_id_ = HttpVersionId::kHTTP11;
    //context_.response_.status_code_ = HttpStatusCode::k200;
    //context_.response_.headers_.emplace("Content-Type", "text/plain");
    //context_.response_.headers_.emplace("Server", "zrsocket");
    //context_.response_.headers_.emplace("Connection", "close");
    //context_.response_.body_.write("hello world!");
    //context_.response_.body_.write("hello world!", sizeof("hello world!") - 1);
    context_.response_.headers_.emplace("Connection", "keep-alive");

    if (context_.request_.uri_ == "/hello") {
        context_.response_.body_.write("hello world!\n");
    }
    else {
        context_.response_.body_.write("uri is invalid!\n");
    }

    //ByteBuffer out;
    //context_.response_.encode(out);
    //context_.response_.send<EchoHttpHandler, ByteBuffer>(out);

    return 0;
}

int HttpAppTimer::handle_timeout()
{
    auto now = Time::instance().current_timestamp_us();
    printf("id:%llu, current_timestamp:%llu, recv_messge_count:%llu\n", 
        data_.id, 
        now, 
        ServerHttpApp::instance().recv_messge_count_.load(std::memory_order_relaxed));

    return 0;
}

int ServerHttpApp::init()
{
    main_event_loop_.open(1024, 1024);
    recv_messge_count_.store(0, std::memory_order_relaxed);
    sub_event_loop_.init(std::thread::hardware_concurrency(), 10000, 2);
    sub_event_loop_.open(1024, 1024);
    sub_event_loop_.loop_thread_start();

    handler_object_pool_.init(10000, 100, 10);
    decoder_config_.update();
    http_server_.set_config(std::thread::hardware_concurrency(), 1000000, 4096);
    http_server_.set_interface(&handler_object_pool_, &sub_event_loop_, &decoder_config_, &main_event_loop_);
    http_server_.open(port_, 1024, nullptr);

    timer1_.interval(1000000);
    timer1_.data_.id = 1;
    main_event_loop_.add_timer(&timer1_);

    //timer2_.interval(100000);
    //timer2_.data_.id = 2;
    //main_event_loop_.add_timer(&timer2_);
    //timer3_.interval(10000);
    //timer3_.data_.id = 3;
    //timer4_.interval(20000);
    //main_event_loop_.add_timer(&timer3_);
    //timer4_.data_.id = 4;
    //main_event_loop_.add_timer(&timer4_);

    init_flag_ = true;
    return 0;
}

int main(int argc, char *argv[])
{
#define TEST 0
#if defined(TEST) && (TEST != 0)
    {
        return 0;
    }
#endif

    ServerHttpApp &app = ServerHttpApp::instance();
    if (argc > 1) {
        app.port_ = atoi(argv[1]);
    }

    printf("zrsocket version:%s\n", ZRSOCKET_VERSION_STR);
    printf("please use the format: port (e.g.: 8080)\n");
    printf("http server port:%d\n", app.port_);
    return app.run();
}
