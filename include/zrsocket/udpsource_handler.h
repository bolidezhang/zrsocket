// Some compilers (e.g. VC++) benefit significantly from using this. 
// We've measured 3-4% build speed improvements in apps as a result 
#pragma once

#ifndef UDPSOURCE_HANDLER_H
#define UDPSOURCE_HANDLER_H

#include "os_api.h"
#include "inet_addr.h"
#include "byte_buffer.h"
#include "mutex.h"
#include "event_handler.h"
#include "event_source.h"
#include <stdlib.h>
#include <algorithm>
#include <deque>

ZRSOCKET_NAMESPACE_BEGIN

template <class TSendBuffer, class TMutex, int MAX_UDPMSG_SIZE = 2048>
class UdpSourceHandler : public EventHandler
{
public:
    UdpSourceHandler()
    {
        queue_active_  = &queue1_;
        queue_standby_ = &queue2_;
    }

    virtual ~UdpSourceHandler()
    {
        close();
    }

    virtual int init(ZRSOCKET_SOCKET fd, EventSource *source, EventLoop *loop, HANDLER_STATE state)
    {
        EventHandler::init(fd, source, loop, EventHandler::STATE_OPENED);
        from_addr_.set(source->local_addr()->is_ipv6());

#ifdef ZRSOCKET_HAVE_RECVSENDMMSG
        if (nullptr != send_mmsgs_) {
            free(send_mmsgs_);
        }
        int iovecs_count = 0;
        loop->iovecs(iovecs_count);
        send_mmsgs_ = (SENDMMSG *)calloc(iovecs_count, sizeof(SENDMMSG));

        if (loop != nullptr) {
            if (nullptr != recv_mmsgs_) {
                delete recv_mmsgs_;
            }

            ByteBuffer *recv_buf = loop->get_recv_buffer();
            recv_mmsgs_ = new RECVMMSGS(std::min<int>(loop->get_recv_buffer()->buffer_size() / MAX_UDPMSG_SIZE, iovecs_count));

            for (int i = 0; i < recv_mmsgs_->msgs_count; ++i) {
                recv_mmsgs_->iovecs[i].iov_base = recv_buf->buffer() + MAX_UDPMSG_SIZE * i;
                recv_mmsgs_->iovecs[i].iov_len = MAX_UDPMSG_SIZE;
                recv_mmsgs_->msgs[i].msg_hdr.msg_iov = recv_mmsgs_->iovecs + i;
                recv_mmsgs_->msgs[i].msg_hdr.msg_iovlen = 1;
                recv_mmsgs_->msgs[i].msg_hdr.msg_name = recv_mmsgs_->from_addrs[i].get_addr();
                recv_mmsgs_->msgs[i].msg_hdr.msg_namelen = recv_mmsgs_->from_addrs[i].get_addr_size();
                recv_mmsgs_->msgs[i].msg_hdr.msg_control = nullptr;
                recv_mmsgs_->msgs[i].msg_hdr.msg_controllen = 0;
                recv_mmsgs_->msgs[i].msg_hdr.msg_flags = 0;
            }
        }
#endif
        return 0;
    }

    virtual void close()
    {
        EventHandler::close();
#ifdef ZRSOCKET_HAVE_RECVSENDMMSG
        if (nullptr != send_mmsgs_) {
            free(send_mmsgs_);
            send_mmsgs_ = nullptr;
        }
        if (nullptr != recv_mmsgs_) {
            delete recv_mmsgs_;
            recv_mmsgs_ = nullptr;
        }
#endif
    }

    virtual int handle_read(const char *data, int len, bool is_alloc_buffer, InetAddr &from_addr)
    {
        return 0;
    }

    virtual char * alloc_buffer(int buf_len)
    {
        return nullptr;
    }

    SendResult send(const char *data, uint_t len, InetAddr &to_addr, bool direct_send = true, int priority = 0, int flags = 0)
    {
        mutex_.lock();
        SendResult ret = send_i(data, len, to_addr, direct_send, priority, flags);
        mutex_.unlock();
        switch (ret) {
            case SendResult::FAILURE:
                event_loop_->delete_handler(this, 0);
                break;
            case SendResult::PUSH_QUEUE:
                event_loop_->add_event(this, EventHandler::WRITE_EVENT_MASK);
                break;
            case SendResult::SUCCESS:
                break;
            case SendResult::END:
                break;
            default:
                break;
        }
        return ret;
    }

