// Some compilers (e.g. VC++) benefit significantly from using this. 
// We've measured 3-4% build speed improvements in apps as a result 
#pragma once

#ifndef ZRSOCKET_TCPSERVER_H
#define ZRSOCKET_TCPSERVER_H
#include "config.h"
#include "object_pool.h"
#include "thread.h"
#include "os_api.h"
#include "inet_addr.h"
#include "event_handler.h"
#include "event_loop.h"
#include "event_source.h"
#include "tcpserver_handler.h"

ZRSOCKET_NAMESPACE_BEGIN

template <class TClientHandler, class TObjectPool, class TServerHandler = TcpServerHandler>
class TcpServer : public EventSource
{
public:
    TcpServer()
        : accept_event_loop_(nullptr)
        , object_pool_(nullptr)
    {
        set_config();
    }

    virtual ~TcpServer()
    { 
        close();
    }

    int open(ushort_t local_port, int backlog = 1024, const char *local_name = nullptr, bool is_ipv6 = false)
    {
        close();

        int ret = local_addr_.set(local_name, local_port, is_ipv6);
        if (ret < 0) {
            return -1;
        }
        int address_family = is_ipv6 ? AF_INET6:AF_INET;
        ZRSOCKET_SOCKET fd = OSApi::socket_open(address_family, SOCK_STREAM, IPPROTO_TCP);
        if (ZRSOCKET_INVALID_SOCKET == fd) {
            return -2;
        }

        server_handler_.init(fd, this, accept_event_loop_, EventHandler::STATE_CONNECTED);
        if (server_handler_.handle_open() < 0) {
            ret = -3;
            goto EXCEPTION_EXIT_PROC;
        }

        OSApi::socket_set_reuseaddr(fd, 1);
        OSApi::socket_set_reuseport(fd, 1);
        ret = OSApi::socket_bind(fd, local_addr_.get_addr(), local_addr_.get_addr_size());
        if (ret < 0) {
            ret = -4;
            goto EXCEPTION_EXIT_PROC;
        }
        ret = OSApi::socket_listen(fd, backlog);
        if (ret < 0) {
            ret = -5;
            goto EXCEPTION_EXIT_PROC;
        }
        
        if (nullptr != accept_event_loop_) {
            OSApi::socket_set_block(fd, false);
            if (accept_event_loop_->add_handler(&server_handler_, EventHandler::READ_EVENT_MASK) < 0) {
                ret = -6;
                goto EXCEPTION_EXIT_PROC;
            }
        }
        else {
            if (accept_thread_num_ > 0) {
                OSApi::socket_set_block(fd, true);
                if (accept_thread_group_.start(accept_thread_num_, accept_thread_num_, accept_proc, this) < 0) {
                    ret = -7;
                    goto EXCEPTION_EXIT_PROC;
                }
            }
            else {
                if (event_loop_->add_handler(&server_handler_, EventHandler::READ_EVENT_MASK) < 0) {
                    ret = -8;
                    goto EXCEPTION_EXIT_PROC;
                }
            }
        }
        handler_ = &server_handler_;
        return 0;

 EXCEPTION_EXIT_PROC:
        OSApi::socket_close(fd);
        return ret;
    }

    int close()
    {
        accept_thread_group_.clear();
        if (nullptr != handler_) {
            if (nullptr != accept_event_loop_) {
                accept_event_loop_->delete_handler(&server_handler_, 0);
            }
            else {
                server_handler_.handle_close();
            }
            server_handler_.close();
            handler_ = nullptr;
        }
        return 0;
    }

    int set_config(uint_t   accept_thread_num = 1,
                   uint_t   max_connect_num   = 100000,
                   uint_t   recvbuffer_size   = 4096)
    {
        accept_thread_num_  = accept_thread_num;
        max_connect_num_    = max_connect_num;
        recvbuffer_size_    = recvbuffer_size;
        return 0;
    }
    
    inline int set_interface(TObjectPool *object_pool, 
        EventLoop *event_loop, 
        MessageDecoderConfig *message_decoder_config, 
        EventLoop *accept_event_loop = nullptr)
    {
        object_pool_            = object_pool;
        event_loop_             = event_loop;
        message_decoder_config_ = message_decoder_config;
        accept_event_loop_      = accept_event_loop;
        return 0;
    }

    TServerHandler * handler()
    {
        return &server_handler_;
    }

    InetAddr * local_addr()
    {
        return &local_addr_;
    }

    EventSource::SOURCE_TYPE source_type()
    {
        return EventSource::TYPE_SERVER;
    }

private:
    EventHandler * alloc_handler()
    {
        EventHandler *handler = object_pool_->pop();
        if (nullptr != handler) {
            handler->in_object_pool_ = false;
            return handler;
        }
        return nullptr;
    }

    int free_handler(EventHandler *handler)
    {
        if (handler != nullptr) {
            if ((handler != &server_handler_) && (!handler->in_object_pool_)) {
                handler->in_object_pool_ = true;
                object_pool_->push((TClientHandler *)handler);
                return 1;
            }
            return 0;
        }
        return -1;
    }

    static int accept_proc(uint_t thread_index, void *arg)
    {
        TcpServer *tcpserver = static_cast<TcpServer *>(arg);
        InetAddr iaddr(tcpserver->local_addr()->is_ipv6());
        sockaddr *addr  = iaddr.get_addr();     
        int addrlen     = iaddr.get_addr_size();

        ZRSOCKET_SOCKET client_fd;
        TServerHandler &server_handler = tcpserver->server_handler_;
        ZRSOCKET_SOCKET listen_fd = server_handler.socket();
        Thread *thread = tcpserver->accept_thread_group_.get_thread(thread_index);

        while (thread->state() == Thread::State::RUNNING) {
            client_fd = OSApi::socket_accept(listen_fd, addr, &addrlen, ZRSOCKET_SOCK_NONBLOCK);
            if (ZRSOCKET_INVALID_SOCKET != client_fd) {
                if (server_handler.handle_accept(client_fd, iaddr) < 0) {
                    OSApi::socket_close(client_fd);
                }
            }
            else {
                //关闭TcpServer::socket_,进而促使accept线程退出
                break;
            }
        }
        return 0;
    }

protected:
    InetAddr        local_addr_;
    uint_t          max_connect_num_;
    uint_t          accept_thread_num_;
    ThreadGroup     accept_thread_group_;
    EventLoop      *accept_event_loop_;
    TObjectPool    *object_pool_;
    TServerHandler  server_handler_;
};

ZRSOCKET_NAMESPACE_END

#endif
