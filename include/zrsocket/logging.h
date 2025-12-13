// Some compilers (e.g. VC++) benefit significantly from using this. 
// We've measured 3-4% build speed improvements in apps as a result 
#pragma once

#ifndef ZRSOCKET_LOGGING_H
#define ZRSOCKET_LOGGING_H
#include <cstdio>
#include <string>
#include <charconv>
#include "byte_buffer.h"
#include "data_convert.h"
#include "lockfree.h"
#include "lockfree_queue.h"
#include "mutex.h"
#include "os_api_file.h"
#include "seda_stage.h"
#include "time.h"

ZRSOCKET_NAMESPACE_BEGIN

//为了输出美观对齐
//所有level_name长度相同,不足的用空格补齐
static constexpr const int   LEVEL_NAME_LEN = sizeof("TRACE") - 1;
static constexpr const char *LEVEL_NAMES[]  = { "TRACE", "DEBUG", "INFO ", "WARN ", "ERROR", "FATAL" };
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

enum class LogFormatType : uint8_t
{
    kTEXT,
    kBINARY,
};

enum class LogTimeSource : uint8_t
{
    kOSTime,        //OS时间
    kFrameworkTime  //框架时间(class Time)
};

//日志回调函数
typedef int (* LogCallbackFunc)(void *context, const char *log, uint_t len);

class LogConfig
{
public:
    LogConfig() = default;
    ~LogConfig() = default;

    inline void log_level(LogLevel level)
    {
        obj_.standby_ptr()->log_level_ = level;
    }

    inline void appender_type(LogAppenderType type)
    {
        obj_.standby_ptr()->appender_type_ = type;
    }

    inline void filename(const char* name)
    {
        obj_.standby_ptr()->filename_ = name;
    }

    inline void work_mode(LogWorkMode mode)
    {
        obj_.standby_ptr()->work_mode_ = mode;
    }

    inline void lock_type(LogLockType type)
    {
        obj_.standby_ptr()->lock_type_ = type;
    }

    inline void buffer_size(uint_t size)
    {
        static const int min_buffer_size = 1024 * 1024 * 4;
        if (size < min_buffer_size) {
            size = min_buffer_size;
        }
        obj_.standby_ptr()->buffer_size_ = size;
    }

    inline void format_type(LogFormatType type)
    {
        obj_.standby_ptr()->format_type_ = type;
    }

    inline void callback_func(LogCallbackFunc func, void* context)
    {
        auto conf = obj_.standby_ptr();
        conf->callback_func_ = func;
        conf->callback_context_ = context;
    }

    inline void log_time_source(LogTimeSource source)
    {
        obj_.standby_ptr()->log_time_source_ = source;
    }

    inline void update_framework_time(bool update_flag)
    {
        auto conf = obj_.standby_ptr();
        conf->update_framework_time_ = update_flag;
        if (update_flag) {
            conf->log_time_source_ = LogTimeSource::kFrameworkTime;
        }
    }

    inline void timedwait_interval_us(uint_t interval_us)
    {
        auto conf = obj_.standby_ptr();
        if (interval_us < 1) {
            interval_us = 1;
        }
        conf->timedwait_interval_us_ = interval_us;
    }

    inline void write_block_size(uint_t block_size)
    {
        auto conf = obj_.standby_ptr();
        if (block_size < 8192) {
            block_size = 8192;
        }
        conf->write_block_size_ = block_size;
    }

    inline LogLevel log_level() const
    {
        return obj_.active_ptr()->log_level_;
    }

    inline LogAppenderType appender_type() const
    {
        return obj_.active_ptr()->appender_type_;
    }

    inline LogWorkMode work_mode() const
    {
        return obj_.active_ptr()->work_mode_;
    }

    inline LogLockType lock_type() const
    {
        return obj_.active_ptr()->lock_type_;
    }

    inline LogFormatType format_type() const
    {
        return obj_.active_ptr()->format_type_;
    }

    inline uint_t buffer_size() const
    {
        return obj_.active_ptr()->buffer_size_;
    }

    inline LogTimeSource log_time_source() const
    {
        return obj_.active_ptr()->log_time_source_;
    }

    inline uint_t write_block_size() const
    {
        return obj_.active_ptr()->write_block_size_;
    }

    inline uint_t timedwait_interval_us() const
    {
        return obj_.active_ptr()->timedwait_interval_us_;
    }

