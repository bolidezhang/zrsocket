// Some compilers (e.g. VC++) benefit significantly from using this. 
// We've measured 3-4% build speed improvements in apps as a result 
#pragma once

#ifndef ZRSOCKET_TCPSERVER_HANDLER_H
#define ZRSOCKET_TCPSERVER_HANDLER_H
#include "os_api.h"
#include "inet_addr.h"
#include "event_handler.h"
#include "event_source.h"

ZRSOCKET_NAMESPACE_BEGIN

class TcpServerHandler : public EventHandler
{
public:
    TcpServerHandler()
    {
    }

    virtual ~TcpServerHandler()
    {
    }

    int handle_read()
    { 
        InetAddr addr(source_->local_addr()->is_ipv6());
        int addrlen = addr.get_addr_size();

        //for(;;)存在的目的, 因为linux epoll边缘触发模式(EPOLLET)
        //监听的句柄被设置为EPOLLET,当同时多个连接建立的时候,
        //若只accept出一个连接进行处理,这样就可能导致后来的连接不能被及时处理,要等到下一次连接才会被激活
        ZRSOCKET_SOCKET client_fd;
        EventHandler *handler;
        EventLoop *event_loop;
        for (;;) {
            client_fd = OSApi::socket_accept(socket_, addr.get_addr(), &addrlen, ZRSOCKET_SOCK_NONBLOCK);
            if (ZRSOCKET_INVALID_SOCKET != client_fd) {
                handler = source_->alloc_handler();
                if (nullptr != handler) {
                    event_loop = source_->event_loop();
                    handler->init(client_fd, source_, event_loop, EventHandler::STATE_CONNECTED);
                    if (handler->handle_open() >= 0) {
                        if (event_loop->add_handler(handler, EventHandler::READ_EVENT_MASK) < 0) {
                            source_->free_handler(handler);
                        }
                    }
                    else {
                        source_->free_handler(handler);
                    }
                }
                else {
                    OSApi::socket_close(client_fd);
                }
            }
            else {
                //非阻塞模式(NIO)下,退出此次激活事件处理
                break;
            }
        }

        return 0;
    }

    int handle_accept(ZRSOCKET_SOCKET client_fd, InetAddr &addr)
    {
        EventHandler *handler = source_->alloc_handler();
        if (nullptr != handler) {
            EventLoop *loop = source_->event_loop();
            handler->init(client_fd, source_, loop, EventHandler::STATE_CONNECTED);
            if (handler->handle_open() >= 0) {
                if (loop->add_handler(handler, EventHandler::READ_EVENT_MASK) < 0) {
                    source_->free_handler(handler);
                    return -1;
                }
            }
            else {
                source_->free_handler(handler);
                return -2;
            }
        }
        else {
            return -3;
        }

        return 0;
    }

    int handle_close()
    {
        return -1;
    }
};

ZRSOCKET_NAMESPACE_END

#endif