    SendResult send(TSendBuffer &msg, InetAddr &to_addr, bool direct_send = true, int priority = 0, int flags = 0)
    {
        mutex_.lock();
        SendResult ret = send_i(msg, to_addr, direct_send, priority, flags);
        mutex_.unlock();
        switch (ret) {
            case SendResult::FAILURE:
                event_loop_->delete_handler(this, 0);
                break;
            case SendResult::PUSH_QUEUE:
                event_loop_->add_event(this, EventHandler::WRITE_EVENT_MASK);
                break;
            case SendResult::SUCCESS:
                break;
            case SendResult::END:
                break;
            default:
                break;
        }
        return ret;
    }

    SendResult send(ZRSOCKET_IOVEC *iovecs, int iovecs_count, std::list<InetAddr *> &to_addrs, bool direct_send = true, int priority = 0, int flags = 0)
    {
        mutex_.lock();
        SendResult ret = send_i(iovecs, iovecs_count, to_addrs, direct_send, priority, flags);
        mutex_.unlock();
        switch (ret) {
        case SendResult::FAILURE:
            event_loop_->delete_handler(this, 0);
            break;
        case SendResult::PUSH_QUEUE:
            event_loop_->add_event(this, EventHandler::WRITE_EVENT_MASK);
            break;
        case SendResult::SUCCESS:
            break;
        case SendResult::END:
            break;
        default:
            break;
        }
        return ret;
    }

protected:
    SendResult send_i(const char *data, uint_t len, InetAddr &to_addr, bool direct_send = true, int priority = 0, int flags = 0)
    {
        if (nullptr != event_loop_) {
            if (direct_send) {
                if (queue_standby_->empty() && queue_active_->empty()) {
                    //直接发送数据
                    int error_id = 0;
                    int send_bytes = OSApi::socket_sendto(socket_, data, len, flags,
                        to_addr.get_addr(), to_addr.get_addr_size(), nullptr, &error_id);
                    if (send_bytes > 0) {
                        return SendResult::SUCCESS;
                    }
                    else {
                        if ((ZRSOCKET_EAGAIN == error_id) ||
                            (ZRSOCKET_EWOULDBLOCK == error_id) ||
                            (ZRSOCKET_IO_PENDING == error_id) ||
                            (ZRSOCKET_ENOBUFS == error_id)) {
                            //非阻塞模式下正常情况
                        }
                        else { //非阻塞模式下异常情况
                            last_errno_ = error_id;
                            return SendResult::FAILURE;
                        }
                    }
                }
            }

            queue_standby_->emplace_back(data, len, to_addr);
            return SendResult::PUSH_QUEUE;
        }
        else {
            int error_id = 0;
            int send_bytes = OSApi::socket_sendto(socket_, data, len, flags,
                to_addr.get_addr(), to_addr.get_addr_size(), nullptr, &error_id);
            if (send_bytes > 0) {
                return SendResult::SUCCESS;
            }
            else {
                last_errno_ = error_id;
                return SendResult::END;
            }
        }
    }

    SendResult send_i(TSendBuffer &msg, InetAddr &to_addr, bool direct_send = true, int priority = 0, int flags = 0)
    {
        if (nullptr != event_loop_) {
            if (direct_send) {
                if (queue_standby_->empty() && queue_active_->empty()) {
                    int error_id = 0;
                    int send_bytes = OSApi::socket_sendto(socket_, msg.data(), msg.data_size(), flags,
                        to_addr.get_addr(), to_addr.get_addr_size(), nullptr, &error_id);
                    if (send_bytes > 0) {
                        return SendResult::SUCCESS;
                    }
                    else {
                        if ((ZRSOCKET_EAGAIN == error_id) ||
                            (ZRSOCKET_EWOULDBLOCK == error_id) ||
                            (ZRSOCKET_IO_PENDING == error_id) ||
                            (ZRSOCKET_ENOBUFS == error_id)) {
                            //非阻塞模式下正常情况
                        }
                        else {
                            //非阻塞模式下异常情况
                            last_errno_ = error_id;
                            return SendResult::FAILURE;
                        }
                    }
                }
            }

            queue_standby_->emplace_back(msg, to_addr);
            return SendResult::PUSH_QUEUE;
        }
        else {
            int error_id = 0;
            int send_bytes = OSApi::socket_sendto(socket_, msg.data(), msg.data_size(), flags, 
                to_addr.get_addr(), to_addr.get_addr_size(), nullptr, &error_id);
            if (send_bytes > 0) {
                return SendResult::SUCCESS;
            }
            else {
                last_errno_ = error_id;
                return SendResult::END;
            }
        }
    }

