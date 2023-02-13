// Some compilers (e.g. VC++) benefit significantly from using this. 
// We've measured 3-4% build speed improvements in apps as a result 
#pragma once

#ifndef ZRSOCKET_LOGGING_H
#define ZRSOCKET_LOGGING_H
#include <cstdio>
#include <string>
#include "byte_buffer.h"
#include "data_convert.h"
#include "lockfree.h"
#include "lockfree_queue.h"
#include "mutex.h"
#include "os_api_file.h"
#include "seda_stage.h"

ZRSOCKET_NAMESPACE_BEGIN

//为了输出美观对齐
//所有level_name长度相同,不足的用空格补齐
static const int   LEVEL_NAME_LEN = sizeof("TRACE") - 1;
static const char *LEVEL_NAMES[]  = { "TRACE", "DEBUG", "INFO ", "WARN ", "ERROR", "FATAL" };
enum class LogLevel : uint8_t
{
    kTRACE,
    kDEBUG,
    kINFO,
    kWARN,
    kERROR,
    kFATAL,

    //内部使用
    NUM_LOG_LEVELS,
};

enum class LogAppenderType : uint8_t
{
    kCONSOLE,   //控制台
    kFILE,      //文件
    kNULL,      //类似于linux中 /dev/null
    kCALLBACK,  //回调模式
};

enum class LogWorkMode : uint8_t
{
    kSYNC,      //同步  直接在调用线程中输出日志
    kASYNC,     //异步  日志线程中输出日志
};

enum class LogLockType : uint8_t
{
    kNULL,
    kSPINLOCK,
    kMUTEX,
};

//日志回调函数
typedef int (* LogCallbackFunc)(void *context, const char *log, uint_t len);

class ILogAppender
{
public:
    ILogAppender() = default;
    virtual ~ILogAppender() = default;

    virtual int open() = 0;
    virtual int close() = 0;
    virtual int write(const char *log, uint_t len) = 0;
};

class ILogWorker
{
public:
    ILogWorker(ILogAppender *appender)
        : appender_(appender)
    {
    }
    virtual ~ILogWorker() = default;

    virtual int open() = 0;
    virtual int close() = 0;
    virtual int push(ByteBuffer &log) = 0;

    inline ILogAppender * get_appender() const
    {
        return appender_;
    }

protected:
    ILogAppender *appender_ = nullptr;
};

class LogConfig
{
public:
    LogConfig() = default;
    ~LogConfig() = default;

    inline void set_log_level(LogLevel level)
    {
        obj_.standby_ptr()->log_level_ = level;
    }

    inline void set_appender_type(LogAppenderType type)
    {
        obj_.standby_ptr()->appender_type_ = type;
    }

    inline void set_filename(const char *name)
    {
        obj_.standby_ptr()->filename_ = name;
    }

    inline void set_work_mode(LogWorkMode mode)
    {
        obj_.standby_ptr()->work_mode_ = mode;
    }

    inline void set_lock_type(LogLockType type)
    {
        obj_.standby_ptr()->lock_type_ = type;
    }

    inline void set_buffer_size(uint_t size)
    {
        if (size < 4096) {
            size = 4096;
        }
        obj_.standby_ptr()->buffer_size_ = size;
    }

    inline void set_callback_func(LogCallbackFunc func, void *context)
    {
        auto conf = obj_.standby_ptr();
        conf->callback_func_ = func;
        conf->callback_context_ = context;
    }

    inline LogLevel get_log_level() const
    {
        return obj_.active_ptr()->log_level_;
    }

    inline LogAppenderType get_appender_type() const
    {
        return obj_.active_ptr()->appender_type_;
    }

    inline LogWorkMode get_work_mode() const
    {
        return obj_.active_ptr()->work_mode_;
    }

    inline LogLockType get_lock_type() const
    {
        return obj_.active_ptr()->lock_type_;
    }

