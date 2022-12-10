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

    int set_config(bool is_recv_thread = false, uint_t recvbuffer_size = 2048, uint_t recv_msgs_count = 16, uint_t recv_timeout_us = 2000000)
    {
        is_recv_thread_  = is_recv_thread;
        recvbuffer_size_ = recvbuffer_size;
        recv_msgs_count_ = recv_msgs_count;
        recv_timeout_us_ = recv_timeout_us;
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
            if (nullptr == event_loop_) {
                is_recv_thread_ = true;
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
            recv_thread_group_.stop();
            if (is_recv_thread_) {
                udpsource_handler_.handle_close();
            }
            if (nullptr != event_loop_) {
                event_loop_->delete_handler(&udpsource_handler_, 0);
            }
            udpsource_handler_.close();
            recv_thread_group_.join();
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

#ifdef ZRSOCKET_HAVE_RECVSENDMMSG
        int max_udpmsg_size = udpsource_handler->get_max_udpmsg_size();
        int msgs_count = udpsource->recv_msgs_count_;
        ByteBuffer recv_buf(max_udpmsg_size * msgs_count);
        std::vector<struct mmsghdr> msgs(msgs_count);
        std::vector<ZRSOCKET_IOVEC> iovecs(msgs_count);
        std::vector<InetAddr> from_addrs(msgs_count);

        for (int i = 0; i < msgs_count; ++i) {
            iovecs[i].iov_base          = recv_buf.data() + max_udpmsg_size * i;
            iovecs[i].iov_len           = max_udpmsg_size;
            msgs[i].msg_hdr.msg_iov     = &iovecs[i];
            msgs[i].msg_hdr.msg_iovlen  = 1;
            msgs[i].msg_hdr.msg_name    = from_addrs[i].get_addr();
            msgs[i].msg_hdr.msg_namelen = from_addrs[i].get_addr_size();
            msgs[i].msg_hdr.msg_control = nullptr;
            msgs[i].msg_hdr.msg_controllen = 0;
            msgs[i].msg_hdr.msg_flags = 0;
        }

        int retval = 0;
        struct timespec timeout;
        timeout.tv_sec  = udpsource->recv_timeout_us_ / 1000000;
        timeout.tv_nsec = (udpsource->recv_timeout_us_ - timeout.tv_sec * 1000000) * 1000;
        while (thread->state() == Thread::State::RUNNING) {
            retval = ::recvmmsg(fd, msgs.data(), msgs.size(), 0, &timeout);
            if (retval > 0) {

#ifdef ZRSOCKET_DEBUG
                printf("msgs_count:%d, recvmmsg_return_value:%d\n", msgs_count, retval);
#endif // DEBUG

                for (int i = 0; i < retval; ++i) {

#ifdef ZRSOCKET_DEBUG
                    printf("recvmmsg: id:%d, recvbytes:%d\n", i, msgs[i].msg_len);
#endif // DEBUG

                    udpsource_handler->handle_read((const char *)iovecs[i].iov_base, msgs[i].msg_len, false, from_addrs[i]);
                }
            }
            else if (retval < 0) {
                return retval;
            }
        }
#else
        InetAddr    from_addr(udpsource->local_addr()->is_ipv6());
        sockaddr   *addr    = from_addr.get_addr();
        int         addrlen = from_addr.get_addr_size();

        int         buf_size = udpsource->recvbuffer_size();
        ByteBuffer  source_buf(buf_size);

        int     ret      = 0;
        int     error_id = 0;
        char   *buf      = nullptr;
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
            ret = OSApi::socket_recvfrom(fd, buf, buf_size, 0, addr, &addrlen, nullptr, error_id);
            if (ret > 0) {
                udpsource_handler->handle_read(buf, ret, is_alloc_buffer, from_addr);
            }
            else {
                return ret;
            }
        }
#endif

        return 0;
    }

protected:
    uint_t              recv_msgs_count_ = 16;
    uint_t              recv_timeout_us_ = 2000;    //2 ms
    bool                is_recv_thread_  = false;
    ThreadGroup         recv_thread_group_;         //多线程接收数据,提高并发性
    InetAddr            local_addr_;
    TUdpSourceHandler   udpsource_handler_;
};

ZRSOCKET_NAMESPACE_END

#endif
