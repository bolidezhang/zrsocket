#include <cstdio>
#include <vector>
#include "echo_client.h"

int ClientTcpHandler::do_message(const char *message, uint_t len)
{
    TestClientApp &app = TestClientApp::instance();
    ++app.recv_count_;
    if (app.recv_count_ == app.send_count_) {

        app.scc_.update_end_counter();
        int64_t tack_time_ms = app.scc_.diff()/1000000ll;
        if (tack_time_ms <= 0) {
            tack_time_ms = 1;
        }
        int64_t tps = app.recv_count_ * 1000LL / tack_time_ms;
        double  teu = app.recv_count_ * 1000.0 * app.message_size_ / tack_time_ms /(1024 * 1024);
        printf("client connect_count:%u, recv_count:%u tack_time:%lld ns tps:%lld tps/s throughput:%0.3f MB/s\n",
            app.connect_count_,
            app.recv_count_, 
            app.scc_.diff(), 
            tps, 
            teu);
    }

    return 0;
}

int TestClientApp::init()
{
    main_event_loop_.open(1024, 1024);
    decoder_config_.length_field_offset_ = 0;
    decoder_config_.length_field_length_ = 2;
    decoder_config_.max_message_length_ = 16384;
    decoder_config_.update();
    tcp_clients_.reserve(connect_count_);
    for (zrsocket::uint_t i = 0; i < connect_count_; ++i) {
        Client *client = new Client();
        client->set_config(true, false, sizeof(Message));
        client->set_interface(&main_event_loop_, &decoder_config_);
        if (client->open(server_.c_str(), port_) < 0) {
            printf("connect server:%s port:%d failure!\n", server_.c_str(), port_);
            return -1;
        }
        tcp_clients_.emplace_back(client);
    }

    //udp setting
    udp_source_.set_config();
    udp_source_.set_interface(&main_event_loop_);
    udp_source_.open(0, nullptr, false);

    send_thread_.start(send_proc, this);
    init_flag_ = true;
    return 0;
}

int TestClientApp::send_proc(void *arg)
{
    TestClientApp *app = (TestClientApp *)arg;

    Message msg;
    msg.head.length = app->message_size_;
    msg.head.type = MessageType::MT_ECHO_REQ;

    zrsocket::uint_t client_send_count = app->send_count_ / app->connect_count_;
    app->scc_.update_start_counter();
    for (zrsocket::uint_t i = 0; i < app->connect_count_; ++i) {
        Client *client = app->tcp_clients_[i];
        for (zrsocket::uint_t j = 0; j < client_send_count; ++j) {
            msg.head.sequence = j;
            msg.head.source = i;
            client->handler()->send((const char*)&msg, msg.head.length, app->send_flag_);
        }
    }
    app->scc_.update_end_counter();
    printf("clients send_data tacktime:%lld ns\n", app->scc_.diff());

    /*
    //udp send
    zrsocket::InetAddr server_addr;
    server_addr.set(app->server_.c_str(), app->port_);
    std::string strudp = "test123456789";
    for (int i = 0; i < 10; ++i) {
        app->udp_source_.send(strudp.c_str(), strudp.length(), server_addr);
    }
    */

    return 0;
}

int main(int argc, char *argv[])
{
    if (argc < 7)
    {
        printf("please use the format: server_ip port connect_count send_count message_size send_flag(e.g.: 127.0.0.1 6611 1000 10000000 12 0)\n");
        return 0;
    }

    printf("zrsocket version:%s\n", ZRSOCKET_VERSION_STR);

    TestClientApp &app  = TestClientApp::instance();
    app.server_        = argv[1];
    app.port_          = atoi(argv[2]);
    app.connect_count_ = atoi(argv[3]);
    app.send_count_    = atoi(argv[4]);
    app.message_size_  = atoi(argv[5]);
    app.send_flag_     = atoi(argv[6]);

    return app.run();
}
