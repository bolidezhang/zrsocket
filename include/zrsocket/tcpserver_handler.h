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

        //for(;;)���ڵ�Ŀ��, ��Ϊlinux epoll��Ե����ģʽ(EPOLLET)
        //�����ľ��������ΪEPOLLET,��ͬʱ������ӽ�����ʱ��,
        //��ֻaccept��һ�����ӽ��д���,�����Ϳ��ܵ��º��������Ӳ��ܱ���ʱ����,Ҫ�ȵ���һ�����ӲŻᱻ����
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
                //������ģʽ(NIO)��,�˳��˴μ����¼�����
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
