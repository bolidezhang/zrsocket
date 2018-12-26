#include "server.h"
#include <stdio.h>

int main(int argc, char* argv[])
{    
    zrsocket::ushort_t port = 7711;
    zrsocket::ushort_t reactor_num = 2;

    if (argc > 1) {
        port = atoi(argv[1]);
    }
    if (argc > 2) {
        reactor_num = atoi(argv[2]);
    }

    printf("zrsocket version:%s\n", ZRSOCKET_VERSION_STR);
    printf("please use the format: port reactor_num (e.g.: 7711 2)\n");
    printf("server port:%d, reactor_num:%d\n", port, reactor_num);
    zrsocket::SystemApi::socket_init(2, 2);

    typedef zrsocket_default_reactor<zrsocket::SpinMutex> BASE_REACTOR;
    zrsocket::EventReactorGroup<BASE_REACTOR> reactor;
    typedef zrsocket::ZRSocketObjectPool<ClientHandler, zrsocket::SpinMutex> CLIENT_OBJECT_POOL;
    CLIENT_OBJECT_POOL handler_object_pool;
    zrsocket::TcpServer<ClientHandler, CLIENT_OBJECT_POOL, zrsocket::TcpServerHandler> tcp_server;

    handler_object_pool.init(100000, 100, 10);
    reactor.init(reactor_num);
    reactor.set_buffer_size(32768, 32768);
    reactor.open(1024, 4096, 10, 0);

    tcp_server.set_config(1);
    tcp_server.set_interface(&handler_object_pool, &reactor, NULL);
    tcp_server.open(port, 1024, NULL);

    reactor.thread_start(1);
    reactor.thread_join();

    return 0;
}
