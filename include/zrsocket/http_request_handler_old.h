#ifndef ZRSOCKET_HTTP_REQUEST_HANDLER_H
#define ZRSOCKET_HTTP_REQUEST_HANDLER_H
#include <algorithm>
#include "config.h"
#include "base_type.h"
#include "byte_buffer.h"
#include "message_handler.h"
#include "http_common.h"

ZRSOCKET_BEGIN

template <class TBuffer, class TMutex>
class HttpRequestHandler : public MessageHandler<TBuffer, TMutex>
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

    int send(HttpContext &context, ByteBuffer &out, bool out_owned = true)
    {
        HttpRequest  &request  = context.request_;
        HttpResponse &response = context.response_;
        out.reset();
        out.reserve(1024);

        const std::string &http_version = find_http_version_name(response.version_id_);
        const std::string &status_desc  = find_http_status_description(response.status_code_);
        int ret = std::snprintf(out.data(), 
            out.free_size()-1,
            "%s %d %s\r\n",
            http_version.c_str(),
            response.status_code_,
            status_desc.c_str());
        out.data_end(static_cast<uint_t>(ret));

        for (auto &iter : response.headers_) {
            out.write(iter.first.c_str(), iter.first.size());
            out.write(": ", 2);
            out.write(iter.second.c_str(), iter.second.size());
            out.write("\r\n", 2);
        }

        if (request.method_id_ != HttpMethodId::kHEAD) { // begin: HttpMethod != HEAD
            uint_t body_size = response.body_.data_size();
            if (body_size > 0) {
                out.reserve(out.data_size() + 32);
                ret = std::snprintf(out.data() + out.data_size(),
                    out.free_size()-1,
                    "Content-Length: %u\r\n\r\n",
                    body_size);
                out.data_end(static_cast<uint_t>(out.data_size() + ret));

                if (body_size < 1024) {
                    out.write(response.body_.data(), body_size);
                    if (!out_owned) {
                        super::send(out.data(), out.data_size());
                    }
                    else {
                        super::send(out);
                    }
                }
                else {
                    if (!out_owned) {
                        super::send(out.data(), out.data_size(), false);
                        super::send(response.body_, false);
                    }
                    else {
                        super::send(out, false);
                        super::send(response.body_, false);
                    }
                }
            }
            else {
                if ((response.status_code_ < HttpStatusCode::k200) ||
                    (response.status_code_ == HttpStatusCode::k204) ||
                    (response.status_code_ == HttpStatusCode::k304)) {
                    out.write("\r\n", 2);
                }
                else {
                    out.write("Content-Length: 0\r\n\r\n", sizeof("Content-Length: 0\r\n\r\n")-1);
                }
                super::send(out.data(), out.data_size());
            }

        } // end: HttpMethod != HEAD
        else { //begin: HttpMethod == HEAD

            uint_t real_body_size = std::max<uint_t>(response.body_.data_size(), response.content_length_);
            if (real_body_size > 0) {
                out.reserve(out.data_size() + 32);
                ret = std::snprintf(out.data() + out.data_size(),
                    out.free_size() - 1,
                    "Content-Length: %u\r\n\r\n",
                    real_body_size);
                out.data_end(static_cast<uint_t>(out.data_size() + ret));
            }
            else {
                out.write("\r\n", 2);
            }
            super::send(out.data(), out.data_size());

        } //end: HttpMethod == HEAD

        return 0;
    }

    int handle_open()
    {
        HttpDecoderConfig *config = static_cast<HttpDecoderConfig *>(super::source_->message_decoder_config());
        header_length_ = 0;
        decode_state_ = HttpDecodeState::kMethod;
        line_state_   = HttpLineState::kNormal;
        field1_.clear();
        field2_.clear();
        field1_.reserve(config->max_uri_length_);
        field2_.reserve(config->max_uri_length_);
        context_.init();
        context_.response_.event_handler_ = this;
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
                send(context_, *super::event_loop_->get_send_buffer(), false);
                context_.reset();
            }
            else {
                context_.init();
            }

            header_length_  = 0;
            decode_state_   = HttpDecodeState::kMethod;
            line_state_     = HttpLineState::kNormal;
            field1_.clear();
            field2_.clear();
        }

        return ret;
    }

    int decode(const char *data, uint_t len)
    {
        HttpRequest  &request  = context_.request_;
        HttpResponse &response = context_.response_;
        HttpDecoderConfig *config = static_cast<HttpDecoderConfig *>(super::source_->message_decoder_config());
        const char *data_end = data + len;

        do {
            if (decode_state_ != HttpDecodeState::kBody) { //begin: parse header
                for (; data < data_end; ++data) {
                    if (++header_length_ >= config->max_header_length_) {
                        return -1;
                    }

                    const char ch = *data;
                    switch (decode_state_) {
                    case HttpDecodeState::kMethod:
                        if (ch != ' ') {
                            field1_.push_back(ch);
                        }
                        else {
                            request.method_id_ = find_http_method_id(field1_);
                            if (HttpMethodId::kUNKNOWN != request.method_id_) {
                                decode_state_ = HttpDecodeState::kUri;
                                field1_.clear();
                            }
                            else {
                                return -1;
                            }
                        }
                        break;
                    case HttpDecodeState::kUri:
                        if (ch != ' ') {
                            request.uri_.push_back(ch);
                        }
                        else {
                            decode_state_ = HttpDecodeState::kVersion;
                        }
                        break;
                    case HttpDecodeState::kVersion:
                        switch (line_state_) {
                        case HttpLineState::kNormal:
                            if (ch != '\r') {
                                field1_.push_back(ch);
                            }
                            else {
                                request.version_id_ = find_http_version_id(field1_);
                                if (HttpVersionId::kUNKNOWN != request.version_id_) {
                                    response.version_id_ = request.version_id_;
                                    line_state_ = HttpLineState::kCR;
                                    field1_.clear();
                                }
                                else {
                                    return -1;
                                }
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
                                decode_state_ = HttpDecodeState::kHeaderKey;
                                line_state_   = HttpLineState::kNormal;
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
                    case HttpDecodeState::kHeaderKey:
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
                                request.headers_.emplace(std::move(field1_), std::move(field2_));
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
                                decode_state_   = HttpDecodeState::kHeaderKey;
                                line_state_     = HttpLineState::kNormal;
                            }
                            else {
                                line_state_ = HttpLineState::kCRLFCR;
                            }
                            break;
                        case HttpLineState::kCRLFCR:
                            if (ch == '\n') {
                                if (context_.update() < 0) {
                                    return -1;
                                }

                                if (request.content_length_ > 0) {
                                    if (request.content_length_ > config->max_body_length_) {
                                        return -1;
                                    }
                                    decode_state_ = HttpDecodeState::kBody;
                                    line_state_   = HttpLineState::kNormal;
                                    ++data;
                                    goto PARSE_BODY;
                                }
                                else { //request.content_length_ == 0
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
                uint_t remain_len = static_cast<uint_t>(data_end - data);
                if (remain_len > 0) {
                    if (request.body_.empty() && remain_len >= request.content_length_) {
                        request.body_ptr_ = const_cast<char *>(data);
                        data += request.content_length_;
                        if (decode_reset() < 0) {
                            return -1;
                        }
                    }
                    else {
                        uint_t need_len = std::min<uint_t>(remain_len, (request.content_length_ - request.body_.data_size()));
                        request.body_.write(data, need_len);
                        data += need_len;
                        if (request.body_.data_size() == request.content_length_) {
                            request.body_ptr_ = request.body_.data();
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
    HttpDecodeState decode_state_   = HttpDecodeState::kMethod;
    HttpLineState   line_state_     = HttpLineState::kNormal;
    std::string     field1_;
    std::string     field2_;
    HttpContext     context_;
};

ZRSOCKET_END

#endif