    inline bool update_framework_time() const
    {
        return obj_.active_ptr()->update_framework_time_;
    }

    inline const char* filename() const
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
        LogLevel        log_level_          = LogLevel::kTRACE;
        LogAppenderType appender_type_      = LogAppenderType::kCONSOLE;
        LogWorkMode     work_mode_          = LogWorkMode::kASYNC;
        LogLockType     lock_type_          = LogLockType::kMUTEX;
        LogFormatType   format_type_        = LogFormatType::kTEXT;
        const char     *filename_           = nullptr;
        LogCallbackFunc callback_func_      = nullptr;    //回调函数
        void           *callback_context_   = nullptr;    //回调上下文
        LogTimeSource   log_time_source_    = LogTimeSource::kOSTime;

        //后台处理线程相关参数
        uint_t          buffer_size_ = 1024 * 1024 * 16;  //缓冲大小
        uint_t          write_block_size_ = 1024 * 256;   //写块大小(默认256k)
        uint_t          timedwait_interval_us_ = 10000;   //空闲时等待时长(条件变量等待时长,默认10ms)
        bool            update_framework_time_ = false;   //更新框架时间标识
};

    DoublePointerObject<Config> obj_;
};

class ILogger
{
public:
    ILogger() = default;
    virtual ~ILogger() = default;
    virtual LogConfig & config() = 0;
};

class ILogAppender
{
public:
    ILogAppender() = default;
    virtual ~ILogAppender() = default;

    virtual int open()  = 0;
    virtual int close() = 0;
    virtual int write(const char *log, uint_t len) = 0;
    virtual int flush()
    {
        return 0;
    }
};

class ILogWorker
{
public:
    ILogWorker(ILogAppender *appender)
        : appender_(appender)
    {
    }
    virtual ~ILogWorker() = default;

    virtual int open()  = 0;
    virtual int close() = 0;
    virtual int push(ByteBuffer &log) = 0;

    inline ILogAppender * appender() const
    {
        return appender_;
    }

protected:
    ILogAppender *appender_ = nullptr;
};

