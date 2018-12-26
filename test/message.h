#ifndef TEST_MESSAGE_H_
#define TEST_MESSAGE_H_
#include "zrsocket/zrsocket.h"

ZRSOCKET_BEGIN

struct MessageType {
    enum type {
        MT_BEGIN_NUMBER = 10,
        MT_ECHO_REQ = 10,
        MT_ECHO_RES = 11,
        MT_HELLO_REQ = 12,
        MT_HELLO_RES = 13,
        MT_HELLO_NOTIFY = 14,
        MT_KEEPALIVE_REQ = 15,
        MT_KEEPALIVE_RES = 16,
        MT_KEEPALIVE_NOTIFY = 17,
        MT_END_NUMBER = 18,
    };
};

struct MessageHead
{
    ushort_t length;    //长度
    ushort_t type;      //消息类型
    uint_t   source;
    uint_t   sequence;  //消息序列号
};

ZRSOCKET_END

#endif
