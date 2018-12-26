#include "client.h"
#include <stdio.h>
#include <vector>

int ClientHandler::handle_message(const char *message, uint_t len)
{
    Application *app = Application::instance();
    ++app->recv_count_;
    if (app->recv_count_ == app->send_count_) {
        printf("client recv_count:%ld tacktime:%lld ms\n", 
            app->recv_count_, 
            SystemApi::util_gettickcount() - app->send_start_time_);
    }
    return 0;
}

int main(int argc, char* argv[])
{
    if (argc < 5) {
        printf("please use the format: server_ip port connect_count send_count (e.g.: 127.0.0.1 7711 100 50000)\n");
        return 0;
    }

    printf("zrsocket version:%s\n", ZRSOCKET_VERSION_STR);
    zrsocket::SystemApi::socket_init(2, 2);

    Application *app    = Application::instance();
    app->server_        = argv[1];
    app->port_          = atoi(argv[2]);
    app->connect_count_ = atoi(argv[3]);
    app->send_count_    = atoi(argv[4]);
    app->run();
    Application::fini();

    return 0;
}