//前向声明
class Logger;

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

    int init(LogLevel level, const char *file, uint_t line, const char *function, Logger *logger);

    inline int fini()
    {
        //加空格分隔符
        buf_.write(' ');

        //output file
        buf_.write(file_);
        buf_.write(':');

        //output line
        uint_t end = buf_.data_end();
        buf_.reserve(end + std::numeric_limits<uint32_t>::digits10 + 50);
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

    inline self& operator<<(char *s)
    {
        buf_.write(s);
        return *this;
    }

    inline self& operator<<(const char *s)
    {
        buf_.write(s);
        return *this;
    }

    inline self& operator<<(const std::string &s)
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

    inline self& operator<<(char c)
    {
        buf_.write(c);
        return *this;
    }

    inline self& operator<<(uint8_t c)
    {
        buf_.write(c);
        return *this;
    }

    inline self& operator<<(int16_t i)
    {
        uint_t end = buf_.data_end();
        buf_.reserve(end + std::numeric_limits<int16_t>::digits10 + 50);
        int len = DataConvert::itoa(i, buf_.buffer() + end);
        buf_.data_end(end + len);
        return *this;
    }

    inline self& operator<<(uint16_t i)
    {
        uint_t end = buf_.data_end();
        buf_.reserve(end + std::numeric_limits<uint16_t>::digits10 + 50);
        int len = DataConvert::itoa(i, buf_.buffer() + end);
        buf_.data_end(end + len);
        return *this;
    }

    inline self& operator<<(int32_t i)
    {
        uint_t end = buf_.data_end();
        buf_.reserve(end + std::numeric_limits<int32_t>::digits10 + 50);

#if 1
        //方案1
        int len = DataConvert::itoa(i, buf_.buffer() + end);
        buf_.data_end(end + len);
#else
        //方案2
        std::to_chars_result res = std::to_chars(buf_.buffer() + end,
            buf_.buffer() + end + std::numeric_limits<int32_t>::max_digits10 + 50, i);
        if (res.ec == std::errc()) {
            buf_.data_end(static_cast<uint_t>(res.ptr - buf_.buffer()));
        }
#endif

        return *this;
    }

    inline self& operator<<(uint32_t i)
    {
        uint_t end = buf_.data_end();
        buf_.reserve(end + std::numeric_limits<uint32_t>::digits10 + 50);
        int len = DataConvert::uitoa(i, buf_.buffer() + end);
        buf_.data_end(end + len);
        return *this;
    }

    inline self& operator<<(int64_t i)
    {
        uint_t end = buf_.data_end();
        buf_.reserve(end + std::numeric_limits<int64_t>::digits10 + 50);
        int len = DataConvert::lltoa(i, buf_.buffer() + end);
        buf_.data_end(end + len);

        return *this;
    }

    inline self& operator<<(uint64_t i)
    {
        uint_t end = buf_.data_end();
        buf_.reserve(end + std::numeric_limits<uint64_t>::digits10 + 50);
        int len = DataConvert::ulltoa(i, buf_.buffer() + end);
        buf_.data_end(end + len);
        return *this;
    }

    inline self& operator<<(float32_t f)
    {
        uint_t end = buf_.data_end();
        buf_.reserve(end + std::numeric_limits<float32_t>::max_digits10 + 50);
        
#if 1
        //方案1
        std::to_chars_result res = std::to_chars(buf_.buffer() + end, 
            buf_.buffer() + end + std::numeric_limits<float32_t>::max_digits10 + 50, f);
        if (res.ec == std::errc()) {
            buf_.data_end(static_cast<uint_t>(res.ptr - buf_.buffer()));
        }
#else
        //方案2
        len = std::snprintf(buf_.buffer() + end, std::numeric_limits<float32_t>::max_digits10 + 30, "%.12g", f);
        buf_.data_end(end + len);
#endif

        return *this;
    }

    inline self& operator<<(float64_t f)
    {
        uint_t end = buf_.data_end();
        buf_.reserve(end + std::numeric_limits<float64_t>::max_digits10 + 50);

#if 1
        //方案1
        std::to_chars_result res = std::to_chars(buf_.buffer() + end,
            buf_.buffer() + end + std::numeric_limits<float64_t>::max_digits10 + 50, f);
        if (res.ec == std::errc()) {
            buf_.data_end(static_cast<uint_t>(res.ptr - buf_.buffer()));
        }
#else
        //方案2
        int len = std::snprintf(buf_.buffer() + end, std::numeric_limits<float64_t>::max_digits10 + 30, "%.12g", f);
        buf_.data_end(end + len);
#endif

        return *this;
    }

    inline ByteBuffer& buffer()
    {
        return buf_;
    }

private:
    const char *file_ = nullptr;
    const char *function_ = nullptr;
    LogLevel level_ = LogLevel::kTRACE;
    uint_t   line_  = 0;

    ByteBuffer buf_;
};

//每线程的输出流
static thread_local LogStream stream_;

template<typename TMutex>
class AsyncLogWorker: public ILogWorker
{
public:
    AsyncLogWorker(ILogAppender *appender, ILogger *logger)
        : ILogWorker(appender)
        , logger_(logger)
    {
        dfbuf_.set_max_size(logger->config().buffer_size());
    }

    ~AsyncLogWorker()
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

        //日志写入共享缓冲失败(可能是共享缓冲满)
        return 0;
    }

