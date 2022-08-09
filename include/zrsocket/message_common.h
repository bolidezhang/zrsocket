// Some compilers (e.g. VC++) benefit significantly from using this. 
// We've measured 3-4% build speed improvements in apps as a result 
#pragma once

#ifndef ZRSOCKET_MESSAGE_COMMON_H
#define ZRSOCKET_MESSAGE_COMMON_H
#include "config.h"
#include "base_type.h"

ZRSOCKET_NAMESPACE_BEGIN

//消息解码配置基类
class MessageDecoderConfig
{
public:
    MessageDecoderConfig() = default;
    virtual ~MessageDecoderConfig() = default;
    virtual int update() = 0;
};

class MessageCommon
{
public:
    static inline uint_t read_length_field_int8(const char *data)
    {
        return *data;
    }

    static inline uint_t read_length_field_int16_host(const char *data)
    {
        return *((int16_t *)data);
    }

    static inline uint_t read_length_field_int16_network(const char *data)
    {
        return ntohs(*((int16_t *)data));
    }

    static inline uint_t read_length_field_int32_host(const char *data)
    {
        return  *((int32_t *)data);
    }

    static inline uint_t read_length_field_int32_network(const char *data)
    {
        return ntohl(*((int32_t *)data));
    }

    static void write_length_field_int8(char *buf, uint_t message_length)
    {
        *buf = static_cast<char>(message_length);
    }

    static void write_length_field_int16_host(char *buf, uint_t message_length)
    {
        *((int16_t *)buf) = static_cast<int16_t>(message_length);
    }

    static void write_length_field_int16_network(char *buf, uint_t message_length)
    {
        *((int16_t *)buf) = htons(message_length);
    }

    static void write_length_field_int32_host(char *buf, uint_t message_length)
    {
        *((int32_t *)buf) = message_length;
    }

    static void write_length_field_int32_network(char *buf, uint_t message_length)
    {
        *((int32_t *)buf) = htonl(message_length);
    }

    typedef uint_t (* read_length_field_proc)(const char *data);
    typedef void (* write_length_field_proc)(char *buf, uint_t message_length);
};

ZRSOCKET_NAMESPACE_END

#endif