    SendResult send_i(ZRSOCKET_IOVEC *iovecs, int iovecs_count, std::list<InetAddr *> &to_addrs, bool direct_send = true, int priority = 0, int flags = 0)
    {
        if (nullptr != event_loop_) {
            if (direct_send) {
            }
        }
        else {
        }

        return SendResult::END;
    }

    inline int handle_read()
    {
#ifndef ZRSOCKET_HAVE_RECVSENDMMSG
        sockaddr   *addr = from_addr_.get_addr();
        int         addrlen = from_addr_.get_addr_size();
        ByteBuffer *recv_buffer = event_loop_->get_recv_buffer();
        int         loop_buf_size = recv_buffer->buffer_size();
        char       *loop_buf = recv_buffer->buffer();
        int         source_buf_size = source_->recvbuffer_size();

        int   error = 0;
        int   ret = 0;
        char *buf = nullptr;
        int   buf_size = 0;
        bool  is_alloc_buf = false;

        do {
            buf = alloc_buffer(source_buf_size);
            if (nullptr == buf) {
                is_alloc_buf = false;
                buf = loop_buf;
                buf_size = loop_buf_size;
            }
            else {
                is_alloc_buf = true;
                buf_size = source_buf_size;
            }

            ret = OSApi::socket_recvfrom(socket_, buf, buf_size, 0, addr, &addrlen, nullptr, error);
            if (ret > 0) {
                handle_read(buf, ret, is_alloc_buf, from_addr_);
            }
            else {
                if ((ZRSOCKET_EAGAIN == error) || (ZRSOCKET_EWOULDBLOCK == error) || (ZRSOCKET_EINTR == error)) {
                    //非阻塞模式下正常情况
                    return 0;
                }
                return -1;
            }
        } while (1);
#else
        int retval = 0;
        do {
            retval = ::recvmmsg(socket_, recv_mmsgs_->msgs, recv_mmsgs_->msgs_count, 0, nullptr);
            if (retval <= 0) {
                int error_id = OSApi::socket_get_lasterror();
                if ((ZRSOCKET_EAGAIN == error_id) ||
                    (ZRSOCKET_EWOULDBLOCK == error_id) ||
                    (ZRSOCKET_IO_PENDING == error_id) ||
                    (ZRSOCKET_ENOBUFS == error_id)) {
                    //非阻塞模式下正常情况
                    return 0;
                }
                else {
                    //非阻塞模式下异常情况
                    last_errno_ = error_id;
                    return -1;
                }
            }

#ifdef ZRSOCKET_DEBUG
            printf("msgs_count:%d, recvmmsg_return_value:%d\n", recv_mmsgs_->msgs_count, retval);
#endif // DEBUG

            for (int i = 0; i < retval; ++i) {

#ifdef ZRSOCKET_DEBUG
                printf("recvmmsg: id:%d, recvbytes:%d\n", i, recv_mmsgs_->msgs[i].msg_len);
#endif // DEBUG

                handle_read((const char *)recv_mmsgs_->iovecs[i].iov_base, recv_mmsgs_->msgs[i].msg_len, false, recv_mmsgs_->from_addrs[i]);
            }
        } while (1);

#endif

        return 0;
    }

