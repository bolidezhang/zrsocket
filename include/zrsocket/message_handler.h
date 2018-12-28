#ifndef ZRSOCKET_MESSAGE_HANDLER_H_
#define ZRSOCKET_MESSAGE_HANDLER_H_
#include <deque>
#include "config.h"
#include "byte_buffer.h"
#include "mutex.h"
#include "system_api.h"
#include "event_handler.h"
#include "event_source.h"

//ZRSOCKET_Message_Handler所处理数据包的格式: 数据包长度+数据包内容
//类似于结构
//struct ZRSOCKET_Message
//{
//    ushort len;
//    char   content[0];
//}

ZRSOCKET_BEGIN

template <typename TMessageBuffer, typename TMutex>
class MessageHandler : public EventHandler
{
public:
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

    enum WRITE_RETURN_VALUE
    {
        WRITE_RETURN_NOT_INITIAL    = -3,   //没初始化
        WRITE_RETURN_STATUS_INVALID = -2,   //表示Socket状态失效
        WRITE_RETURN_SEND_FAIL      = -1,   //表示发送失败(Socket已失效)
        WRITE_RETURN_NOT_DATA       = 0,    //表示没有数据可发送 
        WRITE_RETURN_SEND_SUCCESS   = 1,    //表示发送成功
        WRITE_RETURN_SEND_PART      = 2     //表示发送出部分数据(包含0长度)
    };

    MessageHandler()
        : need_len_(0)
        , last_error_msglen_(0)
        , last_left_(0)
    {
        queue_active_   = &queue1_;
        queue_standby_  = &queue2_;
    }

    virtual ~MessageHandler()
    {
    }

    int handle_open()
    {
        message_buffer_.reserve(source_->get_msgbuffer_size());
        message_buffer_.reset();
        need_len_           = 0;
        last_error_msglen_  = 0;
        last_left_          = 0;
        queue1_.clear();
        queue2_.clear();
        return 0;
    }

    //接收到完整消息时,回调此函数
    virtual int handle_message(const char *message, uint_t len)
    {
        return 0;
    }

    int push_message(uint64_t handler_id, const char *message, uint_t len, bool direct_send = false, int priority = 0, int flags = 0)
    {
        if (handler_id_ == handler_id) {
            mutex_.lock();
            int ret = push_message_i(message, len, priority, flags);
            mutex_.unlock();
            return ret;
        }
        return -1;
    }

    int push_message(uint64_t handler_id, TMessageBuffer &message, bool direct_send = false, int priority = 0, int flags = 0)
    {
        if (handler_id_ == handler_id) {
            mutex_.lock();
            int ret = push_message_i(message, priority, flags);
            mutex_.unlock();
            return ret;
        }
        return -1;
    }

protected:
    int handle_read()
    {
        int   msglen_bytes = source_->get_msglen_bytes();
        int   msgbuffer_size = source_->get_msgbuffer_size();
        int   recv_buffer_size = reactor_->get_recv_buffer_size();
        char *recv_buffer = reactor_->get_recv_buffer();

        char *pos;
        int   pos_size;
        int   msglen;
        int   ret;
        int   error_id;

        do {
            ret = SystemApi::socket_recv(socket_, recv_buffer, recv_buffer_size, 0, NULL, error_id);
            if (ret > 0) {
                pos = recv_buffer;
                pos_size = ret;
                do {
                    if (0 == need_len_) {
                        if (0 == last_left_) {
                            if (pos_size >= msglen_bytes) {
                                msglen = source_->get_msglen_proc_(pos, pos_size);
                                if ((msglen <= 0) || (msglen > msgbuffer_size)) {
                                    //消息大小无效或超过消息缓冲大小
                                    last_errno_ = EventHandler::ERROR_CLOSE_RECV;
                                    last_error_msglen_ = msglen;
                                    return -1;
                                }
                                if (pos_size >= msglen) {
                                    if (handle_message(pos, msglen) < 0) {
                                        last_errno_ = EventHandler::ERROR_CLOSE_ACTIVE;
                                        return -1;
                                    }
                                    pos += msglen;
                                    pos_size -= msglen;
                                }
                                else {
                                    message_buffer_.write(pos, pos_size);
                                    need_len_ = msglen - pos_size;
                                    break;
                                }
                            }
                            else {
                                message_buffer_.write(pos, pos_size);
                                last_left_ = pos_size;
                                break;
                            }
                        }
                        else {
                            //last_left_>0
                            if (last_left_ + pos_size < msglen_bytes) {
                                message_buffer_.write(pos, pos_size);
                                last_left_ += pos_size;
                                break;
                            }
                            else {
                                message_buffer_.write(pos, msglen_bytes - last_left_);
                                pos += (msglen_bytes - last_left_);
                                pos_size -= (msglen_bytes - last_left_);
                                msglen = source_->get_msglen_proc_(message_buffer_.data(), msglen_bytes);
                                if ((msglen <= 0) || (msglen > msgbuffer_size)) {
                                    //消息大小无效或超过消息缓冲大小
                                    last_errno_ = EventHandler::ERROR_CLOSE_RECV;
                                    last_error_msglen_ = msglen;
                                    return -1;
                                }
                                last_left_ = 0;

                                if (pos_size + msglen_bytes >= msglen)
                                {
                                    message_buffer_.write(pos, msglen - msglen_bytes);
                                    if (handle_message(message_buffer_.data(), msglen) < 0)
                                    {
                                        last_errno_ = EventHandler::ERROR_CLOSE_ACTIVE;
                                        return -1;
                                    }
                                    message_buffer_.reset();
                                    pos += (msglen - msglen_bytes);
                                    pos_size -= (msglen - msglen_bytes);
                                }
                                else
                                {
                                    message_buffer_.write(pos, pos_size);
                                    need_len_ = msglen - pos_size - msglen_bytes;
                                    break;
                                }
                            }
                        }
                    }
                    else {
                        //need_size_>0
                        if (pos_size >= need_len_) {
                            message_buffer_.write(pos, need_len_);
                            if (handle_message(message_buffer_.data(), message_buffer_.data_size()) < 0) {
                                last_errno_ = EventHandler::ERROR_CLOSE_ACTIVE;
                                return -1;
                            }
                            message_buffer_.reset();
                            pos += need_len_;
                            pos_size -= need_len_;
                            need_len_ = 0;
                        }
                        else {
                            message_buffer_.write(pos, pos_size);
                            need_len_ -= pos_size;
                            break;
                        }
                    }
                } while (pos_size > 0);
            }
            else if (ret == 0) {
                //连接已关闭
                last_errno_ = EventHandler::ERROR_CLOSE_PASSIVE;
                return -1;
            }
            else {
                //ret < 0: 出现异常(如连接已关闭)
                if ((ZRSOCKET_EAGAIN == error_id) ||
                    (ZRSOCKET_EWOULDBLOCK == error_id) ||
                    (ZRSOCKET_EINTR == error_id)) {
                    //非阻塞模式下正常情况
                    return 0;
                }
                last_errno_ = error_id;
                return -1;
            }
        } while (ret == recv_buffer_size);

        return 1;
    }

