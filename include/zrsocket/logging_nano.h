#pragma once

#ifndef ZRSOCKET_LOGGING_NANO_H
#define ZRSOCKET_LOGGING_NANO_H

#include "config.h"
#include "logging.h"
#include "mutex.h"
#include "thread.h"
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
        if (!log_thread_running_) {
            if (log_file_.open(config_.obj_.active_ptr()->filename_, O_WRONLY | O_APPEND | O_CREAT, S_IWRITE | S_IREAD) != 0) {
                return -1;
            }

            log_thread_running_ = true;
            log_thread_ = std::thread([this]() {
                while (log_thread_running_) {
                    this->log_thread_poll();
                }
                this->log_thread_poll();
            });
        }

        return 1;
    }

    int fini() {
        if (!log_thread_running_) {
            return 0;
        }
        log_thread_running_ = false;
        if (log_thread_.joinable()) {
            log_thread_.join();
        }
        log_file_.close();
        return 1;
    }


    inline LogConfig & config() {
        return config_;
    }

    inline bool check_level(LogLevel level) const {
        return level >= config_.obj_.active_ptr()->log_level_;
    }

    template<typename... Args>
    inline void log(uint32_t &log_id, LogLevel level, const char *file, uint_t line, const char *function, 
        const char *format, Args&&... args) {

        // 1. 注册日志ID (Double-Checked Locking的变体，利用static变量)
        if (log_id == 0) {
            //log_id = register_log_id<Args...>(level, file, line, func, format);
        }

        // 2. 获取当前线程的Buffer
        ThreadBuffer *buffer = get_thread_buffer();
        if (!buffer) {
            return;
        }

        return;
    }

    //日志id信息(静态日志信息)
    struct LogId {
        uint32_t    log_id;
        uint32_t    line;
        const char *file;
        const char *format;
        uint8_t     log_level;
    };

    //日志条目
    struct LogEntry {
        uint32_t  entry_size;
        uint32_t  log_id;
        uint64_t  timestamp;
        uint64_t  thread_id;
        char      arg_data[0];
    };

    // Forward Declarations
    class ThreadBuffer;
    class ThreadBufferDestroyer;

    class ThreadBuffer {
    public:
        ThreadBuffer() {

            // Empty function, but causes the C++ runtime to instantiate the
            // ThreadBufferDestroyer thread_local (see documentation in function).
            thread_buffer_destroyer_.init();
        }

        ~ThreadBuffer() {
        }
        
        //在缓冲区中分配
        char* alloc_free(size_t size) {
            //todo...

            return nullptr;
        }

        //取得logEntry指针和所占大小
        char* log_entry(size_t &size) {
            //todo...

            return nullptr;
        }

        bool should_deallocated() {
            return should_deallocated_;
        }

        void mark_deallocate() { 
            should_deallocated_ = true; 
        }

    private:
        bool should_deallocated_ = false; //将要释放标识
        ByteBuffer buffer_;  //缓冲

        friend ThreadBufferDestroyer;
        friend NanoLogger;
    };

    class ThreadBufferDestroyer {
    public:
        explicit ThreadBufferDestroyer() {
        }

        ~ThreadBufferDestroyer() {
            if (nullptr != thread_buffer_) {
                thread_buffer_->mark_deallocate();
                thread_buffer_ = nullptr;
            }
        }

        // Weird C++ hack; C++ thread_local are instantiated upon first use
        // thus the LogBuffer has to invoke this function in order
        // to instantiate this object.
        void init() {
        }
    };

private:
    NanoLogger() {
        thread_buffers_.reserve(10);
        thread_buffers_bg_.reserve(10);
    }

    ~NanoLogger() {
    }

    ThreadBuffer * get_thread_buffer() {
        if (!thread_buffer_) {
            thread_buffer_ = new ThreadBuffer();
            std::lock_guard<SpinMutex> lock(thread_buffers_mutex_);
            thread_buffers_.push_back(thread_buffer_);
        }
        return thread_buffer_;
    }

    int log_thread_poll() {
        {
            std::lock_guard<SpinMutex> lock(log_ids_mutex_);
            if (!log_ids_.empty()) {
                log_ids_bg_.insert(log_ids_bg_.end(), log_ids_.begin(), log_ids_.end());
                log_ids_.clear();
            }
        }

        {
            std::lock_guard<SpinMutex> lock(thread_buffers_mutex_);
            if (!thread_buffers_.empty()) {                
                thread_buffers_bg_.insert(thread_buffers_bg_.end(), thread_buffers_.begin(), thread_buffers_.end());
                thread_buffers_.clear();
            }
        }

        auto iter = thread_buffers_bg_.begin();
        while (iter != thread_buffers_bg_.end()) {
            ThreadBuffer *tb = *iter;

            //处理log
            //...

            if (tb->should_deallocated_) {
                delete tb;
                iter = thread_buffers_bg_.erase(iter);
            }
            else {
                ++iter;
            }
        }

        return 1;
    }

    // Storage LogBuffer
    static zrsocket_fast_thread_local ThreadBuffer *thread_buffer_;

    // Destroys the __thread LogBuffer upon its own destruction, which
    // is synchronized with thread death
    static thread_local ThreadBufferDestroyer thread_buffer_destroyer_;

    SpinMutex log_ids_mutex_;
    std::vector<LogId> log_ids_;
    std::vector<LogId> log_ids_bg_;

    SpinMutex thread_buffers_mutex_;
    std::vector<ThreadBuffer *> thread_buffers_;
    std::vector<ThreadBuffer *> thread_buffers_bg_;
    
    OSApiFile     log_file_;
    ByteBuffer    log_format_buffer_;  //日志格式缓冲
    std::thread   log_thread_;
    volatile bool log_thread_running_ = false;
    Mutex         timedwait_mutex_;
    Condition     timedwait_condition_;
    AtomicInt     timedwait_flag_ = ATOMIC_VAR_INIT(0);   //条件触发标识

    LogConfig     config_;
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
