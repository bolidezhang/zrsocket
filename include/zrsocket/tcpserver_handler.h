#ifndef ZRSOCKET_TCPSERVER_HANDLER_H_
#define ZRSOCKET_TCPSERVER_HANDLER_H_
#include "system_api.h"
#include "inet_addr.h"
#include "event_handler.h"
#include "event_source.h"

ZRSOCKET_BEGIN

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
        InetAddr addr(source_->get_local_addr()->is_ipv6());
        int addrlen = addr.get_addr_size();

        //for(;;)存在的目的, 因为linux epoll边缘触发模式(EPOLLET)
        //监听的句柄被设置为EPOLLET,当同时多个连接建立的时候,
        //若只accept出一个连接进行处理,这样就可能导致后来的连接不能被及时处理,要等到下一次连接才会被激活
        ZRSOCKET_SOCKET fd;
        EventHandler *handler;
        EventReactor *reactor;
        for (;;) {
            fd = SystemApi::socket_accept(socket_, addr.get_addr(), &addrlen, ZRSOCKET_SOCK_NONBLOCK);
            if (ZRSOCKET_INVALID_SOCKET != fd) {
                handler = source_->alloc_handler();
                if (NULL != handler) {
                    reactor = source_->get_reactor();
                    handler->init(fd, source_, reactor, EventHandler::STATE_CONNECTED);
                    if (handler->handle_open() >= 0) {
                        if (reactor->add_handler(handler, EventHandler::READ_EVENT_MASK) < 0) {
                            source_->free_handler(handler);
                        }
                    }
                    else {
                        source_->free_handler(handler);
                    }
                }
                else {
                    SystemApi::socket_close(fd);
                }
            }
            else {
                //非阻塞模式(NIO)下,退出此次激活事件处理
                break;
            }
        }

        return 0;
    }

    int handle_accept(ZRSOCKET_SOCKET fd, InetAddr &addr)
    {
        EventHandler *handler = source_->alloc_handler();
        if (NULL != handler) {
            EventReactor *reactor = source_->get_reactor();
            handler->init(fd, source_, reactor, EventHandler::STATE_CONNECTED);
            if (handler->handle_open() >= 0) {
                if (reactor->add_handler(handler, EventHandler::READ_EVENT_MASK) < 0) {
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

ZRSOCKET_END

#endif