    inline uint_t get_buffer_size() const
    {
        return obj_.active_ptr()->buffer_size_;
    }

    inline const char * filename() const
    {
        return obj_.active_ptr()->filename_;
    }
   
    inline bool update_config()
    {
        return obj_.swap_pointer();
    }

public:
    struct Config
    {
        LogLevel        log_level_      = LogLevel::kTRACE;
        LogAppenderType appender_type_  = LogAppenderType::kCONSOLE;
        LogWorkMode     work_mode_      = LogWorkMode::kASYNC;
        LogLockType     lock_type_      = LogLockType::kMUTEX;
        uint_t          buffer_size_    = 1024 * 1024 * 16;
        const char     *filename_       = nullptr;

        LogCallbackFunc callback_func_  = nullptr;      //回调函数
        void           *callback_context_ = nullptr;    //回调上下文
    };

    DoublePointerObject<Config> obj_;
};

//输出格式
//[datetime(UTC时区)] level [......] file line thread_id
class LogStream
{
public:
    using self = LogStream;
    LogStream()
        : buf_(4096)
    {
    }
    ~LogStream() = default;

    inline int init(LogLevel level, const char *file, uint_t line, const char *function)
    {
        level_  = level;
        file_   = file;
        line_   = line;
        function_ = function;

        //每线程本地缓存精确到秒的时间字符串
        //buf_的前DATETIME_S_LEN个字节就是 精确到秒的时间字符串
        static const uint_t DATETIME_S_LEN = sizeof("2020-01-01 00:00:00.") - 1;
        static thread_local struct timespec last_ts_ = { 0 };

        //取当前精确到纳秒的时间
        struct timespec ts;
        OSApi::gettimeofday(&ts);

        //output current datetime(format: [YYYY-MM-DD HH:MM:SS.SSSSSSSSSZ] UTC时区)
        if (ts.tv_sec != last_ts_.tv_sec) {
            last_ts_.tv_sec = ts.tv_sec;

            struct tm buf_tm;
            OSApi::gmtime_s(&ts.tv_sec, &buf_tm);

            int  datetime_s_len = 0;
            char *datetime_s    = buf_.data();

            //output year
            int len = DataConvert::uitoa(buf_tm.tm_year + 1900, datetime_s + datetime_s_len);
            datetime_s_len += len;
            datetime_s[datetime_s_len] = '-';
            ++datetime_s_len;

            //output month
            if (buf_tm.tm_mon + 1 < 10) {
                datetime_s[datetime_s_len] = '0';
                ++datetime_s_len;
            }
            len = DataConvert::uitoa(buf_tm.tm_mon + 1, datetime_s + datetime_s_len);
            datetime_s_len += len;
            datetime_s[datetime_s_len] = '-';
            ++datetime_s_len;

            //output mday
            if (buf_tm.tm_mday < 10) {
                datetime_s[datetime_s_len] = '0';
                ++datetime_s_len;
            }
            len = DataConvert::uitoa(buf_tm.tm_mday, datetime_s + datetime_s_len);
            datetime_s_len += len;
            datetime_s[datetime_s_len] = ' ';
            ++datetime_s_len;

            //output hours
            if (buf_tm.tm_hour < 10) {
                datetime_s[datetime_s_len] = '0';
                ++datetime_s_len;
            }
            len = DataConvert::uitoa(buf_tm.tm_hour, datetime_s + datetime_s_len);
            datetime_s_len += len;
            datetime_s[datetime_s_len] = ':';
            ++datetime_s_len;

            //output minutes
            if (buf_tm.tm_min < 10) {
                datetime_s[datetime_s_len] = '0';
                ++datetime_s_len;
            }
            len = DataConvert::uitoa(buf_tm.tm_min, datetime_s + datetime_s_len);
            datetime_s_len += len;
            datetime_s[datetime_s_len] = ':';
            ++datetime_s_len;

            //output seconds
            if (buf_tm.tm_sec < 10) {
                datetime_s[datetime_s_len] = '0';
                ++datetime_s_len;
            }
            len = DataConvert::uitoa(buf_tm.tm_sec, datetime_s + datetime_s_len);
            datetime_s_len += len;
            datetime_s[datetime_s_len] = '.';
        }
        buf_.data_end(DATETIME_S_LEN);

        //output nanoseconds
        ////补零写法一
        //if (ts.tv_nsec < 10) {
        //    buf_.write("00000000", 8);
        //}
        //else if (ts.tv_nsec < 100) {
        //    buf_.write("0000000", 7);
        //}
        //else if (ts.tv_nsec < 1000) {
        //    buf_.write("000000", 6);
        //}
        //else if (ts.tv_nsec < 10000) {
        //    buf_.write("00000", 5);
        //}
        //else if (ts.tv_nsec < 100000) {
        //    buf_.write("0000", 4);
        //}
        //else if (ts.tv_nsec < 1000000) {
        //    buf_.write("000", 3);
        //}
        //else if (ts.tv_nsec < 10000000) {
        //    buf_.write("00", 2);
        //}
        //else if (ts.tv_nsec < 100000000) {
        //    buf_.write("0", 1);
        //}

        ////补零写法二
        //if (ts.tv_nsec < 100000000) {
        //    if (ts.tv_nsec < 10000000) {
        //        if (ts.tv_nsec < 1000000) {
        //            if (ts.tv_nsec < 100000) {
        //                if (ts.tv_nsec < 10000) {
        //                    if (ts.tv_nsec < 1000) {
        //                        if (ts.tv_nsec < 100) {
        //                            if (ts.tv_nsec < 10) {
        //                                buf_.write("00000000", 8);
        //                            }
        //                            else {
        //                                buf_.write("0000000", 7);
        //                            }
        //                        }
        //                        else {
        //                            buf_.write("000000", 6);
        //                        }
        //                    }
        //                    else {
        //                        buf_.write("00000", 5);
        //                    }
        //                }
        //                else {
        //                    buf_.write("0000", 4);
        //                }
        //            }
        //            else {
        //                buf_.write("000", 3);
        //            }
        //        }
        //        else {
        //            buf_.write("00", 2);
        //        }
        //    }
        //    else {
        //        buf_.write("0", 1);
        //    }
        //}

        //补零写法三
        //性能提高不明显(与编译器优化有关)
        if (ts.tv_nsec >= 100000) {
            if (ts.tv_nsec >= 10000000) {
                if (ts.tv_nsec < 100000000) {
                    buf_.write("0", 1);
                }
            }
            else {
                if (ts.tv_nsec >= 1000000) {
                    buf_.write("00", 2);
                }
                else {
                    buf_.write("003", 3);
                }
            }
        }
        else {
            if (ts.tv_nsec >= 1000) {
                if (ts.tv_nsec >= 10000) {
                    buf_.write("0000", 4);
                }
                else {
                    buf_.write("00000", 5);
                }
            }
            else {
                if (ts.tv_nsec >= 10) {
                    if (ts.tv_nsec >= 100) {
                        buf_.write("000000", 6);
                    }
                    else {
                        buf_.write("0000000", 7);
                    }
                }
                else {
                    buf_.write("00000000", 8);
                }
            }
        }
        uint_t end = buf_.data_end();
        int len = DataConvert::uitoa(ts.tv_nsec, buf_.buffer() + end);
        buf_.data_end(end + len);
        buf_.write("Z ", 2);

        //output level        
        buf_.write(LEVEL_NAMES[static_cast<int>(level)], LEVEL_NAME_LEN);
        buf_.write(' ');

        return 1;
    }

