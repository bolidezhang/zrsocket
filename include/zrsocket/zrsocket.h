#ifndef ZRSOCKET_H_
#define ZRSOCKET_H_

#include "version.h"
#include "config.h"
#include "byte_buffer.h"
#include "base_type.h"
#include "malloc.h"
#include "memory.h"
#include "object_pool.h"
#include "mutex.h"
#include "thread.h"
#include "system_api.h"
#include "event_handler.h"
#include "message_handler.h"
#include "event_reactor.h"
#include "event_reactor_group.h"
#include "event_source.h"
#include "select_reactor.h"
#include "epoll_reactor.h"
#include "tcpserver_handler.h"
#include "tcpserver.h"
#include "tcpclient.h"

#ifdef ZRSOCKET_OS_WINDOWS
    #define zrsocket_default_reactor SelectReactor
#else
    #define zrsocket_default_reactor EpollReactor
#endif

#endif
