#ifndef ECHO_MESSAGE_H_
#define ECHO_MESSAGE_H_
#include "zrsocket/zrsocket.h"

ZRSOCKET_NAMESPACE_BEGIN

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

#pragma pack(1)
struct MessageHead
{
    uint16_t    length;    //长度
    uint16_t    type;      //消息类型
    uint32_t    source;
    uint32_t    sequence;  //消息序列号
};

//message max_size: 16384
struct Message
{
    MessageHead head;
    char        body[16384-sizeof(MessageHead)];
};
#pragma pack()

ZRSOCKET_NAMESPACE_END

#endif