    inline int fini()
    {
        //加空格分隔符
        buf_.write(' ');

        //output file
        buf_.write(file_);
        buf_.write(':');

        //output line
        uint_t end = buf_.data_end();
        buf_.reserve(end + std::numeric_limits<uint32_t>::digits10 + 30);
        int len = DataConvert::uitoa(line_, buf_.buffer() + end);
        buf_.data_end(end + len);
        buf_.write(' ');

        //output thread id
        end = buf_.data_end();
        len = DataConvert::ulltoa(OSApi::this_thread_id(), buf_.buffer() + end);
        buf_.data_end(end + len);

        //加换行符
        buf_.write('\n');

        return 1;
    }

    inline self& operator<< (const char *s)
    {
        buf_.write(s);
        return *this;
    }

    inline self& operator<< (std::string &s)
    {
        buf_.write(s.data(), static_cast<uint_t>(s.length()));
        return *this;
    }

    inline self& operator<<(bool b)
    {
        if (b) {
            buf_.write("true", 4);
        }
        else {
            buf_.write("false", 5);
        }
        return *this;
    }

    inline self& operator<<(int8_t c)
    {
        buf_.write(c);
        return *this;
    }

    inline self& operator<<(uint8_t c)
    {
        buf_.write(c);
        return *this;
    }

