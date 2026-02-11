#include "zrsocket/logging_nano.h"

ZRSOCKET_NAMESPACE_BEGIN

zrsocket_fast_thread_local NanoLogger::ThreadBuffer *NanoLogger::thread_buffer_ = nullptr;
thread_local NanoLogger::ThreadBufferDestroyer NanoLogger::thread_buffer_destroyer_;

ZRSOCKET_NAMESPACE_END
