// Some compilers (e.g. VC++) benefit significantly from using this. 
// We've measured 3-4% build speed improvements in apps as a result 
#pragma once

#ifndef ZRSOCKET_HTTP_COMMON_H
#define ZRSOCKET_HTTP_COMMON_H
#include <unordered_map>
#include <map>
#include <string>
#include "config.h"
#include "base_type.h"
#include "byte_buffer.h"
#include "event_handler.h"

ZRSOCKET_NAMESPACE_BEGIN

enum class HttpSpecialChar
{
    kNull  = '\0',     //0  0x00 空字符 Null C风格的字符串的结束符
    kLF    = '\n',     //10 0x0A 换行   Linefeed(line feed, new line)
    kCR    = '\r',     //13 0x0D 回车   Carriage Return (Enter key)
    kSpace = ' ',      //32 0x20 空格   Space
    kColon = ':',      //58 0x3A 冒号   Colon
};

enum class HttpDecodeState
{
    kMethod = 1,
    kUri,
    kVersion,
    kStatusCode,
    kReasonPhrase,
    kHeaderField,
    kHeaderValue,
    kBody,
};

enum class HttpLineState
{
    kNormal = 1,
    kCR = 2,
    kCRLF,
    kCRLFCR,
    kCRLFCRLF,
};

enum class HttpMethodId
{
    kUNKNOWN = 0,
    kOPTIONS = 1,
    kGET,
    kHEAD,
    kPOST,
    kPUT,
    kDELETE,
    kTRACE,
    kCONNECT
};

enum class HttpVersionId
{
    kUNKNOWN = 0,
    kHTTP09 = 1,
    kHTTP10,
    kHTTP11
};

enum class HttpStatusCode
{
    k100 = 100,     //Continue
    k101 = 101,     //Switching Protocols

    k200 = 200,     //OK
    k201 = 201,     //Created
    k202 = 202,     //Accepted
    k203 = 203,     //Non-Authoritative Information
    k204 = 204,     //No Content
    k205 = 205,     //Reset Content
    k206 = 206,     //Partial Content

    k300 = 300,     //Multiple Choices
    k301 = 301,     //Moved Permanently
    k302 = 302,     //Found(old version: Moved Temporarily)
    k303 = 303,     //See Other
    k304 = 304,     //Not Modified
    k305 = 305,     //Use Proxy
    k306 = 306,     //(Unused)
    k307 = 307,     //Temporary Redirect

    k400 = 400,     //Bad Request
    k401 = 401,     //Unauthorized
    k402 = 402,     //Payment Required
    k403 = 403,     //Forbidden
    k404 = 404,     //Not Found
    k405 = 405,     //Method Not Allowed
    k406 = 406,     //Not Acceptable
    k407 = 407,     //Proxy Authentication Required
    k408 = 408,     //Request Timeout
    k409 = 409,     //Conflict
    k410 = 410,     //Gone
    k411 = 411,     //Length Required
    k412 = 412,     //Precondition Failed
    k413 = 413,     //Request Entity Too Large
    k414 = 414,     //Request-URI Too Long
    k415 = 415,     //Unsupported Media Type
    k416 = 416,     //Requested Range Not Satisfiable
    k417 = 417,     //Expectation Failed

    k500 = 500,     //Internal Server Error
    k501 = 501,     //Not Implemented 
    k502 = 502,     //Bad Gateway
    k503 = 503,     //Service Unavailable
    k504 = 504,     //Gateway Timeout
    k505 = 505,     //HTTP Version Not Supported
};

//http null string
static const std::string http_null_string_ = "";

//HttpMethods
//key:value <HttpMethodName, HttpMethodId>
static const std::unordered_map<std::string, HttpMethodId> http_methods_name2id_ = {
    {"OPTIONS", HttpMethodId::kOPTIONS},
    {"GET",     HttpMethodId::kGET},
    {"HEAD",    HttpMethodId::kHEAD},
    {"POST",    HttpMethodId::kPOST},
    {"PUT",     HttpMethodId::kPUT},
    {"DELETE",  HttpMethodId::kDELETE},
    {"TRACE",   HttpMethodId::kTRACE},
    {"CONNECT", HttpMethodId::kCONNECT},
};

