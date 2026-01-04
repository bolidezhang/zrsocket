#pragma once

#ifndef ZRSOCKET_LOGGING_NANO_H
#define ZRSOCKET_LOGGING_NANO_H

#include "config.h"
#include "logging.h"
#include "mutex.h"
#include <vector>
#include <list>

ZRSOCKET_NAMESPACE_BEGIN

class NanoLogger {
public:

    //全局logger
    static NanoLogger & instance() {
        static NanoLogger logger;
        return logger;
    }

    int init() {
        return 0;
    }

    int fini() {
        return 0;
    }


    inline LogConfig & config()
    {
        return config_;
    }

    inline bool check_level(LogLevel level) const
    {
        return level >= config_.obj_.active_ptr()->log_level_;
    }

    template<typename... Args>
    inline void log(uint32_t log_id, LogLevel level, const char *file, uint_t line, const char *function, 
        const char *format, Args&&... args) {
        if (nullptr == stream_) {
            stream_ = new Stream();
            if (nullptr != stream_) {
                std::lock_guard<SpinMutex> lock(thread_streams_mutex_);
                thread_streams_.push_back(stream_);
            }
        }

        if (nullptr != stream_) {

        }

        return;
    }

    // Forward Declarations
    class Stream;
    class StreamDestroyer;

    class Stream {
    public:
        Stream() {

            // Empty function, but causes the C++ runtime to instantiate the
            // stream_destroyer_ thread_local (see documentation in function).
            stream_destroyer_.init();
        }

        ~Stream() {
        }
        bool should_deallocated_ = false; //将要释放标识

        friend StreamDestroyer;
        friend NanoLogger;
    };

    class StreamDestroyer {
    public:
        explicit StreamDestroyer() {
        }

        virtual ~StreamDestroyer() {
            if (nullptr != stream_) {
                stream_->should_deallocated_ = true;
                stream_ = nullptr;
            }
        }

        // Weird C++ hack; C++ thread_local are instantiated upon first use
        // thus the Stream has to invoke this function in order
        // to instantiate this object.
        void init() {
        }
    };

private:
    NanoLogger() {
        thread_streams_.reserve(10);
        thread_streams_bg_.reserve(10);
    }

    ~NanoLogger() {
    }

    // Storage NanoLogStream
    static zrsocket_fast_thread_local Stream *stream_;

    // Destroys the __thread Stream upon its own destruction, which
    // is synchronized with thread death
    static thread_local StreamDestroyer stream_destroyer_;

    SpinMutex thread_streams_mutex_;
    std::vector<Stream *> thread_streams_;
    std::vector<Stream *> thread_streams_bg_;

    LogConfig config_;
    bool init_flag_ = false;
};

ZRSOCKET_NAMESPACE_END

#define ZRSOCKET_NANOLOG_SET_LOG_LEVEL(level)               ZRSOCKET_LOG_SET_LOG_LEVEL2(zrsocket::NanoLogger::instance(),level)
#define ZRSOCKET_NANOLOG_SET_APPENDER_TYPE(type)            ZRSOCKET_LOG_SET_APPENDER_TYPE2(zrsocket::NanoLogger::instance(),type)
#define ZRSOCKET_NANOLOG_SET_FILE_NAME(name)                ZRSOCKET_LOG_SET_FILE_NAME2(zrsocket::NanoLogger::instance(),name)
#define ZRSOCKET_NANOLOG_SET_CALLBACK_FUNC(func,context)    ZRSOCKET_LOG_SET_CALLBACK_FUNC2(zrsocket::NanoLogger::instance(),func,context)
#define ZRSOCKET_NANOLOG_SET_WORK_MODE(mode)                ZRSOCKET_LOG_SET_WORK_MODE2(zrsocket::NanoLogger::instance(),mode)
#define ZRSOCKET_NANOLOG_SET_LOCK_TYPE(type)                ZRSOCKET_LOG_SET_LOCK_TYPE2(zrsocket::NanoLogger::instance(),type)
#define ZRSOCKET_NANOLOG_SET_BUFFER_SIZE(size)              ZRSOCKET_LOG_SET_BUFFER_SIZE2(zrsocket::NanoLogger::instance(),size)
#define ZRSOCKET_NANOLOG_SET_WRITE_BLOCK_SIZE(size)         ZRSOCKET_LOG_SET_WRITE_BLOCK_SIZE2(zrsocket::NanoLogger::instance(), size)    
#define ZRSOCKET_NANOLOG_SET_TIMEDWAIT_INTERVAL_US(us)      ZRSOCKET_LOG_SET_TIMEDWAIT_INTERVAL_US2(zrsocket::NanoLogger::instance(), us)
#define ZRSOCKET_NANOLOG_SET_UPDATE_FRAMEWORK_TIME(flag)    ZRSOCKET_LOG_SET_UPDATE_FRAMEWORK_TIME2(zrsocket::NanoLogger::instance(), flag)
#define ZRSOCKET_NANOLOG_SET_LOG_TIME_SOURCE(source)        ZRSOCKET_LOG_SET_LOG_TIME_SOURCE2(zrsocket::NanoLogger::instance(), source)
#define ZRSOCKET_NANOLOG_INIT                               ZRSOCKET_LOG_INIT2(zrsocket::NanoLogger::instance())

#define ZRSOCKET_NANOLOG_BODY(level, format, ...)                                               \
    do {                                                                                        \
        static uint32_t log_id = 0;                                                             \
        zrsocket::NanoLogger &logger = zrsocket::NanoLogger::instance();                        \
        if (logger.check_level(level)) {                                                        \
            logger.log(log_id, level, __FILE__, __LINE__, __func__, format, ##__VA_ARGS__);     \
        }                                                                                       \
    } while (0)

#endif