// Some compilers (e.g. VC++) benefit significantly from using this. 
// We've measured 3-4% build speed improvements in apps as a result 
#pragma once

#ifndef ZRSOCKET_TIME_H
#define ZRSOCKET_TIME_H
#include "config.h"
#include "base_type.h"
#include "os_api.h"

ZRSOCKET_NAMESPACE_BEGIN

class Time
{
public:
    static Time & instance()
    {
        static Time t;
        return t;
    }

    inline void update_time()
    {
        current_timestamp_ns_   = OSApi::steady_clock_counter();
        current_timestamp_us_   = current_timestamp_ns_ / OSApi::TIME_INTERVAL_UNIT;
        current_timestamp_ms_   = current_timestamp_us_ / OSApi::TIME_INTERVAL_UNIT;
        current_timestamp_s_    = current_timestamp_ms_ / OSApi::TIME_INTERVAL_UNIT;

        current_time_ns_        = OSApi::system_clock_counter();
        current_time_us_        = current_time_ns_ / OSApi::TIME_INTERVAL_UNIT;
        current_time_ms_        = current_time_us_ / OSApi::TIME_INTERVAL_UNIT;
        current_time_s_         = current_time_ms_ / OSApi::TIME_INTERVAL_UNIT;
    }

    inline uint64_t current_timestamp_ns() const
    {
        return current_timestamp_ns_;
    }

    inline uint64_t current_timestamp_us() const
    {
        return current_timestamp_us_;
    }

    inline uint64_t current_timestamp_ms() const
    {
        return current_timestamp_ms_;
    }

    inline uint64_t current_timestamp_s() const
    {
        return current_timestamp_s_;
    }

    inline uint64_t current_time_ns() const
    {
        return current_time_ns_;
    }

    inline uint64_t current_time_us() const
    {
        return current_time_us_;
    }

    inline uint64_t current_time_ms() const
    {
        return current_time_ms_;
    }

    inline uint64_t current_time_s() const
    {
        return current_time_s_;
    }

    inline uint64_t startup_timestamp_ns() const
    {
        return startup_timestamp_ns_;
    }

    inline uint64_t startup_time_ns() const
    {
        return startup_time_ns_;
    }

private:
    Time()
    {
        update_time();
        startup_time_ns_ = current_time_ns_;
        startup_timestamp_ns_ = current_timestamp_ns_;
    }

    ~Time() = default;
    Time(const Time &) = delete;
    Time & operator=(const Time &) = delete;

private:
    //系统当前时间戳,不受系统时间被用户改变的影响(用于减少取当前时间戳的系统调用开销) 只能用于计时
    uint64_t current_timestamp_ns_;
    uint64_t current_timestamp_us_;
    uint64_t current_timestamp_ms_;
    uint64_t current_timestamp_s_;

    //系统当前时间(用于减少取当前时间的系统调用开销)
    uint64_t current_time_ns_;
    uint64_t current_time_us_;
    uint64_t current_time_ms_;
    uint64_t current_time_s_;

    //启动时间戳和启动时间
    uint64_t startup_timestamp_ns_;
    uint64_t startup_time_ns_;
};

ZRSOCKET_NAMESPACE_END

#endif
