// Some compilers (e.g. VC++) benefit significantly from using this. 
// We've measured 3-4% build speed improvements in apps as a result 
#pragma once

#ifndef ZRSOCKET_MESSAGE_HANDLER_H
#define ZRSOCKET_MESSAGE_HANDLER_H
#include <deque>
#include "config.h"
#include "byte_buffer.h"
#include "mutex.h"
#include "os_api.h"
#include "event_handler.h"
#include "event_source.h"

ZRSOCKET_NAMESPACE_BEGIN

template <class TSendBuffer, class TMutex>
class MessageHandler : public EventHandler
{
public:
    MessageHandler()
    {
        queue_active_   = &queue1_;
        queue_standby_  = &queue2_;
    }

    virtual ~MessageHandler()
    {
    }

    virtual int do_open()
    {
        return 0;
    }

    virtual int do_close()
    {
        return 0;
    }

    virtual int do_connect()
    {
        return 0;
    }

    virtual int decode(const char *data, uint_t len)
    {
        return 0;
    }

    int send(const char *data, uint_t len, bool direct_send = true, int priority = 0, int flags = 0)
    {
        mutex_.lock();
        int ret = send_i(data, len, direct_send, priority, flags);
        mutex_.unlock();

        if (static_cast<int>(SendResult::PUSH_QUEUE) == ret) {
            event_loop_->add_event(this, EventHandler::WRITE_EVENT_MASK);
        }
        else if (ret < 0) {
            event_loop_->delete_handler(this, 0);
        }

        return ret;
    }

    int send(TSendBuffer &msg, bool direct_send = true, int priority = 0, int flags = 0)
    {
        mutex_.lock();
        int ret = send_i(msg, direct_send, priority, flags);
        mutex_.unlock();

        if (static_cast<int>(SendResult::PUSH_QUEUE) == ret) {
            event_loop_->add_event(this, EventHandler::WRITE_EVENT_MASK);
        }
        else if (ret < 0) {
            event_loop_->delete_handler(this, 0);
        }
        return ret;
    }

    int send(ZRSOCKET_IOVEC *iovecs, int iovecs_count, bool direct_send = true, int priority = 0, int flags = 0)
    {
        mutex_.lock();
        int ret = send_i(iovecs, iovecs_count, direct_send, priority, flags);
        mutex_.unlock();

        if (static_cast<int>(SendResult::PUSH_QUEUE) == ret) {
            event_loop_->add_event(this, EventHandler::WRITE_EVENT_MASK);
        }
        else if (ret < 0) {
            event_loop_->delete_handler(this, 0);
        }

        return ret;
    }

    int handle_open()
    {
        message_buffer_.reset();
        message_buffer_.reserve(source_->recvbuffer_size());
        queue1_.clear();
        queue2_.clear();
        return do_open();
    }

protected:
    int handle_close()
    {
        int ret = do_close();
        mutex_.lock();
        queue1_.clear();
        queue2_.clear();
        mutex_.unlock();
        close();
        return ret;
    }

    int handle_connect()
    {
        return do_connect();
    }

    int handle_read()
    {
        ByteBuffer *recv_buffer = event_loop_->get_recv_buffer();
        char *recv_buf          = recv_buffer->buffer();
        int   recv_buf_size     = recv_buffer->buffer_size();
        int   error_id = 0;
        int   ret;

        do {
            ret = OSApi::socket_recv(fd_, recv_buf, recv_buf_size, 0, nullptr, error_id);
            if (ret > 0) {
                ret = decode(recv_buf, ret);
                if (ret < 0) {
                    return ret;
                }
            }
            else if (0 == ret) {
                //连接已关闭
                last_errno_ = EventHandler::ERROR_CLOSE_PASSIVE;
                return last_errno_;
            }
            else {
                //ret < 0: 出现异常(如连接已关闭)
                last_errno_ = -error_id;
                if ((ZRSOCKET_EAGAIN == error_id) ||
                    (ZRSOCKET_EWOULDBLOCK == error_id) ||
                    (ZRSOCKET_EINTR == error_id)) {
                    //非阻塞模式下正常情况
                    return 0;
                }
                return last_errno_;
            }
        } while (ret == recv_buf_size);

        return 1;
    }