//key:value <HttpMethodId, HttpMethodName>
static const std::unordered_map<HttpMethodId, std::string> http_methods_id2name_ = {
    {HttpMethodId::kOPTIONS, "OPTIONS"},
    {HttpMethodId::kGET,     "GET"},
    {HttpMethodId::kHEAD,    "HEAD"},
    {HttpMethodId::kPOST,    "POST"},
    {HttpMethodId::kPUT,     "PUT"},
    {HttpMethodId::kDELETE,  "DELETE"},
    {HttpMethodId::kTRACE,   "TRACE"},
    {HttpMethodId::kCONNECT, "CONNECT"},
};

//HttpVersions name2id
//key:value <HttpVersionName, HttpVersionId>
static const std::unordered_map<std::string, HttpVersionId> http_versions_name2id_ = {
    {"HTTP/0.9", HttpVersionId::kHTTP09},
    {"HTTP/1.0", HttpVersionId::kHTTP10},
    {"HTTP/1.1", HttpVersionId::kHTTP11},
};

//HttpVersions id2name
//key:value <HttpVersionId, HttpVersionName>
static const std::unordered_map<HttpVersionId, std::string> http_versions_id2name_ = {
    {HttpVersionId::kHTTP09, "HTTP/0.9"},
    {HttpVersionId::kHTTP10, "HTTP/1.0"},
    {HttpVersionId::kHTTP11, "HTTP/1.1"},
};

//Reason Phrase
//key:vaule <HttpStatusCode, ReasonPhrase>
static const std::unordered_map<HttpStatusCode, std::string> http_status_code_descriptions_ = {
    {HttpStatusCode::k100, "Continue"},
    {HttpStatusCode::k101, "Switching Protocols"},

    {HttpStatusCode::k200, "OK"},
    {HttpStatusCode::k201, "Created"},
    {HttpStatusCode::k202, "Accepted"},
    {HttpStatusCode::k203, "Non-Authoritative Information"},
    {HttpStatusCode::k204, "No Content"},
    {HttpStatusCode::k205, "Reset Content"},
    {HttpStatusCode::k206, "Partial Content"},

    {HttpStatusCode::k300, "Multiple Choices"},
    {HttpStatusCode::k301, "Moved Permanently"},
    {HttpStatusCode::k302, "Found"},
    {HttpStatusCode::k303, "See Other"},
    {HttpStatusCode::k304, "Not Modified"},
    {HttpStatusCode::k305, "Use Proxy"},
    {HttpStatusCode::k306, "(Unused)"},
    {HttpStatusCode::k307, "Temporary Redirect"},

    {HttpStatusCode::k400, "Bad Request"},
    {HttpStatusCode::k401, "Unauthorized"},
    {HttpStatusCode::k402, "Payment Required"},
    {HttpStatusCode::k403, "Forbidden"},
    {HttpStatusCode::k404, "Not Found"},
    {HttpStatusCode::k405, "Method Not Allowed"},
    {HttpStatusCode::k406, "Not Acceptable"},
    {HttpStatusCode::k407, "Proxy Authentication Required"},
    {HttpStatusCode::k408, "Request Timeout"},
    {HttpStatusCode::k409, "Conflict"},
    {HttpStatusCode::k410, "Gone"},
    {HttpStatusCode::k411, "Length Required"},
    {HttpStatusCode::k412, "Precondition Failed"},
    {HttpStatusCode::k413, "Request Entity Too Large"},
    {HttpStatusCode::k414, "Request-URI Too Long"},
    {HttpStatusCode::k415, "Unsupported Media Type"},
    {HttpStatusCode::k416, "Requested Range Not Satisfiable"},
    {HttpStatusCode::k417, "Expectation Failed"},

    {HttpStatusCode::k500, "Internal Server Error"},
    {HttpStatusCode::k501, "Not Implemented"},
    {HttpStatusCode::k502, "Bad Gateway"},
    {HttpStatusCode::k503, "Service Unavailable"},
    {HttpStatusCode::k504, "Gateway Timeout"},
    {HttpStatusCode::k505, "HTTP Version Not Supported"},
};

struct HttpMessage
{
    HttpMessage()
    {
        init();
    }