    inline self& operator<< (int16_t i)
    {
        uint_t end = buf_.data_end();
        buf_.reserve(end + std::numeric_limits<int16_t>::digits10 + 30);
        int len = DataConvert::itoa(i, buf_.buffer() + end);
        buf_.data_end(end + len);
        return *this;
    }

    inline self& operator<< (uint16_t i)
    {
        uint_t end = buf_.data_end();
        buf_.reserve(end + std::numeric_limits<uint16_t>::digits10 + 30);
        int len = DataConvert::itoa(i, buf_.buffer() + end);
        buf_.data_end(end + len);
        return *this;
    }

    inline self& operator<< (int32_t i)
    {
        uint_t end = buf_.data_end();
        buf_.reserve(end + std::numeric_limits<int32_t>::digits10 + 30);
        int len = DataConvert::itoa(i, buf_.buffer() + end);
        buf_.data_end(end + len);
        return *this;
    }

    inline self& operator<< (uint32_t i)
    {
        uint_t end = buf_.data_end();
        buf_.reserve(end + std::numeric_limits<uint32_t>::digits10 + 30);
        int len = DataConvert::uitoa(i, buf_.buffer() + end);
        buf_.data_end(end + len);
        return *this;
    }

    inline self& operator<< (int64_t i)
    {
        uint_t end = buf_.data_end();
        buf_.reserve(end + std::numeric_limits<int64_t>::digits10 + 30);
        int len = DataConvert::lltoa(i, buf_.buffer() + end);
        buf_.data_end(end + len);

        return *this;
    }

    inline self& operator<< (uint64_t i)
    {
        uint_t end = buf_.data_end();
        buf_.reserve(end + std::numeric_limits<uint64_t>::digits10 + 30);
        int len = DataConvert::ulltoa(i, buf_.buffer() + end);
        buf_.data_end(end + len);
        return *this;
    }

    inline self& operator<<(float f)
    {
        uint_t end = buf_.data_end();
        buf_.reserve(end + std::numeric_limits<float>::max_digits10 + 30);
        int len = std::snprintf(buf_.buffer() + end, std::numeric_limits<float>::max_digits10 + 30, "%.12g", f);
        buf_.data_end(end + len);
        return *this;
    }

    inline self& operator<<(double d)
    {
        uint_t end = buf_.data_end();
        buf_.reserve(end + std::numeric_limits<double>::max_digits10 + 30);
        int len = std::snprintf(buf_.buffer() + end, std::numeric_limits<double>::max_digits10 + 30, "%.12g", d);
        buf_.data_end(end + len);
        return *this;
    }

    inline ByteBuffer& get_buffer()
    {
        return buf_;
    }

private:
    LogLevel level_ = LogLevel::kTRACE;
    uint_t   line_  = 0;
    const char *file_ = nullptr;
    const char *function_ = nullptr;