    int send_i(const char *data, uint_t len, bool direct_send = true, int priority = 0, int flags = 0)
    {
        if (direct_send && queue_standby_->empty() && queue_active_->empty()) {
            //直接发送数据
            int error_id = 0;
            int send_bytes = OSApi::socket_send(fd_, data, len, flags, nullptr, &error_id);
            if (send_bytes > 0) {
                if ((uint_t)send_bytes == len) {
                    return static_cast<int>(SendResult::SUCCESS);
                }

                queue_standby_->emplace_back(data + send_bytes, len - send_bytes);
                return static_cast<int>(SendResult::PUSH_QUEUE);
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

        queue_standby_->emplace_back(data, len);
        return static_cast<int>(SendResult::PUSH_QUEUE);
    }

    int send_i(TSendBuffer &msg, bool direct_send = true, int priority = 0, int flags = 0)
    {
        if (direct_send && queue_standby_->empty() && queue_active_->empty()) {
            char *data = msg.data();
            uint_t len = msg.data_size();
            int error_id = 0;
            int send_bytes = OSApi::socket_send(fd_, data, len, flags, nullptr, &error_id);
            if (send_bytes > 0) {
                if ((uint_t)send_bytes == len) {
                    return static_cast<int>(SendResult::SUCCESS);
                }

                msg.data_begin(msg.data_begin() + send_bytes);
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

        queue_standby_->emplace_back(std::move(msg));
        return static_cast<int>(SendResult::PUSH_QUEUE);
    }

    int send_i(ZRSOCKET_IOVEC *iovecs, int iovecs_count, bool direct_send = true, int priority = 0, int flags = 0)
    {
        if (direct_send && queue_standby_->empty() && queue_active_->empty()) {
            int error_id = 0;
            int send_bytes = OSApi::socket_sendv(fd_, iovecs, iovecs_count, flags, nullptr, error_id);
            if (send_bytes > 0) {
                int i = 0;
                int iovec_remain_bytes = 0;
                int remain_bytes = send_bytes;
                for (; i < iovecs_count; ++i) {
                    iovec_remain_bytes = iovecs[i].iov_len - remain_bytes;
                    remain_bytes -= iovecs[i].iov_len;
                    if (remain_bytes <= 0) {
                        break;
                    }
                }
                if (i < iovecs_count) {
                    TSendBuffer buf(1024);
                    if (iovec_remain_bytes > 0) {
                        buf.write(iovecs[i].iov_base + iovec_remain_bytes, iovec_remain_bytes);
                    }
                    ++i;
                    for (; i < iovecs_count; ++i) {
                        buf.write(iovecs[i].iov_base, iovecs[i].iov_len);
                    }
                    queue_standby_->emplace_back(std::move(buf));
                    return static_cast<int>(SendResult::PUSH_QUEUE);
                }

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

        TSendBuffer buf(1024);
        for (int i = 0; i < iovecs_count; ++i) {
            buf.write(iovecs[i].iov_base, iovecs[i].iov_len);
        }
        queue_standby_->emplace_back(std::move(buf));
        return static_cast<int>(SendResult::PUSH_QUEUE);
    }

    int handle_write()
    {
        if (queue_active_->empty()) {
            mutex_.lock();
            if (!queue_standby_->empty()) {
                std::swap(queue_standby_, queue_active_);
                mutex_.unlock();
            }
            else {
                mutex_.unlock();
                event_loop_->delete_event(this, EventHandler::WRITE_EVENT_MASK);
                return EventHandler::WriteResult::WRITE_RESULT_NOT_DATA;
            }
        }

        int iovecs_count = 0;
        ZRSOCKET_IOVEC *iovecs = event_loop_->iovecs(iovecs_count);
        int queue_size = queue_active_->size();
        if (iovecs_count > queue_size) {
            iovecs_count = queue_size;
        }
        int iovecs_bytes = 0;
        auto iter = queue_active_->begin();
        for (int i = 0; i < iovecs_count; ++i) {
            iovecs[i].iov_len  = (*iter).data_size();
            iovecs[i].iov_base = (*iter).data();
            iovecs_bytes += iovecs[i].iov_len;
            ++iter;
        }

        //发送数据
        int error_id = 0;
        int send_bytes = OSApi::socket_sendv(fd_, iovecs, iovecs_count, 0, nullptr, error_id);
        if (send_bytes > 0) {
            EventHandler::WriteResult result;
            if (send_bytes == iovecs_bytes) {
                result = EventHandler::WriteResult::WRITE_RESULT_SUCCESS;
            }
            else {
                result = EventHandler::WriteResult::WRITE_RESULT_PART;
            }

            int data_size = 0;
            for (int i = 0; i < iovecs_count; ++i) {
                data_size = queue_active_->front().data_size();
                if (send_bytes >= data_size) {
                    queue_active_->pop_front();
                    send_bytes -= data_size;
                    if (send_bytes < 1) {
                        break;
                    }
                }
                else {
                    queue_active_->front().data_begin(queue_active_->front().data_begin() + send_bytes);
                    break;
                }
            }

            return result;
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
                last_errno_ = -error_id;
                event_loop_->delete_handler(this, 0);
                return EventHandler::WriteResult::WRITE_RESULT_FAILURE;
            }
        }
    }

protected:
    // These enumerations are used to describe when messages are delivered.
    enum MessagePriority
    {
        // Used by SocketLite to send above-high priority messages.
        SYSTEM_PRIORITY = 0,

        // High priority messages are send before medium priority messages.
        HIGH_PRIORITY,

        // Medium priority messages are send before low priority messages.
        MEDIUM_PRIORITY,

        // Low priority messages are only sent when no other messages are waiting.
        LOW_PRIORITY,

        // 内部使用
        NUMBER_OF_PRIORITIES
    };

    typedef TSendBuffer SendBuffer;

    //经测试发现: deque比list性能好不少
    typedef std::deque<TSendBuffer> SEND_QUEUE;
    //typedef std::list<TMessageBuffer> SEND_QUEUE;

    SEND_QUEUE      queue1_;
    SEND_QUEUE      queue2_;
    SEND_QUEUE     *queue_active_;
    SEND_QUEUE     *queue_standby_;
    TMutex          mutex_;

    ByteBuffer      message_buffer_;        //消息缓存
};

ZRSOCKET_NAMESPACE_END

#endif