private:
    static int worker_thread_proc(void *arg)
    {
        //static constexpr const uint_t WRITE_BLOCK_SIZE = 8192;        //8k
        //static constexpr const uint_t WRITE_BLOCK_SIZE = 1024 *  64;  //64k
        //static constexpr const uint_t WRITE_BLOCK_SIZE = 1024 * 128;  //128k
        //static constexpr const uint_t WRITE_BLOCK_SIZE = 1024 * 256;  //经验值

        AsyncLogWorker<TMutex> *worker = static_cast<AsyncLogWorker<TMutex> *>(arg);
        LogConfig &conf = worker->logger_->config();
        Thread &thread = worker->worker_thread_;
        AtomicInt &timedwait_flag = worker->timedwait_flag_;
        DoubleFixedLengthBuffer<TMutex> &dfbuf = worker->dfbuf_;
        Mutex &timedwait_mutex = worker->timedwait_mutex_;
        Condition &timedwait_condition = worker->timedwait_condition_;
        ILogAppender *appender = worker->appender_;
        Time &time = Time::instance();

        ByteBuffer *buf = nullptr;
        uint_t data_size = 0;
        uint_t write_block_size = 0;
        uint_t timedwait_interval_us = 0;
        bool   update_framework_time_flag = conf.update_framework_time();
        if (!update_framework_time_flag) {
            while (thread.state() == Thread::State::RUNNING) {
                write_block_size = conf.write_block_size();
                buf = dfbuf.active_ptr();
                data_size = buf->data_size();
                while (data_size >= write_block_size) {
                    if (appender->write(buf->data(), write_block_size) > 0) {
                        buf->data_begin(buf->data_begin() + write_block_size);
                        data_size -= write_block_size;
                    }
                    else {
                        break;
                    }
                }
                if (data_size > 0) {
                    if (appender->write(buf->data(), data_size) > 0) {
                        data_size = 0;
                    }
                }

                if (0 == data_size) {
                    buf->reset();
                    if (!dfbuf.swap_buffer()) {
                        timedwait_flag.store(1, std::memory_order_relaxed);
                        {
                            timedwait_interval_us = conf.timedwait_interval_us();
                            std::unique_lock<std::mutex> lock(timedwait_mutex);
                            timedwait_condition.wait_for(lock, std::chrono::microseconds(timedwait_interval_us));
                        }
                        timedwait_flag.store(0, std::memory_order_relaxed);
                        dfbuf.swap_buffer();
                    }
                }
            }
        }
        else {
            while (thread.state() == Thread::State::RUNNING) {
                update_framework_time_flag = conf.update_framework_time();
                if (update_framework_time_flag) {
                    time.update_time();
                }

                write_block_size = conf.write_block_size();
                buf = dfbuf.active_ptr();
                data_size = buf->data_size();
                while (data_size >= write_block_size) {
                    if (appender->write(buf->data(), write_block_size) > 0) {
                        buf->data_begin(buf->data_begin() + write_block_size);
                        data_size -= write_block_size;
                    }
                    else {
                        break;
                    }
                }
                if (data_size > 0) {
                    if (appender->write(buf->data(), data_size) > 0) {
                        data_size = 0;
                    }
                }

                if (0 == data_size) {
                    buf->reset();
                    if (!dfbuf.swap_buffer()) {
                        timedwait_flag.store(1, std::memory_order_relaxed);
                        {
                            if (update_framework_time_flag) {
                                time.update_time();
                            }

                            timedwait_interval_us = conf.timedwait_interval_us();
                            std::unique_lock<std::mutex> lock(timedwait_mutex);
                            timedwait_condition.wait_for(lock, std::chrono::microseconds(timedwait_interval_us));
                        }
                        timedwait_flag.store(0, std::memory_order_relaxed);
                        dfbuf.swap_buffer();
                    }
                }
            }
        }

        appender->flush();
        return 0;
    }

public:
    ILogger *logger_ = nullptr;

    DoubleFixedLengthBuffer<TMutex> dfbuf_;
    Thread      worker_thread_;
    Mutex       timedwait_mutex_;
    Condition   timedwait_condition_;
    AtomicInt   timedwait_flag_ = ATOMIC_VAR_INIT(0);   //条件触发标识
};

template<typename TMutex>
class SyncLogWorker : public ILogWorker
{
public:
    SyncLogWorker(ILogAppender *appender)
        : ILogWorker(appender)
    {
    }
    ~SyncLogWorker() = default;

    int open() override
    {
        return 0;
    }

    int close() override
    {
        return 0;
    }

    inline int push(ByteBuffer &log) override
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

class ConsoleLogAppender : public ILogAppender
{
public:
    ConsoleLogAppender() = default;
    ~ConsoleLogAppender() = default;

    int open() override
    {
        return 0;
    }

    int close() override
    {
        return 0;
    }

    inline int write(const char *log, uint_t len)
    {
        return OSApiFile::os_write(STDOUT_FILENO, log, len);
    }
};

class OSApiFileLogAppender : public ILogAppender
{
public:
    OSApiFileLogAppender() = delete;
    OSApiFileLogAppender(const char *filename)
        : filename_(filename)
    {
    }
    ~OSApiFileLogAppender()
    {
        flush();
        close();
    }

    int open() override
    {
        return file_.open(filename_, O_WRONLY|O_APPEND|O_CREAT, S_IWRITE|S_IREAD);
    }

    int close() override
    {
        return file_.close();
    }

    inline int write(const char *log, uint_t len) override
    {
        return file_.writen(log, len);
    }

