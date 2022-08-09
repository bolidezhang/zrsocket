#pragma once

#ifndef ZRSOCKET_HTTP_RESPONSE_HANDLER_H
#define ZRSOCKET_HTTP_RESPONSE_HANDLER_H

#include <algorithm>
#include "config.h"
#include "base_type.h"
#include "byte_buffer.h"
#include "message_handler.h"
#include "http_common.h"

ZRSOCKET_NAMESPACE_BEGIN

template <class TBuffer, class TMutex>
class HttpResponseHandler : public MessageHandler<TBuffer, TMutex>
{
public:
    using super = MessageHandler<ByteBuffer, TMutex>;

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

    //返回值
    // > 0: 手动回复
    //== 0: 自动回复
    // < 0: 出现异常关闭连接
    virtual int do_message()
    {
        return 0;
    }

    int handle_open()
    {
        HttpDecoderConfig *config = static_cast<HttpDecoderConfig *>(super::source_->message_decoder_config());
        header_length_ = 0;
        decode_state_ = HttpDecodeState::kVersion;
        line_state_   = HttpLineState::kNormal;
        field1_.clear();
        field2_.clear();
        field1_.reserve(config->max_uri_length_);
        field2_.reserve(config->max_uri_length_);
        response_.init();
        response_.event_handler_ = this;
        super::queue1_.clear();
        super::queue2_.clear();
        return do_open();
    }

protected:
    inline int decode_reset()
    {
        int ret = do_message();
        if (ret >= 0) {
            if (0 == ret) {
                response_.reset();
            }
            else {
                response_.init();
            }

            header_length_  = 0;
            decode_state_   = HttpDecodeState::kVersion;
            line_state_     = HttpLineState::kNormal;
            field1_.clear();
            field2_.clear();
        }

        return ret;
    }

    int decode(const char *data, uint_t len)
    {
        HttpResponse &response = response_;
        HttpDecoderConfig *config = static_cast<HttpDecoderConfig *>(super::source_->message_decoder_config());
        const char *data_end = data + len;
        char ch;
        uint_t remain_len;
        uint_t need_len;

        do {
            if (decode_state_ != HttpDecodeState::kBody) { //begin: parse header
                for (; data < data_end; ++data) {
                    if (++header_length_ >= config->max_header_length_) {
                        return -1;
                    }

                    ch = *data;
                    switch (decode_state_) {
                    case HttpDecodeState::kVersion:
                        if (ch != ' ') {
                            field1_.push_back(ch);
                        }
                        else {
                            response.version_id_ = HttpMessage::find_http_version_id(field1_);
                            if (HttpVersionId::kUNKNOWN != response.version_id_) {
                                decode_state_ = HttpDecodeState::kStatusCode;
                                field1_.clear();
                            }
                            else {
                                return -1;
                            }
                        }
                        break;
                    case HttpDecodeState::kStatusCode:
                        if (ch != ' ') {
                            field1_.push_back(ch);
                        }
                        else {
                            response.status_code_ = static_cast<HttpStatusCode>(std::atoi(field1_.c_str()));
                            decode_state_ = HttpDecodeState::kReasonPhrase;
                        }
                        break;
                    case HttpDecodeState::kReasonPhrase:
                        switch (line_state_) {
                        case HttpLineState::kNormal:
                            if (ch != '\r') {
                                response.reason_phrase_.push_back(ch);
                            }
                            else {
                                line_state_ = HttpLineState::kCR;
                            }
                            break;
                        case HttpLineState::kCR:
                            if (ch == '\n') {
                                line_state_ = HttpLineState::kCRLF;
                            }
                            else {
                                return -1;
                            }
                            break;
                        case HttpLineState::kCRLF:
                            if (ch != '\r') {
                                field1_.clear();
                                field1_.push_back(ch);
                                decode_state_ = HttpDecodeState::kHeaderField;
                                line_state_ = HttpLineState::kNormal;
                            }
                            else {
                                line_state_ = HttpLineState::kCRLFCR;
                            }
                            break;
                        case HttpLineState::kCRLFCR:
                            if (ch == '\n') {
                                if (decode_reset() < 0) {
                                    return -1;
                                }
                            }
                            else {
                                return -1;
                            }
                            break;
                        default:
                            break;
                        }
                        break;
                    case HttpDecodeState::kHeaderField:
                        if (ch != ':') {
                            field1_.push_back(ch);
                        }
                        else {
                            decode_state_ = HttpDecodeState::kHeaderValue;
                            field2_.clear();
                        }
                        break;
                    case HttpDecodeState::kHeaderValue:
                        switch (line_state_) {
                        case HttpLineState::kNormal:
                            if (ch != '\r') {
                                field2_.push_back(ch);
                            }
                            else {
                                response.headers_.emplace(std::move(field1_), std::move(field2_));
                                line_state_ = HttpLineState::kCR;
                                field1_.reserve(config->max_uri_length_);
                                field2_.reserve(config->max_uri_length_);
                            }
                            break;
                        case HttpLineState::kCR:
                            if (ch == '\n') {
                                line_state_ = HttpLineState::kCRLF;
                            }
                            else {
                                return -1;
                            }
                            break;
                        case HttpLineState::kCRLF:
                            if (ch != '\r') {
                                field1_.push_back(ch);
                                decode_state_ = HttpDecodeState::kHeaderField;
                                line_state_ = HttpLineState::kNormal;
                            }
                            else {
                                line_state_ = HttpLineState::kCRLFCR;
                            }
                            break;
                        case HttpLineState::kCRLFCR:
                            if (ch == '\n') {
                                if (response.update() < 0) {
                                    return -1;
                                }

                                if (response.content_length_ > 0) {
                                    if (response.content_length_ > config->max_body_length_) {
                                        return -1;
                                    }
                                    decode_state_ = HttpDecodeState::kBody;
                                    line_state_   = HttpLineState::kNormal;
                                    ++data;
                                    goto PARSE_BODY;
                                }
                                else { //response.content_length_ == 0
                                    if (decode_reset() < 0) {
                                        return -1;
                                    }
                                }
                            }
                            else {
                                return -1;
                            }
                            break;
                        default:
                            break;
                        }
                        break;
                    default:
                        break;
                    }

                }
            } //end: parse header
            else { //begin: parse body

            PARSE_BODY:
                remain_len = static_cast<uint_t>(data_end - data);
                if (remain_len > 0) {
                    if (response.body_.empty() && remain_len >= response.content_length_) {
                        response.body_ptr_ = const_cast<char *>(data);
                        data += response.content_length_;
                        if (decode_reset() < 0) {
                            return -1;
                        }
                    }
                    else {
                        need_len = std::min<uint_t>(remain_len, (response.content_length_ - response.body_.data_size()));
                        response.body_.write(data, need_len);
                        data += need_len;
                        if (response.body_.data_size() == response.content_length_) {
                            response.body_ptr_ = response.body_.data();
                            if (decode_reset() < 0) {
                                return -1;
                            }
                        }
                    }
                }
                else {
                    break;
                }
            } //end: parse body

        } while (data < data_end);

        return 1;
    }

protected:
    uint_t          header_length_  = 0;
    HttpDecodeState decode_state_   = HttpDecodeState::kVersion;
    HttpLineState   line_state_     = HttpLineState::kNormal;
    std::string     field1_;
    std::string     field2_;
    HttpResponse    response_;
};

ZRSOCKET_NAMESPACE_END

#endif
