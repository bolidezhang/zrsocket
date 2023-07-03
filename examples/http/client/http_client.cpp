#include <cstdio>
#include "http_client.h"
int ClientHttpHandler::do_message()
{
    ClientHttpApp &app = ClientHttpApp::instance();
    app.scc_.update_end_counter();
    printf("http client request tack_time:%lld ns\n", app.scc_.diff());
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

int HttpAppTimer::handle_timeout()
{
    ClientHttpApp &app = ClientHttpApp::instance();

    app.scc_.update_start_counter();
    // printf("id:%llu, current_timestamp:%llu, recv_messge_count:%llu\n", 
    //     data_.id, 
    //     app.scc_.get_start_counter(), 
    //     ClientHttpApp::instance().recv_messge_count_.load(std::memory_order_relaxed));

    HttpRequest req;
    req.method_id_ = HttpMethodId::kGET;
    req.uri_ = "/";
    //req.headers_.emplace("Connection", "keep-alive");
    req.headers_.emplace("Host", app.server_.c_str());
    req.headers_.emplace("User-Agent", "curl/7.58.0");
    req.headers_.emplace("Accept", "*/*");
    ByteBuffer out;
    out.reserve(128);
    req.encode(out);
    ClientHttpApp::instance().http_client_.handler()->send(out);
    //ClientHttpApp::instance().http_client_.handler()->send(out);
    //ClientHttpApp::instance().http_client_.handler()->send(out);
    //ClientHttpApp::instance().http_client_.handler()->send(out);
    //ClientHttpApp::instance().http_client_.handler()->send(out);

    timer_type_ = Timer::TimerType::ONCE;
    return 0;
}

int ClientHttpApp::init()
{
    main_event_loop_.open(1024, 1024);
    recv_messge_count_.store(0, std::memory_order_relaxed);

    decoder_config_.max_body_length_ = 4096;
    decoder_config_.max_header_length_ = 4096;
    decoder_config_.update();
    http_client_.set_config(true, false, 4096);
    http_client_.set_interface(&main_event_loop_, &decoder_config_);
    http_client_.open(server_.c_str(), port_);

    timer1_.interval(1000000);
    timer1_.data_.id = 1;
    main_event_loop_.add_timer(&timer1_);

    init_flag_ = true;
    return 0;
}

int main(int argc, char *argv[])
{
    ClientHttpApp &app = ClientHttpApp::instance();
    if (argc > 2) {
        app.server_ = argv[1];
        app.port_   = atoi(argv[2]);
    }

    printf("zrsocket version:%s\n", ZRSOCKET_VERSION_STR);
    printf("please use the format: server port (e.g.: 127.0.0.1 8080)\n");
    printf("http client connect: server:%s port:%d\n", app.server_.c_str(), app.port_);
    return app.run();
}