    inline int flush()
    {
        return file_.fsync();
    }

private:
    const char *filename_;
    OSApiFile file_;
};

class NullLogAppender : public ILogAppender
{
public:
    NullLogAppender() = default;
    ~NullLogAppender() = default;

    int open() override
    {
        return 0;
    }

    int close() override
    {
        return 0;
    }

    inline int write(const char *log, uint_t len) override
    {
        return 0;
    }
};

class CallbackLogAppender : public ILogAppender
{
public:
    CallbackLogAppender() = delete;
    CallbackLogAppender(LogCallbackFunc func, void *context)
        : callback_func_(func)
        , callback_context_(context)
    {
    }
    ~CallbackLogAppender() = default;

    int open() override
    {
        return 0;
    }

    int close() override
    {
        return 0;
    }

    inline int write(const char *log, uint_t len) override
    {
        return callback_func_(callback_context_, log, len);
    }

private:
    LogCallbackFunc callback_func_;
    void           *callback_context_ = nullptr;
};

class StdioFileLogAppender : public ILogAppender
{
public:
    StdioFileLogAppender() = delete;
    StdioFileLogAppender(const char *filename)
        : filename_(filename)
    {
    }
    ~StdioFileLogAppender()
    {
        flush();
        close();
    }

    int open() override
    {
        return file_.open(filename_, "a+");
    }

    int close() override
    {
        return file_.close();
    }

    inline int write(const char *log, uint_t len) override
    {
        return file_.writen(log, len);
    }

    inline int flush()
    {
        return file_.flush();
    }

private:
    const char *filename_;
    StdioFile file_;
};

class Logger : public ILogger
{
public:

    //全局logger
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

    LogConfig & config()
    {
        return config_;
    }

