// Some compilers (e.g. VC++) benefit significantly from using this. 
// We've measured 3-4% build speed improvements in apps as a result 
#pragma once

#ifndef ZRSOCKET_H
#define ZRSOCKET_H
#include "version.h"
#include "config.h"
#include "atomic.h"
#include "byte_buffer.h"
#include "base_type.h"
#include "malloc.h"
#include "memory.h"
#include "object_pool.h"
#include "mutex.h"
#include "thread.h"
#include "os_api.h"
#include "os_api_file.h"
#include "os_constant.h"
#include "event_type.h"
#include "event_type_queue.h"
#include "event_handler.h"
#include "message_common.h"
#include "message_handler.h"
#include "notify_handler.h"
#include "fixed_length_message_handler.h"
#include "length_field_message_handler.h"
#include "event_source.h"
#include "event_loop.h"
#include "event_loop_group.h"
#include "event_loop_queue.h"
#include "select_event_loop.h"
#include "epoll_event_loop.h"
#include "tcpserver_handler.h"
#include "tcpserver.h"
#include "tcpclient.h"
#include "udpsource.h"
#include "udpsource_handler.h"
#include "time.h"
#include "timer.h"
#include "timer_queue.h"
#include "http_common.h"
#include "http_request_handler.h"
#include "http_response_handler.h"
#include "seda_event.h"
#include "seda_event_queue.h"
#include "seda_timer.h"
#include "seda_timer_queue.h"
#include "seda_interface.h"
#include "seda_stage_handler.h"
#include "seda_stage.h"
#include "seda_stage_thread.h"
#include "seda_stage2.h"
#include "seda_stage2_thread.h"
#include "global.h"
#include "measure_counter.h"
#include "logging.h"
#include "lockfree.h"
#include "lockfree_queue.h"
#include "application.h"

ZRSOCKET_NAMESPACE_BEGIN

#ifdef ZRSOCKET_OS_WINDOWS
    template<typename TMutex, typename TLoopData, typename TEventTypeHandler>
    using DefaultEventLoop = SelectEventLoop<TMutex, TLoopData, TEventTypeHandler>;

    #define zrsocket_default_event_loop zrsocket::SelectEventLoop
    #define ZRSOCKET_DEFAULT_EVENT_LOOP zrsocket::SelectEventLoop
#else
    template<typename TMutex, typename TLoopData, typename TEventTypeHandler>
    using DefaultEventLoop = EpollEventLoop<TMutex, TLoopData, TEventTypeHandler>;

    #define zrsocket_default_event_loop zrsocket::EpollEventLoop
    #define ZRSOCKET_DEFAULT_EVENT_LOOP zrsocket::EpollEventLoop
#endif

ZRSOCKET_NAMESPACE_END

#endif
