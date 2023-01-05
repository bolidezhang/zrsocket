// Some compilers (e.g. VC++) benefit significantly from using this. 
// We've measured 3-4% build speed improvements in apps as a result 
#pragma once

#ifndef ZRSOCKET_SEDA_EVENT_QUEUE_H
#define ZRSOCKET_SEDA_EVENT_QUEUE_H
#include "event_type_queue.h"

ZRSOCKET_NAMESPACE_BEGIN
typedef EventTypeQueue SedaEventQueue;
ZRSOCKET_NAMESPACE_END

#endif