    ByteBuffer buf_;
};

//每线程的输出流
static thread_local zrsocket::LogStream stream_;

template<typename TMutex>
class AsyncWorker: public ILogWorker
{
public:
    AsyncWorker(ILogAppender *appender, uint_t buffer_size)
        : ILogWorker(appender)
    {
        dfbuf_.set_max_size(buffer_size);
    }

    virtual ~AsyncWorker()
    {
        close();
    }

    int open() override
    {
        return worker_thread_.start(worker_thread_proc, this);
    }

    int close() override
    {
        worker_thread_.stop();
        timedwait_condition_.notify_one();
        worker_thread_.join();
        return 1;
    }

    inline int push(ByteBuffer &log) override
    {
        if (dfbuf_.write(log.data(), log.data_size(), true) > 0) {
            if (timedwait_flag_.load(std::memory_order_relaxed)) {
                timedwait_flag_.store(0, std::memory_order_relaxed);
                timedwait_condition_.notify_one();
            }
            return 1;
        }
        return 0;
    }

private:
    static int worker_thread_proc(void *arg)
    {
        static const uint_t WRITE_BLOCK_SIZE = 8192;

        AsyncWorker<TMutex> *worker = static_cast<AsyncWorker<TMutex> *>(arg);
        Thread &thread = worker->worker_thread_;
        AtomicInt &timedwait_flag = worker->timedwait_flag_;
        DoubleFixedLengthBuffer<TMutex> &dfbuf = worker->dfbuf_;
        Mutex &timedwait_mutex = worker->timedwait_mutex_;
        Condition &timedwait_condition = worker->timedwait_condition_;
        uint_t timedwait_interval_us = worker->timedwait_interval_us_;
        ILogAppender *appender = worker->appender_;

        uint_t data_size;
        ByteBuffer *buf;
        while (thread.state() == Thread::State::RUNNING) {
            buf = dfbuf.active_ptr();
            data_size = buf->data_size();
            while (data_size >= WRITE_BLOCK_SIZE) {
                if (appender->write(buf->data(), WRITE_BLOCK_SIZE) > 0) {
                    buf->data_begin(buf->data_begin() + WRITE_BLOCK_SIZE);
                    data_size -= WRITE_BLOCK_SIZE;
                }
                else {
                    break;
                }
            }
            if (data_size > 0) {
                if (appender->write(buf->data(), data_size) > 0) {
                    buf->reset();
                    data_size = 0;
                }
            }

            if (0 == data_size) {
                if (!dfbuf.swap_buffer()) {
                    timedwait_flag.store(1, std::memory_order_relaxed);
                    {
                        std::unique_lock<std::mutex> lock(timedwait_mutex);
                        timedwait_condition.wait_for(lock, std::chrono::microseconds(timedwait_interval_us));
                    }
                    timedwait_flag.store(0, std::memory_order_relaxed);
                    dfbuf.swap_buffer();
                }
            }
        }

        return 0;
    }

public:
    DoubleFixedLengthBuffer<TMutex> dfbuf_;

    Thread      worker_thread_;
    Mutex       timedwait_mutex_;
    Condition   timedwait_condition_;
    uint_t      timedwait_interval_us_ = 50000;
    AtomicInt   timedwait_flag_ = ATOMIC_VAR_INIT(0);   //条件触发标识
};

template<typename TMutex>
class SyncWorker : public ILogWorker
{
public:
    SyncWorker(ILogAppender* appender)
        : ILogWorker(appender)
    {
    }
    virtual ~SyncWorker() = default;

    int open() override
    {
        return 0;
    }
    int close() override
    {
        return 0;
    }

    inline int push(ByteBuffer& log) override
    {
        int ret;
        mutex_.lock();
        ret = appender_->write(log.data(), log.data_size());
        mutex_.unlock();
        return ret;
    }

private:
    TMutex mutex_;
};