    int init()
    {
        if (init_flag_) {
            return 1;
        }

        Time::instance();
        config_.update_config();
        switch (config_.appender_type()) {
        case LogAppenderType::kCONSOLE:
            appender_ = new ConsoleLogAppender();
            if (config_.work_mode() == LogWorkMode::kSYNC) {
                worker_ = new SyncLogWorker<NullMutex>(appender_);
            }
            break;
        case LogAppenderType::kFILE:
            appender_ = new OSApiFileLogAppender(config_.filename());
            if (config_.work_mode() == LogWorkMode::kSYNC) {
                switch (config_.lock_type()) {
                case LogLockType::kNULL:
                    worker_ = new SyncLogWorker<NullMutex>(appender_);
                    break;
                case LogLockType::kSPINLOCK:
                    worker_ = new SyncLogWorker<SpinlockMutex>(appender_);
                    break;
                case LogLockType::kMUTEX:
                default:
                    worker_ = new SyncLogWorker<ThreadMutex>(appender_);
                    break;
                }
            }
            break;
        case LogAppenderType::kNULL:
            appender_ = new NullLogAppender();
            worker_   = new SyncLogWorker<NullMutex>(appender_);
            break;
        case LogAppenderType::kCALLBACK:
            {
                auto conf = config_.obj_.active_ptr();
                if (nullptr != conf->callback_func_) {
                    appender_ = new CallbackLogAppender(conf->callback_func_, conf->callback_context_);
                    if (config_.work_mode() == LogWorkMode::kSYNC) {
                        switch (config_.lock_type()) {
                        case LogLockType::kNULL:
                            worker_ = new SyncLogWorker<NullMutex>(appender_);
                            break;
                        case LogLockType::kSPINLOCK:
                            worker_ = new SyncLogWorker<SpinlockMutex>(appender_);
                            break;
                        case LogLockType::kMUTEX:
                        default:
                            worker_ = new SyncLogWorker<ThreadMutex>(appender_);
                            break;
                        }
                    }
                }
                else {
                    appender_ = new NullLogAppender();
                    worker_ = new SyncLogWorker<NullMutex>(appender_);
                }
            }
            break;
        default:
            appender_ = new NullLogAppender();
            worker_   = new SyncLogWorker<NullMutex>(appender_);
            break;
        }

        if (config_.work_mode() == LogWorkMode::kASYNC) {
            if (nullptr == worker_) {
                switch (config_.lock_type()) {
                case LogLockType::kNULL:
                    worker_ = new AsyncLogWorker<NullMutex>(appender_, this);
                    break;
                case LogLockType::kSPINLOCK:
                    worker_ = new AsyncLogWorker<SpinlockMutex>(appender_, this);
                    break;
                case LogLockType::kMUTEX:
                default:
                    worker_ = new AsyncLogWorker<ThreadMutex>(appender_, this);
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
            return worker_->push(stream.buffer());
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

#define ZRSOCKET_LOG_SET_LOG_LEVEL2(logger,level)               logger.config().log_level(level)
#define ZRSOCKET_LOG_SET_APPENDER_TYPE2(logger,type)            logger.config().appender_type(type)
#define ZRSOCKET_LOG_SET_FILE_NAME2(logger,name)                logger.config().filename(name)
#define ZRSOCKET_LOG_SET_CALLBACK_FUNC2(logger,func,context)    logger.config().callback_func(func, context)
#define ZRSOCKET_LOG_SET_WORK_MODE2(logger,mode)                logger.config().work_mode(mode)
#define ZRSOCKET_LOG_SET_LOCK_TYPE2(logger,type)                logger.config().lock_type(type)
#define ZRSOCKET_LOG_SET_BUFFER_SIZE2(logger,size)              logger.config().buffer_size(size)
#define ZRSOCKET_LOG_SET_FORMAT_TYPE2(logger,type)              logger.config().format_type(type)
#define ZRSOCKET_LOG_SET_WRITE_BLOCK_SIZE2(logger, size)        logger.config().write_block_size(size)
#define ZRSOCKET_LOG_SET_TIMEDWAIT_INTERVAL_US2(logger, us)     logger.config().timedwait_interval_us(us)
#define ZRSOCKET_LOG_SET_UPDATE_FRAMEWORK_TIME2(logger, flag)   logger.config().update_framework_time(flag)
#define ZRSOCKET_LOG_SET_LOG_TIME_SOURCE2(logger, source)       logger.config().log_time_source(source)
#define ZRSOCKET_LOG_INIT2(logger)                              logger.init()

#define ZRSOCKET_LOG_BODY(logger,logEvent,logLevel)                     \
    do {                                                                \
        if (logger.is_logged(logLevel)) {                               \
            zrsocket::LogStream &stream = zrsocket::stream_;            \
            stream.init(logLevel,__FILE__,__LINE__,__func__, &logger);  \
            stream << logEvent;                                         \
            logger.log(stream);                                         \
        }                                                               \
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
#define ZRSOCKET_LOG_SET_WRITE_BLOCK_SIZE(size)         ZRSOCKET_LOG_SET_WRITE_BLOCK_SIZE2(zrsocket::Logger::instance(), size)    
#define ZRSOCKET_LOG_SET_TIMEDWAIT_INTERVAL_US(us)      ZRSOCKET_LOG_SET_TIMEDWAIT_INTERVAL_US2(zrsocket::Logger::instance(), us)
#define ZRSOCKET_LOG_SET_UPDATE_FRAMEWORK_TIME(flag)    ZRSOCKET_LOG_SET_UPDATE_FRAMEWORK_TIME2(zrsocket::Logger::instance(), flag)
#define ZRSOCKET_LOG_SET_LOG_TIME_SOURCE(source)        ZRSOCKET_LOG_SET_LOG_TIME_SOURCE2(zrsocket::Logger::instance(), source)
#define ZRSOCKET_LOG_INIT                               ZRSOCKET_LOG_INIT2(zrsocket::Logger::instance())

#define ZRSOCKET_LOG_TRACE(e)   ZRSOCKET_LOG_BODY(zrsocket::Logger::instance(),e,zrsocket::LogLevel::kTRACE)
#define ZRSOCKET_LOG_DEBUG(e)   ZRSOCKET_LOG_BODY(zrsocket::Logger::instance(),e,zrsocket::LogLevel::kDEBUG)
#define ZRSOCKET_LOG_INFO(e)    ZRSOCKET_LOG_BODY(zrsocket::Logger::instance(),e,zrsocket::LogLevel::kINFO)
#define ZRSOCKET_LOG_WARN(e)    ZRSOCKET_LOG_BODY(zrsocket::Logger::instance(),e,zrsocket::LogLevel::kWARN)
#define ZRSOCKET_LOG_ERROR(e)   ZRSOCKET_LOG_BODY(zrsocket::Logger::instance(),e,zrsocket::LogLevel::kERROR)
#define ZRSOCKET_LOG_FATAL(e)   ZRSOCKET_LOG_BODY(zrsocket::Logger::instance(),e,zrsocket::LogLevel::kFATAL)

#endif
