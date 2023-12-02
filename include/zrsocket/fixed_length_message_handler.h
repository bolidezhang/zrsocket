// Some compilers (e.g. VC++) benefit significantly from using this. 
// We've measured 3-4% build speed improvements in apps as a result 
#pragma once

#ifndef ZRSOCKET_FIXED_LENGTH_MESSAGE_HANDLER
#define ZRSOCKET_FIXED_LENGTH_MESSAGE_HANDLER
#include "message_handler.h"

ZRSOCKET_NAMESPACE_BEGIN

class FixedLengthMessageDecoderConfig : public MessageDecoderConfig
{
public:   
    FixedLengthMessageDecoderConfig() = default;
    virtual ~FixedLengthMessageDecoderConfig() = default;

    int update() 
    {
        return 0;
    }

    //消息长度
    uint_t message_length_ = sizeof(int);
};

template <class TSendBuffer, class TMutex>
class FixedLengthMessageHandler : public MessageHandler<TSendBuffer, TMutex>
{
public:
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

    virtual int do_message(const char *message, uint_t len)
    {
        return 0;
    }

    int handle_open()
    {
        super::message_buffer_.reset();
        super::message_buffer_.reserve(static_cast<FixedLengthMessageDecoderConfig *>(
            super::source_->message_decoder_config())->message_length_);
        super::queue1_.clear();
        super::queue2_.clear();
        return do_open();
    }

protected:
    int decode(const char *data, uint_t len)
    {
        uint_t  message_length = 
            static_cast<FixedLengthMessageDecoderConfig*>(super::source_->message_decoder_config())->message_length_;
        char   *remain         = const_cast<char *>(data);
        uint_t  remain_len     = len;

        if (super::message_buffer_.empty()) {
            while (remain_len >= message_length) {
                if (do_message(remain, message_length) >= 0) {
                    remain     += message_length;
                    remain_len -= message_length;
                }
                else {
                    super::last_errno_ = EventHandler::ERROR_CLOSE_ACTIVE;
                    return -1;
                }
            }
            if (remain_len > 0) {
                super::message_buffer_.write(remain, remain_len);
            }
        }
        else {
            uint_t need_len = message_length - super::message_buffer_.data_size();
            if (remain_len >= need_len) {
                super::message_buffer_.write(remain, need_len);
                if (do_message(super::message_buffer_.data(), message_length) >= 0) {
                    remain     += need_len;
                    remain_len -= need_len;
                    super::message_buffer_.reset();
                }
                else {
                    super::last_errno_ = EventHandler::ERROR_CLOSE_ACTIVE;
                    return -1;
                }

                while (remain_len >= message_length) {
                    if (do_message(remain, message_length) >= 0) {
                        remain     += message_length;
                        remain_len -= message_length;
                    }
                    else {
                        super::last_errno_ = EventHandler::ERROR_CLOSE_ACTIVE;
                        return -1;
                    }
                }
                if (remain_len > 0) {
                    super::message_buffer_.write(remain, remain_len);
                }
            }
            else {
                super::message_buffer_.write(remain, remain_len);
            }
        }

        return 0;
    }

protected:
    using super = MessageHandler<TSendBuffer, TMutex>;
};

ZRSOCKET_NAMESPACE_END

#endif
