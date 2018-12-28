#ifndef TEST_SERVER_H
#define TEST_SERVER_H
#include "message.h"
#include "zrsocket/zrsocket.h"

using namespace zrsocket;
class ClientHandler : public MessageHandler<ByteBuffer, SpinMutex>
{
public:
    int handle_open()
    {
        printf("server handle_open:%d\n", socket_);
        return 0;
    }

    int handle_close()
    {
        printf("server handle_close:%d\n", socket_);
        return 0;
    }

    int handle_message(const char *message, uint_t len)
    {
        MessageHead *head = (MessageHead *)message;
        head->type = MessageType::MT_ECHO_RES;
        push_message(handler_id_, message, len);
        return 0;
    }
};

#endif
