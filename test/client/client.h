#ifndef TEST_CLIENT_H
#define TEST_CLIENT_H
#include "zrsocket/zrsocket.h"
#include "message.h"
using namespace zrsocket;
class Application;

class ClientHandler : public MessageHandler<ByteBuffer, SpinMutex>
{
public:
    ClientHandler()
    {
    }

    virtual ~ClientHandler()
    {
    }

    int handle_open()
    {
        printf("client handle_open:%d\n", socket_);
        return 0;
    }

    int handle_close()
    {
        printf("client handle_close:%d\n", socket_);
        return 0;
    }

    int handle_connect()
    {
        printf("client handle_connect:%d\n", socket_);
        return 0;
    }

    int handle_message(const char *message, uint_t len);
};

class Application
{
private:
    Application()
        : recv_count_(0)
    {
    }

    ~Application()
    {
    }

public:
    typedef zrsocket::TcpClient<ClientHandler> Client;
    std::vector<Client*> tcp_clients_;
    zrsocket_default_reactor<zrsocket::SpinMutex> reactor_;

    std::string         server_;
    zrsocket::ushort_t  port_;
    zrsocket::uint_t    connect_count_;
    zrsocket::uint_t    send_count_;
    zrsocket::Thread    send_thread_;
    uint64_t            send_start_time_;
    zrsocket::uint_t    recv_count_;

    int run()
    {
        reactor_.set_buffer_size(32768, 32768);
        reactor_.open(1024, 8192, 10, 0);
        reactor_.thread_start(1);

        tcp_clients_.reserve(connect_count_);
        for (zrsocket::uint_t i = 0; i < connect_count_; ++i) {
            Client *client = new Client();
            client->set_config(true, false);
            client->set_interface(&reactor_);
            if (client->open(server_.c_str(), port_) < 0) {
                printf("connect error server:%s port:%d\n", server_.c_str(), port_);
                reactor_.thread_stop();
                reactor_.thread_join();
                return 0;
            }
            tcp_clients_.emplace_back(client);
        }

        send_thread_.start(send_function, this);
        reactor_.thread_join();

        return 0;
    }

    static Application* instance()
    {
        static Application *app = NULL;
        if (NULL == app) {
            app = new Application();
        }
        return app;
    }

    static void fini()
    {
        Application *app = instance();
        if (NULL == app) {
            delete app;
        }
    }

    static int send_function(void *arg)
    {
        Application *app = (Application *)arg;
        MessageHead head;
        head.length = sizeof(MessageHead);
        head.type   = MessageType::MT_ECHO_REQ;
        zrsocket::uint_t client_send_count = app->send_count_ / app->connect_count_;
        app->send_start_time_ = SystemApi::util_gettickcount();
        for (zrsocket::uint_t i = 0; i<app->connect_count_; ++i) {
            Client *client = app->tcp_clients_[i];
            for (zrsocket::uint_t j = 0; j < client_send_count; ++j) {
                head.sequence = j;
                head.source   = i;
                client->get_handler()->push_message(client->get_handler()->handler_id(), (const char *)&head, head.length);
            }
        }
        printf("clients push_messsage tacktime:%lld ms\n", SystemApi::util_gettickcount() - app->send_start_time_);
        return 0;
    }
};

#endif
