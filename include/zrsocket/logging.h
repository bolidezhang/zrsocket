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
    CONSOLE,   //控制台
    FILE,      //文件
};

enum class LogWorkMode : uint8_t
{
    SYNC,      //同步  直接在调用线程中输出日志
    ASYNC,     //异步  日志线程中输出日志
};

enum class LogLockType : uint8_t
{
    NONE,
    SPINLOCK,
    MUTEX,
};

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
        config_.standby_ptr()->log_level_ = level;
    }

    inline void set_filename(const char *filename)
    {
        auto config = config_.standby_ptr();
        config->filename_ = filename;
        config->appender_type_ = LogAppenderType::FILE;
    }

    inline void set_work_mode(LogWorkMode mode)
    {
        config_.standby_ptr()->work_mode_ = mode;
    }

    inline void set_lock_type(LogLockType type)
    {
        config_.standby_ptr()->lock_type_ = type;
    }

    inline void set_buffer_size(uint_t buf_size)
    {
        if (buf_size < 4096) {
            buf_size = 4096;
        }
        config_.standby_ptr()->buffer_size_ = buf_size;
    }

    inline LogLevel get_log_level() const
    {
        return config_.active_ptr()->log_level_;
    }

    inline LogAppenderType get_appender_type() const
    {
        return config_.active_ptr()->appender_type_;
    }

    inline LogWorkMode get_work_mode() const
    {
        return config_.active_ptr()->work_mode_;
    }

    inline LogLockType get_lock_type() const
    {
        return config_.active_ptr()->lock_type_;
    }

    inline uint_t get_buffer_size() const
    {
        return config_.active_ptr()->buffer_size_;
    }

    inline const char * filename() const
    {
        return config_.active_ptr()->filename_;
    }

    inline bool is_logged(LogLevel level) const
    {
        return level >= config_.active_ptr()->log_level_;
    }
    
    inline bool update_config()
    {
        return config_.swap_pointer();
    }

public:
    struct Config
    {
        LogLevel        log_level_      = LogLevel::kTRACE;
        LogAppenderType appender_type_  = LogAppenderType::CONSOLE;
        LogWorkMode     work_mode_      = LogWorkMode::ASYNC;
        LogLockType     lock_type_      = LogLockType::MUTEX;
        uint_t          buffer_size_    = 1024 * 1024 * 16;
        const char     *filename_       = nullptr;
    };

    DoublePointerObject<Config> config_;
};

//输出格式
//datetime level ...... file line thread_id
class LogStream
{
public:
    using self = LogStream;
    LogStream()
        : buf_(2048)
    {
    }

    ~LogStream() = default;

    inline int init(LogLevel level, const char *source_file, uint_t line, const char *func_name)
    {
        level_ = level;
        source_file_ = source_file;
        line_ = line;
        func_name_ = func_name;

        buf_.reset();
        buf_.reserve(2048);

        //output current datetime(format: [yyyy-mm-dd hh:mm:ss.nnnnnnnnn])
        struct timespec ts;
        OSApi::gettimeofday(&ts);
        struct tm buf_tm;
        OSApi::localtime_s(&(ts.tv_sec), &buf_tm);
        int len = std::snprintf(buf_.buffer() + buf_.data_end(), 100, "[%04d-%02d-%02d %02d:%02d:%02d.%09ld] ",
            buf_tm.tm_year + 1900, buf_tm.tm_mon + 1, buf_tm.tm_mday, buf_tm.tm_hour, buf_tm.tm_min, buf_tm.tm_sec, ts.tv_nsec);
        buf_.data_end(buf_.data_end() + len);

        //output level        
        buf_.write(LEVEL_NAMES[static_cast<int>(level)], LEVEL_NAME_LEN);

        //加分隔符
        buf_.write(" [");

        return 1;
    }

