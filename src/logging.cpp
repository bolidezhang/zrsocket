#include "zrsocket/logging.h"

ZRSOCKET_NAMESPACE_BEGIN

int LogStream::init(LogLevel level, const char* file, uint_t line, const char* function, Logger *logger)
{
    level_ = level;
    file_ = file;
    line_ = line;
    function_ = function;

    //每线程本地缓存精确到秒的时间字符串
    //buf_的前DATETIME_S_LEN个字节就是 精确到秒的时间字符串
    static const uint_t DATETIME_S_LEN = sizeof("1900-01-01 00:00:00.") - 1;
    static thread_local struct timespec last_ts_ = { 0 };

    //计算当前时间(精确到纳秒)
    Time &time = Time::instance();
    struct timespec ts;
    if (logger->config().log_time_source() == LogTimeSource::kFrameworkTime) {
        ts.tv_sec = time.current_time_s();
        ts.tv_nsec = static_cast<long>(time.current_time_ns() - ts.tv_sec * OSApi::NANOS_PER_SEC);
    }
    else {
        //uint64_t current_timestamp_ns = OSApi::steady_clock_counter();
        //uint64_t current_time_ns = (current_timestamp_ns - time.startup_timestamp_ns()) + time.startup_time_ns();
        //ts.tv_sec = current_time_ns / OSApi::NANOS_PER_SEC;
        //ts.tv_nsec = static_cast<long>(current_time_ns - ts.tv_sec * OSApi::NANOS_PER_SEC);
        uint64_t current_time_ns = TscClock::instance().now_ns();
        ts.tv_sec = current_time_ns / OSApi::NANOS_PER_SEC;
        ts.tv_nsec = static_cast<long>(current_time_ns - ts.tv_sec * OSApi::NANOS_PER_SEC);
    }

    //output current datetime(format: [YYYY-MM-DD HH:MM:SS.SSSSSSSSSZ] UTC时区)
    if (ts.tv_sec != last_ts_.tv_sec) {
        last_ts_.tv_sec = ts.tv_sec;

        struct tm buf_tm;
        OSApi::gmtime_s(&ts.tv_sec, &buf_tm);

        int  datetime_s_len = 0;
        char *datetime_s = buf_.data();

        //output year
        int len = DataConvert::uitoa(buf_tm.tm_year + 1900, datetime_s + datetime_s_len, DataConvert::max_digits10_int32);
        datetime_s_len += len;
        datetime_s[datetime_s_len] = '-';
        ++datetime_s_len;

        //output month
        if (buf_tm.tm_mon + 1 < 10) {
            datetime_s[datetime_s_len] = '0';
            ++datetime_s_len;
        }
        len = DataConvert::uitoa(buf_tm.tm_mon + 1, datetime_s + datetime_s_len, DataConvert::max_digits10_int32);
        datetime_s_len += len;
        datetime_s[datetime_s_len] = '-';
        ++datetime_s_len;

        //output mday
        if (buf_tm.tm_mon + 1 < 10) {
            datetime_s[datetime_s_len] = '0';
            ++datetime_s_len;
        }
        len = DataConvert::uitoa(buf_tm.tm_mon + 1, datetime_s + datetime_s_len, DataConvert::max_digits10_int32);
        if (buf_tm.tm_mday < 10) {
            datetime_s[datetime_s_len] = '0';
            ++datetime_s_len;
        }
        len = DataConvert::uitoa(buf_tm.tm_mday, datetime_s + datetime_s_len, DataConvert::max_digits10_int32);
        datetime_s_len += len;
        datetime_s[datetime_s_len] = ' ';
        ++datetime_s_len;

        //output hours
        if (buf_tm.tm_hour < 10) {
            datetime_s[datetime_s_len] = '0';
            ++datetime_s_len;
        }
        len = DataConvert::uitoa(buf_tm.tm_hour, datetime_s + datetime_s_len, DataConvert::max_digits10_int32);
        datetime_s_len += len;
        datetime_s[datetime_s_len] = ':';
        ++datetime_s_len;

        //output minutes
        if (buf_tm.tm_min < 10) {
            datetime_s[datetime_s_len] = '0';
            ++datetime_s_len;
        }
        len = DataConvert::uitoa(buf_tm.tm_min, datetime_s + datetime_s_len, DataConvert::max_digits10_int32);
        datetime_s_len += len;
        datetime_s[datetime_s_len] = ':';
        ++datetime_s_len;

        //output seconds
        if (buf_tm.tm_sec < 10) {
            datetime_s[datetime_s_len] = '0';
            ++datetime_s_len;
        }
        len = DataConvert::uitoa(buf_tm.tm_sec, datetime_s + datetime_s_len, DataConvert::max_digits10_int32);
        datetime_s_len += len;
        datetime_s[datetime_s_len] = '.';
    }
    buf_.data_end(DATETIME_S_LEN);

    //output nanoseconds
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
                buf_.write("000", 3);
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
    int len = DataConvert::uitoa(ts.tv_nsec, buf_.buffer() + end, DataConvert::max_digits10_int32);
    buf_.data_end(end + len);
    buf_.write("Z ", 2);

    //output level        
    buf_.write(LEVEL_NAMES[static_cast<int>(level)], LEVEL_NAME_LEN);
    buf_.write(' ');

    return 1;
}

ZRSOCKET_NAMESPACE_END