class ConsoleAppender : public ILogAppender
{
public:
    ConsoleAppender()
    {
    }
    virtual ~ConsoleAppender()
    {
    }

    int open()
    {
        return 0;
    }

    int close()
    {
        return 0;
    }

    inline int write(const char *log, uint_t len)
    {
        return OSApiFile::os_write(STDOUT_FILENO, log, len);
    }
};

class OSApiFileAppender : public ILogAppender
{
public:
    OSApiFileAppender() = delete;
    OSApiFileAppender(const char* filename)
        : filename_(filename)
    {
    }
    virtual ~OSApiFileAppender()
    {
        close();
    }

    int open()
    {
        return file_.open(filename_, O_WRONLY|O_APPEND|O_CREAT, S_IWRITE|S_IREAD);
    }

    int close()
    {
        return file_.close();
    }

    inline int write(const char *log, uint_t len)
    {
        return file_.writen(log, len);
    }

private:
    const char *filename_;
    OSApiFile file_;
};

class NullAppender : public ILogAppender
{
public:
    NullAppender() = default;
    virtual ~NullAppender() = default;

    int open()
    {
        return 0;
    }

    int close()
    {
        return 0;
    }

    inline int write(const char *log, uint_t len)
    {
        return 0;
    }
};

class CallbackAppender : public ILogAppender
{
public:
    CallbackAppender() = delete;
    CallbackAppender(LogCallbackFunc func, void *context)
        : callback_func_(func)
        , callback_context_(context)
    {
    }
    virtual ~CallbackAppender() = default;

    int open()
    {
        return 0;
    }

    int close()
    {
        return 0;
    }

    inline int write(const char *log, uint_t len)
    {
        return callback_func_(callback_context_, log, len);
    }

private:
    LogCallbackFunc callback_func_;
    void           *callback_context_ = nullptr;
};

class StdioFileAppender : public ILogAppender
{
public:
    StdioFileAppender() = delete;
    StdioFileAppender(const char *filename)
        : filename_(filename)
    {
    }
    virtual ~StdioFileAppender()
    {
        close();
    }

    int open()
    {
        return file_.open(filename_, "a+");
    }

    int close()
    {
        return file_.close();
    }

    inline int write(const char *log, uint_t len)
    {
        int ret = file_.writen(log, len);
        if (ret > 0) {
            file_.flush();
        }
        return ret;
    }

private:
    const char *filename_;
    StdioFile file_;
};

class Logger
{
public:

    //访问全局logger
    static Logger & instance()
    {
        static Logger logger;
        return logger;
    }

    Logger() = default;
    ~Logger()
    {
        fini();
    }

    inline LogConfig & config()
    {
        return config_;
    }