    int handle_write()
    {
        mutex_.lock();
        if (queue_active_->empty()) {
            if (!queue_standby_->empty()) {
                std::swap(queue_standby_, queue_active_);
            }
            else {
                mutex_.unlock();
                event_loop_->delete_event(this, EventHandler::WRITE_EVENT_MASK);
                return EventHandler::WriteResult::WRITE_RESULT_NOT_DATA;
            }
        }
        mutex_.unlock();

#ifdef ZRSOCKET_HAVE_RECVSENDMMSG
        int iovecs_count = 0;
        ZRSOCKET_IOVEC *iovecs = event_loop_->iovecs(iovecs_count);
        int msgs_count = -1;
        for (auto iter = queue_active_->begin(); iter != queue_active_->end(); ++iter) {
            for (auto iter_addr = (*iter).to_addrs_.begin(); iter_addr != (*iter).to_addrs_.end(); ++iter_addr) {
                ++msgs_count;
                if (msgs_count < iovecs_count) {
                    iovecs[msgs_count].iov_base = (*iter).buf_.data();
                    iovecs[msgs_count].iov_len  = (*iter).buf_.data_size();
                    send_mmsgs_[msgs_count].msg_len = 0;
                    send_mmsgs_[msgs_count].msg_hdr.msg_iov = iovecs + msgs_count;
                    send_mmsgs_[msgs_count].msg_hdr.msg_iovlen = 1;
                    send_mmsgs_[msgs_count].msg_hdr.msg_name = (*iter_addr).get_addr();
                    send_mmsgs_[msgs_count].msg_hdr.msg_namelen = (*iter_addr).get_addr_size();
                    send_mmsgs_[msgs_count].msg_hdr.msg_control = nullptr;
                    send_mmsgs_[msgs_count].msg_hdr.msg_controllen = 0;
                    send_mmsgs_[msgs_count].msg_hdr.msg_flags = 0;
                }
                else {
                    goto START_SENDMMSG;
                }
            }
        }

    START_SENDMMSG:
        ++msgs_count;
        if (msgs_count > 0) {
            int retval = ::sendmmsg(socket_, send_mmsgs_, msgs_count, 0);
            if (retval <= 0) {
                int error_id = OSApi::socket_get_lasterror();
                if ((ZRSOCKET_EAGAIN == error_id) ||
                    (ZRSOCKET_EWOULDBLOCK == error_id) ||
                    (ZRSOCKET_IO_PENDING == error_id) ||
                    (ZRSOCKET_ENOBUFS == error_id)) {
                    //非阻塞模式下正常情况
                    return EventHandler::WriteResult::WRITE_RESULT_PART;
                }
                else {
                    //非阻塞模式下异常情况
                    last_errno_ = error_id;
                    return EventHandler::WriteResult::WRITE_RESULT_FAILURE;
                }
            }

#ifdef ZRSOCKET_DEBUG
            printf("msgs_count:%d, sendmmsg_return_value:%d\n", msgs_count, retval);
            for (int i = 0; i < msgs_count; ++i) {
                printf("sendmmsg: id:%d, sendbytes:%d\n", i, send_mmsgs_[i].msg_len);
            }
#endif // DEBUG

            int remain = retval;
            auto iter = queue_active_->begin();
            while (iter != queue_active_->end()) {
                std::deque<InetAddr> &addrs = (*iter).to_addrs_;
                auto addrs_size = addrs.size();
                if (remain - addrs_size >= 0) {
                    remain -= addrs_size;
                    addrs.clear();
                    iter = queue_active_->erase(iter);
                }
                else {
                    addrs.erase(addrs.begin(), addrs.begin() + (addrs_size - remain));
                    break;
                }
            }

            if (retval == msgs_count) {
                return EventHandler::WriteResult::WRITE_RESULT_SUCCESS;
            }
            else {
                return EventHandler::WriteResult::WRITE_RESULT_PART;
            }
        }
        else {
            return EventHandler::WriteResult::WRITE_RESULT_NOT_DATA;
        }
#else
        int error_id = 0;
        do {
            UdpBuffer &buf = queue_active_->front();
            if (OSApi::socket_sendto(socket_, buf.buf_.data(), buf.buf_.data_size(), 0, 
                buf.to_addr_.get_addr(), buf.to_addr_.get_addr_size(), nullptr, &error_id) > 0) {
                queue_active_->pop_front();
            }
            else {
                if ((ZRSOCKET_EAGAIN == error_id) ||
                    (ZRSOCKET_EWOULDBLOCK == error_id) ||
                    (ZRSOCKET_IO_PENDING == error_id) ||
                    (ZRSOCKET_ENOBUFS == error_id)) {
                    //非阻塞模式下正常情况
                    return EventHandler::WriteResult::WRITE_RESULT_PART;
                }
                else {
                    //非阻塞模式下异常情况
                    last_errno_ = error_id;
                    return EventHandler::WriteResult::WRITE_RESULT_FAILURE;
                }
            }
        } while (!queue_active_->empty());
#endif

        return EventHandler::WriteResult::WRITE_RESULT_NOT_DATA;
    }

private:
    InetAddr from_addr_;

#ifdef ZRSOCKET_HAVE_RECVSENDMMSG
    class UdpBuffer
    {
    public:
        UdpBuffer(const char *buf, uint_t len, InetAddr &to_addr)
            : buf_(buf, len)
        {
            to_addrs_.emplace_back(to_addr);
        }

