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
#include <vector>
#include <deque>

ZRSOCKET_NAMESPACE_BEGIN

template <class TSendBuffer, class TMutex, int MAX_UDPMSG_SIZE = 2048>
class UdpSourceHandler : public EventHandler
{
public:
    UdpSourceHandler()
        : max_udpmsg_size_(MAX_UDPMSG_SIZE)
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
        if (nullptr != loop) {
            int iovecs_count = 0;
            loop->iovecs(iovecs_count);
            send_mmsgs_.resize(iovecs_count);

            if (nullptr != recv_mmsgs_) {
                delete recv_mmsgs_;
            }

            ByteBuffer *recv_buf = loop->get_recv_buffer();
            recv_mmsgs_ = new RECVMMSGS(std::min<int>(loop->get_recv_buffer()->buffer_size() / MAX_UDPMSG_SIZE, iovecs_count));

            for (int i = 0; i < recv_mmsgs_->msgs_count; ++i) {
                recv_mmsgs_->iovecs[i].iov_base = recv_buf->buffer() + MAX_UDPMSG_SIZE * i;
                recv_mmsgs_->iovecs[i].iov_len  = MAX_UDPMSG_SIZE;
                recv_mmsgs_->msgs[i].msg_hdr.msg_iov     = recv_mmsgs_->iovecs + i;
                recv_mmsgs_->msgs[i].msg_hdr.msg_iovlen  = 1;
                recv_mmsgs_->msgs[i].msg_hdr.msg_name    = recv_mmsgs_->from_addrs[i].get_addr();
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
        if (nullptr != recv_mmsgs_) {
            delete recv_mmsgs_;
            recv_mmsgs_ = nullptr;
        }
#endif
    }

    int get_max_udpmsg_size()
    {
        return max_udpmsg_size_;
    }

    virtual int handle_read(const char *data, int len, bool is_alloc_buffer, InetAddr &from_addr)
    {
        return 0;
    }

    virtual char * alloc_buffer(int buf_len)
    {
        return nullptr;
    }

    int send(const char *data, uint_t len, InetAddr &to_addr, bool direct_send = true, int priority = 0, int flags = 0)
    {
        mutex_.lock();
        int ret = send_i(data, len, to_addr, direct_send, priority, flags);
        mutex_.unlock();

        if (nullptr != event_loop_) {
            if (static_cast<int>(SendResult::PUSH_QUEUE) == ret) {
                event_loop_->add_event(this, EventHandler::WRITE_EVENT_MASK);
            }
            else if (ret < 0) {
                event_loop_->delete_handler(this, 0);
            }
        }

        return ret;
    }

    int send(TSendBuffer &msg, InetAddr &to_addr, bool direct_send = true, int priority = 0, int flags = 0)
    {
        mutex_.lock();
        int ret = send_i(msg, to_addr, direct_send, priority, flags);
        mutex_.unlock();

        if (nullptr != event_loop_) {
            if (static_cast<int>(SendResult::PUSH_QUEUE) == ret) {
                event_loop_->add_event(this, EventHandler::WRITE_EVENT_MASK);
            }
            else if (ret < 0) {
                event_loop_->delete_handler(this, 0);
            }
        }

        return ret;
    }

    int send(ZRSOCKET_IOVEC *iovecs, int iovecs_count, InetAddr &to_addr, bool direct_send = true, int priority = 0, int flags = 0)
    {
        mutex_.lock();
        int ret = send_i(iovecs, iovecs_count, to_addr, direct_send, priority, flags);
        mutex_.unlock();

        if (nullptr != event_loop_) {
            if (static_cast<int>(SendResult::PUSH_QUEUE) == ret) {
                event_loop_->add_event(this, EventHandler::WRITE_EVENT_MASK);
            }
            else if (ret < 0) {
                event_loop_->delete_handler(this, 0);
            }
        }

        return ret;
    }

    //返回已发送的消息数
    template <class TAddrsIt, class TFnGetInetAddr>
    int send(ZRSOCKET_IOVEC *iovecs, int iovecs_count, TAddrsIt addrs_first, TAddrsIt addrs_last, TFnGetInetAddr get_addr, bool direct_send = true, int priority = 0, int flags = 0)
    {
        mutex_.lock();
        int ret = send_i(iovecs, iovecs_count, addrs_first, addrs_last, get_addr, direct_send, priority, flags);
        mutex_.unlock();

        if (nullptr != event_loop_) {
            if (static_cast<int>(SendResult::PUSH_QUEUE) == ret) {
                event_loop_->add_event(this, EventHandler::WRITE_EVENT_MASK);
            }
            else if (ret < 0) {
                event_loop_->delete_handler(this, 0);
            }
        }

        return ret;
    }   

protected:
    int send_i(const char *data, uint_t len, InetAddr &to_addr, bool direct_send = true, int priority = 0, int flags = 0)
    {
        if (nullptr != event_loop_) {
            if (direct_send) {
                if (queue_standby_->empty() && queue_active_->empty()) {
                    //直接发送数据
                    int error_id = 0;
                    int send_bytes = OSApi::socket_sendto(fd_, data, len, flags, 
                        to_addr.get_addr(), to_addr.get_addr_size(), nullptr, &error_id);
                    if (send_bytes > 0) {
                        return static_cast<int>(SendResult::SUCCESS);
                    }
                    else {
                        last_errno_ = -error_id;
                        if ((ZRSOCKET_EAGAIN == error_id) ||
                            (ZRSOCKET_EWOULDBLOCK == error_id) ||
                            (ZRSOCKET_IO_PENDING == error_id) ||
                            (ZRSOCKET_ENOBUFS == error_id)) {
                            //非阻塞模式下正常情况
                        }
                        else { //非阻塞模式下异常情况
                            return last_errno_;
                        }
                    }
                }
            }

            queue_standby_->emplace_back(data, len, to_addr);
            return static_cast<int>(SendResult::PUSH_QUEUE);
        }
        else {
            int error_id = 0;
            int send_bytes = OSApi::socket_sendto(fd_, data, len, flags, 
                to_addr.get_addr(), to_addr.get_addr_size(), nullptr, &error_id);
            if (send_bytes > 0) {
                return static_cast<int>(SendResult::SUCCESS);
            }
            else {
                last_errno_ = -error_id;
                return last_errno_;
            }
        }
    }

    int send_i(TSendBuffer &msg, InetAddr &to_addr, bool direct_send = true, int priority = 0, int flags = 0)
    {
        if (nullptr != event_loop_) {
            if (direct_send && queue_standby_->empty() && queue_active_->empty()) {
                int error_id = 0;
                int send_bytes = OSApi::socket_sendto(fd_, msg.data(), msg.data_size(), flags, 
                    to_addr.get_addr(), to_addr.get_addr_size(), nullptr, &error_id);
                if (send_bytes > 0) {
                    return static_cast<int>(SendResult::SUCCESS);
                }
                else {
                    last_errno_ = -error_id;
                    if ((ZRSOCKET_EAGAIN == error_id) ||
                        (ZRSOCKET_EWOULDBLOCK == error_id) ||
                        (ZRSOCKET_IO_PENDING == error_id) ||
                        (ZRSOCKET_ENOBUFS == error_id)) {
                        //非阻塞模式下正常情况
                    }
                    else {
                        //非阻塞模式下异常情况
                        return last_errno_;
                    }
                }
            }

            queue_standby_->emplace_back(msg, to_addr);
            return static_cast<int>(SendResult::PUSH_QUEUE);
        }
        else {
            int error_id = 0;
            int send_bytes = OSApi::socket_sendto(fd_, msg.data(), msg.data_size(), flags, 
                to_addr.get_addr(), to_addr.get_addr_size(), nullptr, &error_id);
            if (send_bytes > 0) {
                return static_cast<int>(SendResult::SUCCESS);
            }
            else {
                last_errno_ = -error_id;
                return last_errno_;
            }
        }
    }

    int send_i(ZRSOCKET_IOVEC *iovecs, int iovecs_count, InetAddr &to_addr, bool direct_send = true, int priority = 0, int flags = 0)
    {
        if (nullptr != event_loop_) {
            if (direct_send && queue_standby_->empty() && queue_active_->empty()) {
                int error_id = 0;
                int send_bytes = OSApi::socket_sendtov(fd_, iovecs, iovecs_count, flags, 
                    to_addr.get_addr(), to_addr.get_addr_size(), nullptr, &error_id);
                if (send_bytes > 0) {
                    return static_cast<int>(SendResult::SUCCESS);
                }
                else {
                    last_errno_ = -error_id;
                    if ((ZRSOCKET_EAGAIN == error_id) ||
                        (ZRSOCKET_EWOULDBLOCK == error_id) ||
                        (ZRSOCKET_IO_PENDING == error_id) ||
                        (ZRSOCKET_ENOBUFS == error_id)) {
                        //非阻塞模式下正常情况
                    }
                    else {
                        //非阻塞模式下异常情况
                        return last_errno_;
                    }
                }
            }

            UdpBuffer buffer;
            buffer.buf_.reserve(1024);
            for (int i = 0; i < iovecs_count; ++i) {
                buffer.buf_.write(static_cast<const char *>(iovecs[i].iov_base), iovecs[i].iov_len);
            }
            buffer.to_addrs_.push_back(to_addr);
            queue_standby_->push_back(std::move(buffer));
            return static_cast<int>(SendResult::PUSH_QUEUE);
        }
        else {
            int error_id = 0;
            int send_bytes = OSApi::socket_sendtov(fd_, iovecs, iovecs_count, flags,
                to_addr.get_addr(), to_addr.get_addr_size(), nullptr, &error_id);
            if (send_bytes > 0) {
                return static_cast<int>(SendResult::SUCCESS);
            }
            else {
                last_errno_ = -error_id;
                return last_errno_;
            }
        }
    }

    //返回已发送的消息数
    template <class TAddrsIt, class TfnGetInetAddr>
    int send_i(ZRSOCKET_IOVEC *iovecs, int iovecs_count, TAddrsIt addrs_first, TAddrsIt addrs_last, TfnGetInetAddr get_addr, bool direct_send = true, int priority = 0, int flags = 0)
    {
#ifdef ZRSOCKET_HAVE_RECVSENDMMSG
        if (nullptr != event_loop_) {
            int sendmsg_count = 0;
            if (direct_send && queue_standby_->empty() && queue_active_->empty()) {
                int  i = 0;
                std::vector<struct mmsghdr> msgs;
                msgs.reserve(100);
                struct mmsghdr msg;
                for (auto iter = addrs_first; iter != addrs_last; ++iter) {
                    InetAddr *addr = get_addr(*iter);
                    msg.msg_len = 0;
                    msg.msg_hdr.msg_name    = addr->get_addr();
                    msg.msg_hdr.msg_namelen = addr->get_addr_size();
                    msg.msg_hdr.msg_iov     = iovecs;
                    msg.msg_hdr.msg_iovlen  = iovecs_count;
                    msg.msg_hdr.msg_control = nullptr;
                    msg.msg_hdr.msg_controllen = 0;
                    msg.msg_hdr.msg_flags = flags;
                    msgs.push_back(std::move(msg));
                    ++i;
                }

                sendmsg_count = ::sendmmsg(fd_, msgs.data(), msgs.size(), flags);
                if (static_cast<size_t>(sendmsg_count) == msgs.size()) {
                    return sendmsg_count;
                }
                else if (sendmsg_count <= 0) {
                    int error_id = OSApi::socket_get_lasterror();
                    last_errno_  = -error_id;
                    if ((ZRSOCKET_EAGAIN == error_id) ||
                        (ZRSOCKET_EWOULDBLOCK == error_id) ||
                        (ZRSOCKET_IO_PENDING == error_id) ||
                        (ZRSOCKET_ENOBUFS == error_id)) {
                        //非阻塞模式下正常情况
                    }
                    else {
                        //非阻塞模式下异常情况
                        return last_errno_;
                    }
                }
            }

            UdpBuffer buffer;
            buffer.buf_.reserve(1024);
            for (int i = 0; i < iovecs_count; ++i) {
                buffer.buf_.write(static_cast<const char *>(iovecs[i].iov_base), iovecs[i].iov_len);
            }
            auto iter = addrs_first;
            for (int i = 0; i < sendmsg_count; ++i) {
                ++iter;
            }
            for (; iter != addrs_last; ++iter) {
                buffer.to_addrs_.push_back(*get_addr(*iter));
            }
            queue_standby_->push_back(std::move(buffer));

            return sendmsg_count;
        }
        else {
            int  i = 0;
            std::vector<struct mmsghdr> msgs;
            msgs.reserve(100);
            struct mmsghdr msg;
            for (auto iter = addrs_first; iter != addrs_last; ++iter) {
                InetAddr *addr = get_addr(*iter);
                msg.msg_len = 0;
                msg.msg_hdr.msg_name    = addr->get_addr();
                msg.msg_hdr.msg_namelen = addr->get_addr_size();
                msg.msg_hdr.msg_iov     = iovecs;
                msg.msg_hdr.msg_iovlen  = iovecs_count;
                msg.msg_hdr.msg_control = nullptr;
                msg.msg_hdr.msg_controllen = 0;
                msg.msg_hdr.msg_flags = flags;
                msgs.push_back(std::move(msg));
                ++i;
            }

            int sendmsg_count = ::sendmmsg(fd_, msgs.data(), msgs.size(), flags);
            if (sendmsg_count <= 0) {
                int error_id = OSApi::socket_get_lasterror();
                last_errno_  = -error_id;
            }

            return sendmsg_count;
        }
#else
        int sendmsg_count = 0;
        if (nullptr != event_loop_) {
            auto iter = addrs_first;
            if (direct_send && queue_standby_->empty() && queue_active_->empty()) {
                for (; iter != addrs_last; ++iter) {
                    InetAddr *addr = get_addr(*iter);
                    int error_id = 0;
                    int send_bytes = OSApi::socket_sendtov(fd_, iovecs, iovecs_count, flags, 
                        addr->get_addr(), addr->get_addr_size(), nullptr, &error_id);
                    if (send_bytes <= 0) {
                        last_errno_ = -error_id;
                        if ((ZRSOCKET_EAGAIN == error_id) ||
                            (ZRSOCKET_EWOULDBLOCK == error_id) ||
                            (ZRSOCKET_IO_PENDING == error_id) ||
                            (ZRSOCKET_ENOBUFS == error_id)) {
                            //非阻塞模式下正常情况
                            break;
                        }
                        else {
                            //非阻塞模式下异常情况
                            return sendmsg_count;
                        }
                    }
                    ++sendmsg_count;
                }
            }

            if (iter != addrs_last) {
                UdpBuffer buffer;
                buffer.buf_.reserve(1024);
                for (int i = 0; i < iovecs_count; ++i) {
                    buffer.buf_.write(static_cast<const char*>(iovecs[i].iov_base), iovecs[i].iov_len);
                }
                for (; iter != addrs_last; ++iter) {
                    buffer.to_addrs_.push_back(*get_addr(*iter));
                }
                queue_standby_->push_back(std::move(buffer));
            }

            return sendmsg_count;

        }
        else {
            for (auto iter = addrs_first; iter != addrs_last; ++iter) {
                InetAddr *addr = get_addr(*iter);
                int error_id = 0;
                int send_bytes = OSApi::socket_sendtov(fd_, iovecs, iovecs_count, flags, 
                    addr->get_addr(), addr->get_addr_size(), nullptr, &error_id);
                if (send_bytes <= 0) {
                    last_errno_ = -error_id;
                    return sendmsg_count;
                }
                ++sendmsg_count;
            }

            return sendmsg_count;
        }
#endif

        return 0;
    }

    inline int handle_read()
    {
#ifdef ZRSOCKET_HAVE_RECVSENDMMSG
        int retval = 0;
        do {
            retval = ::recvmmsg(fd_, recv_mmsgs_->msgs, recv_mmsgs_->msgs_count, 0, nullptr);
            if (retval <= 0) {
                int error_id = OSApi::socket_get_lasterror();
                last_errno_  = -error_id;
                if ((ZRSOCKET_EAGAIN == error_id) ||
                    (ZRSOCKET_EWOULDBLOCK == error_id) ||
                    (ZRSOCKET_IO_PENDING == error_id) ||
                    (ZRSOCKET_ENOBUFS == error_id)) {
                    //非阻塞模式下正常情况
                    return 0;
                }
                else {
                    //非阻塞模式下异常情况
                    return last_errno_;
                }
            }

#ifdef ZRSOCKET_DEBUG
            printf("msgs_count:%d, recvmmsg_return_value:%d\n", recv_mmsgs_->msgs_count, retval);
#endif // DEBUG

            for (int i = 0; i < retval; ++i) {

#ifdef ZRSOCKET_DEBUG
                printf("recvmmsg: id:%d, recvbytes:%d\n", i, recv_mmsgs_->msgs[i].msg_len);
#endif // DEBUG

                handle_read((const char*)recv_mmsgs_->iovecs[i].iov_base, recv_mmsgs_->msgs[i].msg_len, false, recv_mmsgs_->from_addrs[i]);
            }
        } while (1);

#else
        sockaddr   *addr = from_addr_.get_addr();
        int         addrlen = from_addr_.get_addr_size();
        ByteBuffer *recv_buffer = event_loop_->get_recv_buffer();
        int         loop_buf_size = recv_buffer->buffer_size();
        char       *loop_buf = recv_buffer->buffer();
        int         source_buf_size = source_->recvbuffer_size();

        int   error_id = 0;
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

            ret = OSApi::socket_recvfrom(fd_, buf, buf_size, 0, addr, &addrlen, nullptr, error_id);
            if (ret > 0) {
                handle_read(buf, ret, is_alloc_buf, from_addr_);
            }
            else {
                error_id = OSApi::socket_get_lasterror();
                last_errno_ = -error_id;
                if ((ZRSOCKET_EAGAIN == error_id) || (ZRSOCKET_EWOULDBLOCK == error_id) || (ZRSOCKET_EINTR == error_id)) {
                    //非阻塞模式下正常情况
                    return 0;
                }
                return last_errno_;
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
                    send_mmsgs_[msgs_count].msg_hdr.msg_iov     = iovecs + msgs_count;
                    send_mmsgs_[msgs_count].msg_hdr.msg_iovlen  = 1;
                    send_mmsgs_[msgs_count].msg_hdr.msg_name    = (*iter_addr).get_addr();
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
            int retval = ::sendmmsg(fd_, send_mmsgs_.data(), send_mmsgs_.size(), 0);
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

            auto iter_addr = buf.to_addrs_.begin();
            while (iter_addr != buf.to_addrs_.end()) {
                if (OSApi::socket_sendto(fd_, buf.buf_.data(), buf.buf_.data_size(), 0, 
                    (*iter_addr).get_addr(), (*iter_addr).get_addr_size(), nullptr, &error_id) > 0) {
                    iter_addr = buf.to_addrs_.erase(iter_addr);
                }
                else {
                    last_errno_ = -error_id;
                    if ((ZRSOCKET_EAGAIN == error_id) ||
                        (ZRSOCKET_EWOULDBLOCK == error_id) ||
                        (ZRSOCKET_IO_PENDING == error_id) ||
                        (ZRSOCKET_ENOBUFS == error_id)) {
                        //非阻塞模式下正常情况
                        return EventHandler::WriteResult::WRITE_RESULT_PART;
                    }
                    else {
                        //非阻塞模式下异常情况
                        return EventHandler::WriteResult::WRITE_RESULT_FAILURE;
                    }
                }
            }

            queue_active_->pop_front();
        } while (!queue_active_->empty());
#endif

        return EventHandler::WriteResult::WRITE_RESULT_NOT_DATA;
    }

private:
    int      max_udpmsg_size_ = 2048;
    InetAddr from_addr_;

    class UdpBuffer
    {
    public:
        UdpBuffer() = default;
        ~UdpBuffer()
        {
            buf_.clear();
            to_addrs_.clear();
        }

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

    typedef std::deque<UdpBuffer> SEND_QUEUE;
    SEND_QUEUE      queue1_;
    SEND_QUEUE      queue2_;
    SEND_QUEUE     *queue_active_;
    SEND_QUEUE     *queue_standby_;
    TMutex          mutex_;

#ifdef ZRSOCKET_HAVE_RECVSENDMMSG
    typedef struct mmsghdr SENDMMSG;
    std::vector<SENDMMSG> send_mmsgs_;

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
