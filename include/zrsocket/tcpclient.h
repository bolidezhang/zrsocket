// Some compilers (e.g. VC++) benefit significantly from using this. 
// We've measured 3-4% build speed improvements in apps as a result 
#pragma once

#ifndef ZRSOCKET_TCPCLIENT_H
#define ZRSOCKET_TCPCLIENT_H
#include <string>
#include "config.h"
#include "base_type.h"
#include "byte_buffer.h"
#include "mutex.h"
#include "os_api.h"
#include "inet_addr.h"
#include "event_source.h"

ZRSOCKET_NAMESPACE_BEGIN

template <class THandler>
class TcpClient : public EventSource
{
public:
    TcpClient()
        : local_port_(0)
        , is_bind_localaddr_(false)
        , is_block_connect_(false)
        , auto_connected_(false)
        , source_state_(EventSource::STATE_CLOSED)
    {
        handler_ = &client_handler_;
        set_config();
    }

    virtual ~TcpClient()
    {
        close();
    }

    int open(const char *server_name, 
        ushort_t server_port, 
        const char *local_name = nullptr, 
        ushort_t local_port = 0, 
        bool is_bind_localaddr = false, 
        bool is_ipv6 = false)
    {
        server_name_ = server_name ? server_name : "";
        server_port_ = server_port;
        local_name_  = local_name ? local_name : "";
        local_port_  = local_port;

        is_bind_localaddr_ = is_bind_localaddr;
        server_addr_.set(server_name, server_port, is_ipv6);
        local_addr_.set(local_name, local_port, is_ipv6);

        return connect();
    }

    int close()
    {
        auto_connected_ = false;
        source_state_   = EventSource::STATE_CLOSED;
        event_loop_->remove_handler(&client_handler_, 0);
        client_handler_.close();
        return 0;
    }

    int set_config(bool     is_block_connect = false,
                   bool     auto_connected   = false,
                   uint_t   recvbuffer_size  = 4096)
    {
        is_block_connect_ = is_block_connect;
        auto_connected_   = auto_connected;
        recvbuffer_size_  = recvbuffer_size;
        return 0;
    }

    int set_interface(EventLoop *loop, MessageDecoderConfig *message_decoder_config)
    {
        event_loop_ = loop;
        message_decoder_config_ = message_decoder_config;
        return 0;
    }

    EventSource::SOURCE_STATE source_state()
    {
        return source_state_;
    }

    EventSource::SOURCE_TYPE source_type()
    {
        return EventSource::TYPE_CLIENT;
    }

    bool auto_connected()
    {
        return auto_connected_;
    }

    void auto_connected(bool auto_connected)
    {
        auto_connected_ = auto_connected;
    }

    THandler * handler()
    {
        return &client_handler_;
    }

private:
    int free_handler(EventHandler *handler)
    {
        source_state_ = EventSource::STATE_CLOSED;
        if (auto_connected_) {
            connect();
        }
        return 0;
    }

    void source_state(EventSource::SOURCE_STATE state)
    {
        source_state_ = state;
    }

    int connect()
    {
        if (source_state_ == EventSource::STATE_CLOSED) {
            source_state_ = EventSource::STATE_CONNECTING;
            int address_family = server_addr_.is_ipv6() ? AF_INET6 : AF_INET;
            ZRSOCKET_SOCKET fd = OSApi::socket_open(address_family, SOCK_STREAM, IPPROTO_TCP);
            if (ZRSOCKET_INVALID_SOCKET == fd) {
                return -1;
            }

            int ret = 0;
            if (is_bind_localaddr_) {
                OSApi::socket_set_reuseaddr(fd, 1);
                if (OSApi::socket_bind(fd, local_addr_.get_addr(), local_addr_.get_addr_size()) < 0) {
                    ret = -2;
                    goto ERROR_CONNECT;
                }
            }

            if (!is_block_connect_) {
                OSApi::socket_set_block(fd, false);
            }
            ret = OSApi::socket_connect(fd, server_addr_.get_addr(), server_addr_.get_addr_size());
            if (ret < 0) {
                int error = OSApi::socket_get_lasterror();
                if (error == ZRSOCKET_ECONNECTING) {
                    client_handler_.init(fd, this, event_loop_, EventHandler::STATE_OPENED);
                    if (client_handler_.handle_open() < 0) {
                        ret = -3;
                        goto ERROR_CONNECT;
                    }
                    if (event_loop_->add_handler(&client_handler_, EventHandler::WRITE_EVENT_MASK|EventHandler::EXCEPT_EVENT_MASK) < 0) {
                        ret = -4;
                        goto ERROR_CONNECT;
                    }
                }
                else {
                    ret = -5;
                    goto ERROR_CONNECT;
                }
            }
            else {
                source_state_ = EventSource::STATE_CONNECTED;
                client_handler_.init(fd, this, event_loop_, EventHandler::STATE_CONNECTED);
                if (client_handler_.handle_open() < 0) {
                    ret = -6;
                    goto ERROR_CONNECT;
                }
                if (event_loop_->add_handler(&client_handler_, EventHandler::READ_EVENT_MASK) < 0) {
                    ret = -7;
                    goto ERROR_CONNECT;
                }
            }

            return 0;

        ERROR_CONNECT:
            source_state_ = EventSource::STATE_CLOSED;
            OSApi::socket_close(fd);
            return ret;
        }

        return 0;
    }

protected:
    InetAddr        server_addr_;
    InetAddr        local_addr_;
    std::string     server_name_;
    std::string     local_name_;
    ushort_t        server_port_;
    ushort_t        local_port_;
    bool            is_bind_localaddr_;
    bool            is_block_connect_;

    volatile bool   auto_connected_;                    //是否自动重连
    volatile EventSource::SOURCE_STATE source_state_;   //连接状态

    THandler        client_handler_;
};

ZRSOCKET_NAMESPACE_END

#endif