    inline int fini()
    {
        //加分隔符
        buf_.write("] ");

        //output file
        buf_.write(source_file_);
        buf_.write(' ');

        //output line
        int len = DataConvert::itoa(line_, buf_.buffer() + buf_.data_end());
        buf_.data_end(buf_.data_end() + len);
        buf_.write(' ');

        //output thread id        
        len = DataConvert::ulltoa(OSApi::this_thread_id(), buf_.buffer() + buf_.data_end());
        buf_.data_end(buf_.data_end() + len);

        //换行
        buf_.write('\n');

        return 0;
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
        buf_.reserve(buf_.data_end() + std::numeric_limits<int16_t>::digits10+1);
        int len = DataConvert::itoa(i, buf_.buffer() + buf_.data_end());
        buf_.data_end(buf_.data_end() + len);
        return *this;
    }

    inline self& operator<< (uint16_t i)
    {
        buf_.reserve(buf_.data_end() + std::numeric_limits<uint16_t>::digits10+1);
        int len = DataConvert::itoa(i, buf_.buffer()+ buf_.data_end());
        buf_.data_end(buf_.data_end()+len);
        return *this;
    }

    inline self& operator<< (int32_t i)
    {
        buf_.reserve(buf_.data_end() + std::numeric_limits<int32_t>::digits10+1);
        int len = DataConvert::itoa(i, buf_.buffer() + buf_.data_end());
        buf_.data_end(buf_.data_end() + len);
        return *this;
    }

    inline self& operator<< (uint32_t i)
    {
        buf_.reserve(buf_.data_end() + std::numeric_limits<uint32_t>::digits10+1);
        int len = DataConvert::uitoa(i, buf_.buffer() + buf_.data_end());
        buf_.data_end(buf_.data_end() + len);
        return *this;
    }

    inline self& operator<< (int64_t i)
    {
        buf_.reserve(buf_.data_end() + std::numeric_limits<int64_t>::digits10+1);
        int len = DataConvert::lltoa(i, buf_.buffer() + buf_.data_end());
        buf_.data_end(buf_.data_end() + len);

        return *this;
    }

    inline self& operator<< (uint64_t i)
    {
        buf_.reserve(buf_.data_end() + std::numeric_limits<uint64_t>::digits10+1);
        int len = DataConvert::ulltoa(i, buf_.buffer() + buf_.data_end());
        buf_.data_end(buf_.data_end() + len);
        return *this;
    }

    inline self& operator<<(float f)
    {
        buf_.reserve(buf_.data_end() + std::numeric_limits<float>::max_digits10+1);
        int len = std::snprintf(buf_.buffer() + buf_.data_end(), std::numeric_limits<float>::max_digits10, "%.12g", f);
        buf_.data_end(buf_.data_end() + len);
        return *this;
    }

    inline self& operator<<(double d)
    {
        buf_.reserve(buf_.data_end() + std::numeric_limits<double>::max_digits10+1);
        int len = std::snprintf(buf_.buffer() + buf_.data_end(), std::numeric_limits<double>::max_digits10, "%.12g", d);
        buf_.data_end(buf_.data_end() + len);
        return *this;
    }

    inline ByteBuffer& get_buffer()
    {
        return buf_;
    }

private:
    LogLevel level_ = LogLevel::kTRACE;
    uint_t   line_  = 0;
    const char *source_file_ = nullptr;
    const char *func_name_ = nullptr;

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
        const uint_t WRITE_BLOCK_SIZE = 8192;

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
    uint_t      timedwait_interval_us_ = 100000;
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

    virtual ~SyncWorker()
    {
    }

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
    OSApiFileAppender() = default;
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

class StdioFileAppender : public ILogAppender
{
public:
    StdioFileAppender() = default;
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
    static Logger & instance()
    {
        static Logger logger;
        return logger;
    }

    inline LogConfig & config()
    {
        return config_;
    }