    int init()
    {
        if (init_flag_) {
            return 1;
        }

        config_.update_config();
        switch (config_.get_appender_type()) {
        case LogAppenderType::kCONSOLE:
            appender_ = new ConsoleAppender();
            if (config_.get_work_mode() == LogWorkMode::kSYNC) {
                worker_ = new SyncWorker<NullMutex>(appender_);
            }
            break;
        case LogAppenderType::kFILE:
            appender_ = new OSApiFileAppender(config_.filename());
            if (config_.get_work_mode() == LogWorkMode::kSYNC) {
                switch (config_.get_lock_type()) {
                case LogLockType::kNULL:
                    worker_ = new SyncWorker<NullMutex>(appender_);
                    break;
                case LogLockType::kSPINLOCK:
                    worker_ = new SyncWorker<SpinlockMutex>(appender_);
                    break;
                case LogLockType::kMUTEX:
                default:
                    worker_ = new SyncWorker<ThreadMutex>(appender_);
                    break;
                }
            }
            break;
        case LogAppenderType::kNULL:
            appender_ = new NullAppender();
            worker_   = new SyncWorker<NullMutex>(appender_);
            break;
        case LogAppenderType::kCALLBACK:
            {
                auto conf = config_.obj_.active_ptr();
                if (nullptr != conf->callback_func_) {
                    appender_ = new CallbackAppender(conf->callback_func_, conf->callback_context_);
                    if (config_.get_work_mode() == LogWorkMode::kSYNC) {
                        switch (config_.get_lock_type()) {
                        case LogLockType::kNULL:
                            worker_ = new SyncWorker<NullMutex>(appender_);
                            break;
                        case LogLockType::kSPINLOCK:
                            worker_ = new SyncWorker<SpinlockMutex>(appender_);
                            break;
                        case LogLockType::kMUTEX:
                        default:
                            worker_ = new SyncWorker<ThreadMutex>(appender_);
                            break;
                        }
                    }
                }
                else {
                    appender_ = new NullAppender();
                    worker_ = new SyncWorker<NullMutex>(appender_);
                }
            }
            break;
        default:
            appender_ = new NullAppender();
            worker_   = new SyncWorker<NullMutex>(appender_);
            break;
        }

        if (config_.get_work_mode() == LogWorkMode::kASYNC) {
            if (nullptr == worker_) {
                switch (config_.get_lock_type()) {
                case LogLockType::kNULL:
                    worker_ = new AsyncWorker<NullMutex>(appender_, config_.get_buffer_size());
                    break;
                case LogLockType::kSPINLOCK:
                    worker_ = new AsyncWorker<SpinlockMutex>(appender_, config_.get_buffer_size());
                    break;
                case LogLockType::kMUTEX:
                default:
                    worker_ = new AsyncWorker<ThreadMutex>(appender_, config_.get_buffer_size());
                    break;
                }
            }
        }

        if (nullptr == appender_) {
            return -1;
        }

        if (nullptr == worker_) {
            return -2;
        }

        int ret = appender_->open();
        if (ret < 0) {
            return ret;
        }
        ret = worker_->open();
        if (ret < 0) {
            return ret;
        }

        init_flag_ = true;
        return 1;
    }

    int fini()
    {
        if (worker_ != nullptr) {
            worker_->close();
            delete worker_;
            worker_ = nullptr;
        }
        if (appender_ != nullptr) {
            appender_->close();
            delete appender_;
            appender_ = nullptr;
        }
        init_flag_ = false;
        return 1;
    }

    inline bool is_logged(LogLevel level) const
    {
        return level >= config_.obj_.active_ptr()->log_level_;
    }

    inline int log(LogStream &stream)
    {
        if (init_flag_) {
            stream.fini();
            return worker_->push(stream.get_buffer());
        }

        return 0;
    }

private:
    ILogAppender *appender_ = nullptr;
    ILogWorker   *worker_   = nullptr;
    LogConfig     config_;

    bool init_flag_ = false;
};

ZRSOCKET_NAMESPACE_END


//一个系统可以有多个logger

#define ZRSOCKET_LOG_SET_LOG_LEVEL2(logger,level)               logger.config().set_log_level(level)
#define ZRSOCKET_LOG_SET_APPENDER_TYPE2(logger,type)            logger.config().set_appender_type(type)
#define ZRSOCKET_LOG_SET_FILE_NAME2(logger,name)                logger.config().set_filename(name)
#define ZRSOCKET_LOG_SET_CALLBACK_FUNC2(logger,func,context)    logger.config().set_callback_func(func, context)
#define ZRSOCKET_LOG_SET_WORK_MODE2(logger,mode)                logger.config().set_work_mode(mode)
#define ZRSOCKET_LOG_SET_LOCK_TYPE2(logger,type)                logger.config().set_lock_type(type)
#define ZRSOCKET_LOG_SET_BUFFER_SIZE2(logger,size)              logger.config().set_buffer_size(size)
#define ZRSOCKET_LOG_INIT2(logger)                              logger.init()

