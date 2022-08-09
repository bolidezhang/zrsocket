// Some compilers (e.g. VC++) benefit significantly from using this. 
// We've measured 3-4% build speed improvements in apps as a result 
#pragma once

#ifndef ZRSOCKET_LENGTH_FIELD_MESSAGE_HANDLER
#define ZRSOCKET_LENGTH_FIELD_MESSAGE_HANDLER
#include "message_common.h"
#include "message_handler.h"
#include "os_api.h"
#include "os_constant.h"

ZRSOCKET_NAMESPACE_BEGIN

class LengthFieldMessageDecoderConfig : public MessageDecoderConfig
{
public:
    LengthFieldMessageDecoderConfig() = default;
    virtual ~LengthFieldMessageDecoderConfig() = default;

    int update()
    {
        min_head_length_ = length_field_offset_ + length_field_length_;

        if (ByteOrder::BYTE_ORDER_UNKNOWN_ENDIAN == byte_order_) {
            byte_order_ = OSConstant::instance().byte_order();
        }
        if (ByteOrder::BYTE_ORDER_BIG_ENDIAN == byte_order_) {
            switch (length_field_length_) {
            case 1:
                read_length_field_proc_ = MessageCommon::read_length_field_int8;
                break;
            case 2:
                read_length_field_proc_ = MessageCommon::read_length_field_int16_network;
                break;
            case 4:
                read_length_field_proc_ = MessageCommon::read_length_field_int32_network;
                break;
            default:
                read_length_field_proc_ = MessageCommon::read_length_field_int16_network;
                break;
            }
        }
        else {
            switch (length_field_length_) {
            case 1:
                read_length_field_proc_ = MessageCommon::read_length_field_int8;
                break;
            case 2:
                read_length_field_proc_ = MessageCommon::read_length_field_int16_host;
                break;
            case 4:
                read_length_field_proc_ = MessageCommon::read_length_field_int32_host;
                break;
            default:
                read_length_field_proc_ = MessageCommon::read_length_field_int16_host;
                break;
            }
        }

        return 0;
    }

public:
    //长度字段偏移
    uint_t length_field_offset_ = 0;
    //长度字段长度
    uint_t length_field_length_ = 2;
    //最大消息长度
    uint_t max_message_length_  = 4096;
    //长度字段的字节序
    ByteOrder byte_order_       = ByteOrder::BYTE_ORDER_UNKNOWN_ENDIAN;
    //读长度字段函数指针
    MessageCommon::read_length_field_proc read_length_field_proc_ = MessageCommon::read_length_field_int16_host;
    //最小头长度
    uint_t min_head_length_     = 2;
};

template <class TSendBuffer, class  TMutex>
class LengthFieldMessageHandler : public MessageHandler<TSendBuffer, TMutex>
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
        super::message_buffer_.reserve(static_cast<LengthFieldMessageDecoderConfig *>(super::source_->message_decoder_config())->max_message_length_);
        super::queue1_.clear();
        super::queue2_.clear();
        message_length_ = 0;
        return do_open();
    }

protected:
    int decode(const char *data, uint_t len)
    {
        LengthFieldMessageDecoderConfig *config = static_cast<LengthFieldMessageDecoderConfig *>(super::source_->message_decoder_config());
        char   *remain     = const_cast<char *>(data);
        uint_t  remain_len = len;
        uint_t  last_len;
        uint_t  need_len;

        do {
            if (super::message_buffer_.empty()) {
                if (0 == message_length_) {
                    if (remain_len >= config->min_head_length_) {
                        message_length_ = config->read_length_field_proc_(remain + config->length_field_offset_);
                        if ((message_length_ > config->max_message_length_) || (0 == message_length_)) {
                            super::last_errno_ = EventHandler::ERROR_CLOSE_ACTIVE;
                            return -1;
                        }
                    }
                    else {
                        super::message_buffer_.write(remain, remain_len);
                        return 0;
                    }
                }

                if (remain_len >= message_length_) {
                    if (do_message(remain, message_length_) >= 0) {
                        remain         += message_length_;
                        remain_len     -= message_length_;
                        message_length_ = 0;
                    }
                    else {
                        super::last_errno_ = EventHandler::ERROR_CLOSE_ACTIVE;
                        return -1;
                    }
                }
                else {
                    super::message_buffer_.write(remain, remain_len);
                    return 0;
                }
            }
            else { //begin: message_buffer_.data_size() > 0
                last_len = super::message_buffer_.data_size();
                if (message_length_ > 0) {
                    need_len = message_length_ - last_len;
                    if (remain_len >= need_len) {
                        super::message_buffer_.write(remain, need_len);
                        if (do_message(super::message_buffer_.data(), message_length_) >= 0) {
                            remain         += need_len;
                            remain_len     -= need_len;
                            message_length_ = 0;
                            super::message_buffer_.reset();
                        }
                        else {
                            super::last_errno_ = EventHandler::ERROR_CLOSE_ACTIVE;
                            return -1;
                        }
                    }
                    else {
                        super::message_buffer_.write(remain, remain_len);
                        return 0;
                    }                
                }
                else { //begin: message_length_ == 0
                    need_len = config->min_head_length_ - last_len;
                    if (remain_len >= need_len) {
                        super::message_buffer_.write(remain, need_len);
                        message_length_ = config->read_length_field_proc_(super::message_buffer_.data() + config->length_field_offset_);
                        if ((message_length_ > config->max_message_length_) || (0 == message_length_)) {
                            super::last_errno_ = EventHandler::ERROR_CLOSE_ACTIVE;
                            return -1;
                        }

                        remain     += need_len;
                        remain_len -= need_len;
                    }
                    else {
                        super::message_buffer_.write(remain, remain_len);
                        return 0;
                    }
                } //end: message_length_ == 0
            } //end: message_buffer_.data_size() > 0
        } while (remain_len > 0);

        return 0;
    }

protected:
    using super = MessageHandler<TSendBuffer, TMutex>;
    uint_t message_length_ = 0;
};

ZRSOCKET_NAMESPACE_END

#endif