        UdpBuffer(TSendBuffer &buf, InetAddr &to_addr)
            : buf_(std::move(buf))
        {
            to_addrs_.emplace_back(to_addr);
        }

        UdpBuffer(const UdpBuffer &udp_buffer)
            : buf_(std::move(udp_buffer.buf_))
            , to_addrs_(std::move(udp_buffer.to_addrs_))
        {
        }

        UdpBuffer(UdpBuffer &&udp_buffer)
            : buf_(std::move(udp_buffer.buf_))
            , to_addrs_(std::move(udp_buffer.to_addrs_))
        {
        }

        UdpBuffer &operator= (const UdpBuffer &udp_buffer)
        {
            buf_ = udp_buffer.buf_;
            to_addrs_ = udp_buffer.to_addrs_;
            return *this;
        }

        UdpBuffer &operator= (UdpBuffer &&udp_buffer) noexcept
        {
            buf_ = std::move(udp_buffer.buf_);
            to_addrs_ = std::move(udp_buffer.to_addrs_);
            return *this;
        }

        TSendBuffer buf_;
        std::deque<InetAddr> to_addrs_;
    };
#else
    class UdpBuffer
    {
    public:
        UdpBuffer(const char *buf, uint_t len, InetAddr &to_addr)
            : buf_(buf, len)
            , to_addr_(std::move(to_addr))
        {
        }

        UdpBuffer(TSendBuffer &buf, InetAddr &to_addr)
            : buf_(std::move(buf))
            , to_addr_(std::move(to_addr))
        {
        }

        UdpBuffer(const UdpBuffer &udp_buffer)
            : buf_(std::move(udp_buffer.buf_))
            , to_addr_(std::move(udp_buffer.to_addr_))
        {
        }

        UdpBuffer(UdpBuffer &&udp_buffer)
            : buf_(std::move(udp_buffer.buf_))
            , to_addr_(std::move(udp_buffer.to_addr_))
        {
        }

        UdpBuffer & operator= (const UdpBuffer &udp_buffer)
        {
            buf_ = udp_buffer.buf_;
            to_addr_ = udp_buffer.to_addr_;
            return *this;
        }

        UdpBuffer & operator= (UdpBuffer &&udp_buffer) noexcept
        {
            buf_ = std::move(udp_buffer.buf_);
            to_addr_ = std::move(udp_buffer.to_addr_);
            return *this;
        }

        TSendBuffer buf_;
        InetAddr    to_addr_;
    };
#endif

    typedef std::deque<UdpBuffer> SEND_QUEUE;
    SEND_QUEUE      queue1_;
    SEND_QUEUE      queue2_;
    SEND_QUEUE     *queue_active_;
    SEND_QUEUE     *queue_standby_;
    TMutex          mutex_;

#ifdef ZRSOCKET_HAVE_RECVSENDMMSG
    typedef struct mmsghdr SENDMMSG;
    SENDMMSG *send_mmsgs_ = nullptr;

    //const int MAX_UDPMSG_SIZE = 2048;
    //const int MTU_SIZE = 2048;
    class RECVMMSGS 
    {
    public:
        RECVMMSGS(int count)
            : msgs_count(count)
        {
            msgs = new mmsghdr[count];
            iovecs = new ZRSOCKET_IOVEC[count];
            from_addrs = new InetAddr[count];
        }

        ~RECVMMSGS()
        {
            if (nullptr != msgs) {
                delete []msgs;
            }
            if (nullptr != iovecs) {
                delete []iovecs;
            }
            if (nullptr != from_addrs) {
                delete []from_addrs;
            }
        }
    public:
        mmsghdr *msgs = nullptr;
        ZRSOCKET_IOVEC *iovecs = nullptr;
        InetAddr *from_addrs = nullptr;
        int msgs_count = 0;
    };
    RECVMMSGS *recv_mmsgs_ = nullptr;
#endif

};

ZRSOCKET_NAMESPACE_END

#endif