    int handle_write()
    {
        return write_data();
    }

    int push_message_i(const char *message, uint_t len, bool direct_send = false, int priority = 0, int flags = 0)
    {
        if (direct_send) {
            if (queue_standby_->empty() && queue_active_->empty()) {
                //直接发送数据
                int error_id = 0;
                int send_bytes = SystemApi::socket_send(socket_, message, len, 0, NULL, &error_id);
                if (send_bytes > 0) {
                    if (send_bytes == len) {
                        return 1;
                    }

                    TMessageBuffer tmsg(len - send_bytes);
                    tmsg.write(message + send_bytes, len - send_bytes);
                    queue_standby_->emplace_back(tmsg);
                    return 0;
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
                        reactor_->del_handler(this, 0);
                        return -1;
                    }
                }
            }
        }
        else {
            TMessageBuffer tmsg(len);
            tmsg.write(message, len);
            queue_standby_->emplace_back(tmsg);
            reactor_->add_event(this, EventHandler::WRITE_EVENT_MASK);
        }

        return 0;
    }

    int push_message_i(TMessageBuffer &message, bool direct_send = false, int priority = 0, int flags = 0)
    {
        if (direct_send) {
            if (queue_standby_->empty() && queue_active_->empty()) {
                char  *msg = message.data();
                uint_t len = message.data_size();
                int  error_id = 0;
                int  send_bytes = SystemApi::socket_send(socket_, msg, len, 0, NULL, &error_id);
                if (send_bytes > 0) {
                    if (send_bytes == len) {
                        return 1;
                    }
                    message.data_begin(message.data_begin() + send_bytes);
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
                        reactor_->del_handler(this, 0);
                        return -1;
                    }
                }
            }
        }
        else {
            queue_standby_->emplace_back(message);
            reactor_->add_event(this, EventHandler::WRITE_EVENT_MASK);
        }

        return 0;
    }

    int write_data()
    {
        if (queue_active_->empty())  {
            mutex_.lock();
            if (!queue_standby_->empty()) {
                std::swap(queue_standby_, queue_active_);
                mutex_.unlock();
            }
            else {
                queue_standby_->shrink_to_fit();
                queue_active_->shrink_to_fit();
                mutex_.unlock();
                reactor_->del_event(this, EventHandler::WRITE_EVENT_MASK);
                return WRITE_RETURN_NOT_DATA;
            }
        }

        ZRSOCKET_IOVEC *iovec = reactor_->get_iovec();
        int iovec_count = reactor_->get_iovec_count();
        int queue_size = queue_active_->size();
        int node_count = (queue_size < iovec_count) ? queue_size : iovec_count;
        auto iter = queue_active_->begin();
        for (int i = 0; i < node_count; ++i) {
            iovec[i].iov_len  = (*iter).data_size();
            iovec[i].iov_base = (*iter).data();
            ++iter;
        }

        //发送数据
        int error_id = 0;
        int send_bytes = SystemApi::socket_sendv(socket_, iovec, node_count, 0, NULL, error_id);
        if (send_bytes > 0) {
            int data_size;
            int i = 1;
            for (; i <= node_count; ++i) {
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

            WRITE_RETURN_VALUE ret;
            if (i == node_count) {
                ret = WRITE_RETURN_SEND_SUCCESS;
            }
            else {
                ret = WRITE_RETURN_SEND_PART;
            }
            return ret;
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
                reactor_->del_handler(this, 0);
                return WRITE_RETURN_SEND_FAIL;
            }
        }
        return WRITE_RETURN_NOT_DATA;
    }

protected:
    typedef std::deque<TMessageBuffer> SEND_QUEUE;

    int             need_len_;              //组成一个消息包，还需要长度
    int             last_error_msglen_;     //组包出错时数据包长度
    int8_t          last_left_;             //当收到的数据长度小于消息len域所占长度时有效，一般为0

    ByteBuffer      message_buffer_;        //消息缓存
    SEND_QUEUE      queue1_;
    SEND_QUEUE      queue2_;
    SEND_QUEUE     *queue_active_;
    SEND_QUEUE     *queue_standby_;

    TMutex          mutex_;
};

ZRSOCKET_END

#endif