    virtual ~HttpMessage() = default;


    static HttpMethodId find_http_method_id(std::string &method)
    {
        auto iter = http_methods_name2id_.find(method);
        if (iter != http_methods_name2id_.end()) {
            return iter->second;
        }

        return HttpMethodId::kUNKNOWN;
    }

    static const std::string& find_http_method_name(HttpMethodId method_id)
    {
        auto iter = http_methods_id2name_.find(method_id);
        if (iter != http_methods_id2name_.end()) {
            return iter->second;
        }

        return http_null_string_;
    }

    static HttpVersionId find_http_version_id(std::string &version)
    {
        auto iter = http_versions_name2id_.find(version);
        if (iter != http_versions_name2id_.end()) {
            return iter->second;
        }

        return HttpVersionId::kUNKNOWN;
    }

    static const std::string& find_http_version_name(HttpVersionId version_id)
    {
        auto iter = http_versions_id2name_.find(version_id);
        if (iter != http_versions_id2name_.end()) {
            return iter->second;
        }

        return http_null_string_;
    }

    static const std::string& find_http_status_description(HttpStatusCode status_code)
    {
        auto iter = http_status_code_descriptions_.find(status_code);
        if (iter != http_status_code_descriptions_.end()) {
            return iter->second;
        }

        return http_null_string_;
    }

    virtual void init()
    {
        content_length_ = 0;
        version_id_     = HttpVersionId::kHTTP11;
        headers_.clear();
        body_.reset();
        body_.reserve(2048);
    }

    virtual void reset()
    {
        content_length_ = 0;
        version_id_     = HttpVersionId::kHTTP11;
        headers_.clear();
        body_.reset();
    }

    virtual int update()
    {
        auto iter = headers_.find("Content-Length");
        if (iter != headers_.end()) {
            auto tmp = std::atoi(iter->second.c_str());
            if (tmp >= 0) {
                content_length_ = tmp;
                return 0;
            }
            return -1;
        }

        return 0;
    }

    const std::string & header(const std::string &key)
    {
        auto iter = headers_.find(key);
        if (iter != headers_.end()) {
            return iter->second;
        }

        return http_null_string_;
    }

    uint_t content_length_ = 0;
    HttpVersionId version_id_ = HttpVersionId::kHTTP11;
    std::map<std::string, std::string> headers_;
    ByteBuffer body_;
    char *body_ptr_ = nullptr;
};

struct HttpRequest : public HttpMessage
{
    HttpRequest()
    {
        init();
    }

    virtual ~HttpRequest()
    {
    }

    void init()
    {
        HttpMessage::init();
        method_id_ = HttpMethodId::kGET;
        body_ptr_  = nullptr;
        uri_.clear();
        uri_.reserve(1024);
    }

    void reset()
    {
        HttpMessage::reset();
        method_id_ = HttpMethodId::kGET;
        body_ptr_  = nullptr;
        uri_.clear();
    }

    template <class TBuffer>
    int encode_headers(TBuffer &out)
    {
        if (uri_.empty()) {
            uri_.push_back('/');
        }
        out.reserve(uri_.length() + 1024);

        //start line(request line)
        const std::string &method  = find_http_method_name(method_id_);
        const std::string &version = find_http_version_name(version_id_);
        int ret = std::snprintf(out.data(),
            out.free_size() - 1,
            "%s %s %s\r\n",
            method.c_str(),
            uri_.c_str(),
            version.c_str());
        out.data_end(static_cast<uint_t>(ret));

        //headers line
        for (auto &iter : headers_) {
            out.write(iter.first.c_str(), iter.first.size());
            out.write(": ", 2);
            out.write(iter.second.c_str(), iter.second.size());
            out.write("\r\n", 2);
        }

        //header.Content-Length
        if (method_id_ != HttpMethodId::kHEAD) {
            uint_t real_body_size = std::max<uint_t>(body_.data_size(), content_length_);
            if (real_body_size > 0) {
                out.reserve(out.data_size() + 32);
                ret = std::snprintf(out.data() + out.data_size(),
                    out.free_size() - 1,
                    "Content-Length: %u\r\n\r\n",
                    real_body_size);
                out.data_end(static_cast<uint_t>(out.data_size() + ret));
                return 0;
            }
        }

        //separate char: CRLF
        out.write("\r\n", 2);

        return 0;
    }