    int init()
    {
        config_.update_config();
        switch (config_.get_appender_type()) {
        case LogAppenderType::CONSOLE:
            appender_ = new ConsoleAppender();
            if (config_.get_work_mode() == LogWorkMode::SYNC) {
                worker_ = new SyncWorker<NullMutex>(appender_);
            }
            break;
        case LogAppenderType::FILE:
            appender_ = new OSApiFileAppender(config_.filename());
            if (config_.get_work_mode() == LogWorkMode::SYNC) {
                switch (config_.get_lock_type()) {
                case LogLockType::NONE:
                    worker_ = new SyncWorker<NullMutex>(appender_);
                    break;
                case LogLockType::SPINLOCK:
                    worker_ = new SyncWorker<SpinlockMutex>(appender_);
                    break;
                case LogLockType::MUTEX:
                default:
                    worker_ = new SyncWorker<ThreadMutex>(appender_);
                    break;
                }
            }
            break;
        default:
            appender_ = new ConsoleAppender();
            worker_   = new SyncWorker<NullMutex>(appender_);
            break;
        }

        if (config_.get_work_mode() == LogWorkMode::ASYNC) {
            if (nullptr == worker_) {
                switch (config_.get_lock_type()) {
                case LogLockType::NONE:
                    worker_ = new AsyncWorker<NullMutex>(appender_, config_.get_buffer_size());
                    break;
                case LogLockType::SPINLOCK:
                    worker_ = new AsyncWorker<SpinlockMutex>(appender_, config_.get_buffer_size());
                    break;
                case LogLockType::MUTEX:
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
        return 0;
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
        return 1;
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
    Logger() = default;
    ~Logger()
    {
        fini();
    }

    ILogAppender *appender_ = nullptr;
    ILogWorker   *worker_   = nullptr;
    LogConfig     config_;

    bool init_flag_ = false;
};

ZRSOCKET_NAMESPACE_END

#define ZRSOCKET_LOG_SET_LOGLEVEL(loglevel)     zrsocket::Logger::instance().config().set_log_level(loglevel)
#define ZRSOCKET_LOG_SET_FILENAME(filename)     zrsocket::Logger::instance().config().set_filename(filename)
#define ZRSOCKET_LOG_SET_WORKMODE(workmode)     zrsocket::Logger::instance().config().set_work_mode(workmode)
#define ZRSOCKET_LOG_SET_LOCKTYPE(locktype)     zrsocket::Logger::instance().config().set_lock_type(locktype)
#define ZRSOCKET_LOG_SET_BUFFERSIZE(buffersize) zrsocket::Logger::instance().config().set_buffer_size(buffersize)
#define ZRSOCKET_LOG_INIT                       zrsocket::Logger::instance().init()

#define ZRSOCKET_LOG_BODY(logEvent,logLevel)                                \
            do {                                                            \
                zrsocket::Logger &logger = zrsocket::Logger::instance();    \
                zrsocket::LogConfig &config = logger.config();              \
                if (config.is_logged(logLevel)) {                           \
                    zrsocket::LogStream &stream = zrsocket::stream_;        \
                    stream.init(logLevel,__FILE__, __LINE__,__func__);      \
                    stream << logEvent;                                     \
                    logger.log(stream);                                     \
                }                                                           \
            } while (0)

#define ZRSOCKET_LOG_TRACE(e)   ZRSOCKET_LOG_BODY(e,zrsocket::LogLevel::kTRACE)
#define ZRSOCKET_LOG_DEBUG(e)   ZRSOCKET_LOG_BODY(e,zrsocket::LogLevel::kDEBUG)
#define ZRSOCKET_LOG_INFO(e)    ZRSOCKET_LOG_BODY(e,zrsocket::LogLevel::kINFO)
#define ZRSOCKET_LOG_WARN(e)    ZRSOCKET_LOG_BODY(e,zrsocket::LogLevel::kWARN)
#define ZRSOCKET_LOG_ERROR(e)   ZRSOCKET_LOG_BODY(e,zrsocket::LogLevel::kERROR)
#define ZRSOCKET_LOG_FATAL(e)   ZRSOCKET_LOG_BODY(e,zrsocket::LogLevel::kFATAL)

#endif
