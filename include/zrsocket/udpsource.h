// Some compilers (e.g. VC++) benefit significantly from using this. 
// We've measured 3-4% build speed improvements in apps as a result 
#pragma once

#ifndef ZRSOCKET_UDPSOURCE_H
#define ZRSOCKET_UDPSOURCE_H

#include "config.h"
#include "event_source.h"
#include "inet_addr.h"
#include "thread.h"
#include "udpsource_handler.h"

ZRSOCKET_NAMESPACE_BEGIN

template <class TUdpSourceHandler>
class UdpSource : public EventSource
{
public:
    UdpSource()
    {
        set_config();
    }

    virtual ~UdpSource()
    {
        close();
    }

    TUdpSourceHandler * handler()
    {
        return &udpsource_handler_;
    }

    InetAddr * local_addr()
    {
        return &local_addr_;
    }

    int set_config(bool is_recv_thread = false, uint_t recvbuffer_size = 2048)
    {
        is_recv_thread_  = is_recv_thread;
        recvbuffer_size_ = recvbuffer_size;
        return 0;
    }

    int set_interface(EventLoop *loop)
    {
        event_loop_ = loop;
        return 0;
    }

    int open(ushort_t local_port = 0, 
        const char *local_name = nullptr, 
        bool is_bind_localaddr = true, 
        bool is_ipv6 = false, 
        int recv_thread_num = 1)
    {
        int ret = 0;
        int address_family = is_ipv6 ? AF_INET6:AF_INET;
        ZRSOCKET_SOCKET fd = OSApi::socket_open(address_family, SOCK_DGRAM, IPPROTO_UDP);
        if (ZRSOCKET_INVALID_SOCKET == fd) {
            return -1;
        }
        if (is_bind_localaddr) {
            ret = local_addr_.set(local_name, local_port, is_ipv6);
            if (ret < 0) {
                ret = -2;
                goto EXCEPTION_EXIT_PROC;
            }
            OSApi::socket_set_reuseaddr(fd, 1);
            OSApi::socket_set_reuseport(fd, 1);
            ret = OSApi::socket_bind(fd, local_addr_.get_addr(), local_addr_.get_addr_size());
            if (ret < 0) {
                ret = -3;
                goto EXCEPTION_EXIT_PROC;
            }
            udpsource_handler_.init(fd, this, event_loop_, EventHandler::STATE_OPENED);
            if (udpsource_handler_.handle_open() < 0) {
                ret = -4;
                goto EXCEPTION_EXIT_PROC;
            }
            if (is_recv_thread_) {
                OSApi::socket_set_block(fd, true);
                if (recv_thread_group_.start(recv_thread_num, recv_thread_num, recv_proc, this) < 0) {
                    ret = -5;
                    goto EXCEPTION_EXIT_PROC;
                }
                handler_ = &udpsource_handler_;
                return 0;
            }
        }
        else {
            udpsource_handler_.init(fd, this, event_loop_, EventHandler::STATE_OPENED);
            if (udpsource_handler_.handle_open() < 0) {
                ret = -7;
                goto EXCEPTION_EXIT_PROC;
            }
        }

        OSApi::socket_set_block(fd, false);
        if (nullptr != event_loop_) {
            is_recv_thread_ = false;
            if (event_loop_->add_handler(&udpsource_handler_, EventHandler::READ_EVENT_MASK) < 0) {
                ret = -8;
                goto EXCEPTION_EXIT_PROC;
            }
        }
        handler_ = &udpsource_handler_;
        return 0;

EXCEPTION_EXIT_PROC:
        udpsource_handler_.socket_ = ZRSOCKET_INVALID_SOCKET;
        OSApi::socket_close(fd);
        return ret;
    }

    int close()
    {
        if (nullptr != handler_) {
            if (is_recv_thread_) {
                recv_thread_group_.stop();
                udpsource_handler_.handle_close();
            }
            else {
                event_loop_->delete_handler(&udpsource_handler_, 0);
            }
            udpsource_handler_.close();
            handler_ = nullptr;
        }
        return 0;
    }

    int send(const char *buf, int len, InetAddr &to_addr)
    {
        return udpsource_handler_.send(buf, len, to_addr);
    }

private:
    static int recv_proc(uint_t thread_index, void *arg)
    {
        UdpSource *udpsource = static_cast<UdpSource *>(arg);
        TUdpSourceHandler *udpsource_handler = &(udpsource->udpsource_handler_);
        ZRSOCKET_SOCKET fd = udpsource->udpsource_handler_.socket();
        Thread *thread = udpsource->recv_thread_group_.get_thread(thread_index);

        InetAddr    from_addr(udpsource->local_addr()->is_ipv6());
        sockaddr   *addr    = from_addr.get_addr();
        int         addrlen = from_addr.get_addr_size();

        int         buf_size = udpsource->recvbuffer_size();
        ByteBuffer  source_buf(buf_size);

        int     ret     = 0;
        int     error   = 0;
        char   *buf     = nullptr;
        bool    is_alloc_buffer = false;

        while (thread->state() == Thread::State::RUNNING) {
            buf = udpsource_handler->alloc_buffer(buf_size);
            if (nullptr != buf) {
                is_alloc_buffer = true;
            }
            else {
                is_alloc_buffer = false;
                buf = source_buf.buffer();
            }
            ret = OSApi::socket_recvfrom(fd, buf, buf_size, 0, addr, &addrlen, nullptr, error);
            if (ret > 0) {
                udpsource_handler->handle_read(buf, ret, is_alloc_buffer, from_addr);
            }
        }
        return 0;
    }

protected:
    bool                is_recv_thread_ = false;
    ThreadGroup         recv_thread_group_;     //多线程接收数据,提高并发性
    InetAddr            local_addr_;
    TUdpSourceHandler   udpsource_handler_;
};

ZRSOCKET_NAMESPACE_END

#endif