    template <class TBuffer>
    int encode(TBuffer &out)
    {
        int ret = encode_headers<TBuffer>(out);
        if (ret >= 0) {
            auto body_size = body_.data_size();
            if (body_size > 0) {
                out.write(body_.data(), body_size);
            }
        }
        return ret;
    }

    HttpMethodId method_id_ = HttpMethodId::kGET;
    std::string  uri_;
};

struct HttpResponse : public HttpMessage
{
    HttpResponse()
    {
        init();
    }

    virtual ~HttpResponse()
    {
    }

    void init()
    {
        HttpMessage::init();
        status_code_   = HttpStatusCode::k200;
        event_handler_ = nullptr;
        type_ = 0;
        attach_data_.id = 0;
        reason_phrase_.clear();
        reason_phrase_.reserve(64);
    }

    void reset()
    {
        HttpMessage::reset();
        status_code_   = HttpStatusCode::k200;
        event_handler_ = nullptr;
        type_ = 0;
        attach_data_.id = 0;
        reason_phrase_.clear();
    }

    template <class TBuffer>
    int encode_headers(TBuffer &out)
    {
        out.reserve(1024);

        //start line
        const std::string &version     = find_http_version_name(version_id_);
        const std::string &status_desc = find_http_status_description(status_code_);
        int ret = std::snprintf(out.data(),
            out.free_size() - 1,
            "%s %d %s\r\n",
            version.c_str(),
            status_code_,
            status_desc.c_str());
        out.data_end(static_cast<uint_t>(ret));

        //headers line 
        for (auto &iter : headers_) {
            out.write(iter.first.c_str(), iter.first.size());
            out.write(": ", 2);
            out.write(iter.second.c_str(), iter.second.size());
            out.write("\r\n", 2);
        }

        //header.Content-Length
        uint_t real_body_size = std::max<uint_t>(body_.data_size(), content_length_);
        if (real_body_size > 0) {
            out.reserve(out.data_size() + 32);
            int ret = std::snprintf(out.data() + out.data_size(),
                out.free_size()-1,
                "Content-Length: %u\r\n\r\n",
                real_body_size);
            out.data_end(static_cast<uint_t>(out.data_size() + ret));
        }
        else {
            out.write("Content-Length: 0\r\n\r\n", sizeof("Content-Length: 0\r\n\r\n") - 1);
        }

        return 0;
    }

    template <class TBuffer>
    int encode(TBuffer &out)
    {
        int ret = encode_headers<TBuffer>(out);
        if (ret >= 0) {
            auto body_size = body_.data_size();
            if (body_size > 0) {
                out.write(body_.data(), body_size);
            }
        }
        return ret;
    }

    template <class THanlder, class TBuffer>
    SendResult send(TBuffer &buf)
    {
        typename THanlder::super *handler = static_cast<typename THanlder::super *>(event_handler_);
        return handler->send(buf);
    }

    HttpStatusCode status_code_  = HttpStatusCode::k200;
    std::string reason_phrase_;
    EventHandler *event_handler_ = nullptr;
    int type_ = 0;
    union Data
    {
        void   *ptr;
        int64_t id;
    } attach_data_;
};

class HttpContext
{
public:
    HttpContext()
    {
        init();
    }

    virtual ~HttpContext()
    {
    }

    void init()
    {
        request_.init();
        response_.init();
    }

    void reset()
    {
        request_.reset();
        response_.reset();
    }

    int update()
    {
        return request_.update();
    }

public:
    HttpRequest  request_;
    HttpResponse response_;
};

class HttpDecoderConfig : public MessageDecoderConfig
{
public:
    HttpDecoderConfig() = default;
    virtual ~HttpDecoderConfig() = default;

    int update()
    {
        max_message_length_ = max_header_length_ + max_body_length_;
        return 0;
    }

    uint_t max_uri_length_      = 1024;
    uint_t max_header_length_   = 2048;
    uint_t max_body_length_     = 2048;
    uint_t max_message_length_  = 4096;
};

ZRSOCKET_NAMESPACE_END

#endif
