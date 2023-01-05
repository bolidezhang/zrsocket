// Some compilers (e.g. VC++) benefit significantly from using this. 
// We've measured 3-4% build speed improvements in apps as a result 
#pragma once

#ifndef ZRSOCKET_EVENT_TYPE_HANDLER_H
#define ZRSOCKET_EVENT_TYPE_HANDLER_H
#include "config.h"
#include "event_type.h"

ZRSOCKET_NAMESPACE_BEGIN

class EventTypeHandler
{
public:
    EventTypeHandler() = default;
    virtual ~EventTypeHandler() = default;

    virtual int handle_event(const EventType *event)
    {
        return 0;
    }
};

ZRSOCKET_NAMESPACE_END

#endif