#define ZRSOCKET_LOG_BODY(logger,logEvent,logLevel)             \
    do {                                                        \
        if (logger.is_logged(logLevel)) {                       \
            zrsocket::LogStream &stream = zrsocket::stream_;    \
            stream.init(logLevel,__FILE__,__LINE__,__func__);   \
            stream << logEvent;                                 \
            logger.log(stream);                                 \
        }                                                       \
    } while (0)

#define ZRSOCKET_LOG_TRACE2(logger,e)   ZRSOCKET_LOG_BODY(logger,e,zrsocket::LogLevel::kTRACE)
#define ZRSOCKET_LOG_DEBUG2(logger,e)   ZRSOCKET_LOG_BODY(logger,e,zrsocket::LogLevel::kDEBUG)
#define ZRSOCKET_LOG_INFO2(logger,e)    ZRSOCKET_LOG_BODY(logger,e,zrsocket::LogLevel::kINFO)
#define ZRSOCKET_LOG_WARN2(logger,e)    ZRSOCKET_LOG_BODY(logger,e,zrsocket::LogLevel::kWARN)
#define ZRSOCKET_LOG_ERROR2(logger,e)   ZRSOCKET_LOG_BODY(logger,e,zrsocket::LogLevel::kERROR)
#define ZRSOCKET_LOG_FATAL2(logger,e)   ZRSOCKET_LOG_BODY(logger,e,zrsocket::LogLevel::kFATAL)


//全局logger

#define ZRSOCKET_LOG_SET_LOG_LEVEL(level)               ZRSOCKET_LOG_SET_LOG_LEVEL2(zrsocket::Logger::instance(),level)
#define ZRSOCKET_LOG_SET_APPENDER_TYPE(type)            ZRSOCKET_LOG_SET_APPENDER_TYPE2(zrsocket::Logger::instance(),type)
#define ZRSOCKET_LOG_SET_FILE_NAME(name)                ZRSOCKET_LOG_SET_FILE_NAME2(zrsocket::Logger::instance(),name)
#define ZRSOCKET_LOG_SET_CALLBACK_FUNC(func,context)    ZRSOCKET_LOG_SET_CALLBACK_FUNC2(zrsocket::Logger::instance(),func,context)
#define ZRSOCKET_LOG_SET_WORK_MODE(mode)                ZRSOCKET_LOG_SET_WORK_MODE2(zrsocket::Logger::instance(),mode)
#define ZRSOCKET_LOG_SET_LOCK_TYPE(type)                ZRSOCKET_LOG_SET_LOCK_TYPE2(zrsocket::Logger::instance(),type)
#define ZRSOCKET_LOG_SET_BUFFER_SIZE(size)              ZRSOCKET_LOG_SET_BUFFER_SIZE2(zrsocket::Logger::instance(),size)
#define ZRSOCKET_LOG_INIT                               ZRSOCKET_LOG_INIT2(zrsocket::Logger::instance())

#define ZRSOCKET_LOG_TRACE(e)   ZRSOCKET_LOG_BODY(zrsocket::Logger::instance(),e,zrsocket::LogLevel::kTRACE)
#define ZRSOCKET_LOG_DEBUG(e)   ZRSOCKET_LOG_BODY(zrsocket::Logger::instance(),e,zrsocket::LogLevel::kDEBUG)
#define ZRSOCKET_LOG_INFO(e)    ZRSOCKET_LOG_BODY(zrsocket::Logger::instance(),e,zrsocket::LogLevel::kINFO)
#define ZRSOCKET_LOG_WARN(e)    ZRSOCKET_LOG_BODY(zrsocket::Logger::instance(),e,zrsocket::LogLevel::kWARN)
#define ZRSOCKET_LOG_ERROR(e)   ZRSOCKET_LOG_BODY(zrsocket::Logger::instance(),e,zrsocket::LogLevel::kERROR)
#define ZRSOCKET_LOG_FATAL(e)   ZRSOCKET_LOG_BODY(zrsocket::Logger::instance(),e,zrsocket::LogLevel::kFATAL)

#endif
