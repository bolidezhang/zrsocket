﻿// Some compilers (e.g. VC++) benefit significantly from using this. 
// We've measured 3-4% build speed improvements in apps as a result 
#pragma once

#ifndef ZRSOCKET_MEASURE_COUNTER_H
#define ZRSOCKET_MEASURE_COUNTER_H
#include <iostream>
#include "os_api.h"

ZRSOCKET_NAMESPACE_BEGIN

//不受系统时钟的影响的计数器
class SteadyClockCounter
{
public:
    inline SteadyClockCounter()  = default;
    inline ~SteadyClockCounter() = default;
    inline SteadyClockCounter(const SteadyClockCounter&) = default;
    inline SteadyClockCounter(SteadyClockCounter&&) = default;
    inline SteadyClockCounter& operator=(const SteadyClockCounter&) = default;

    static uint64_t now()
    {
        return OSApi::steady_clock_counter();
    }

    inline void update_start_counter()
    {
        start_counter_ = now();
    }
    inline void update_end_counter() 
    {
        end_counter_ = now();
    }
    inline uint64_t get_start_counter() const
    {
        return start_counter_;
    }
    inline uint64_t get_end_counter() const
    {
        return end_counter_;
    }
    inline int64_t diff() const
    {
        return end_counter_ - start_counter_;
    }

private:
    uint64_t start_counter_ = 0;
    uint64_t end_counter_   = 0;
};

class SystemClockCounter
{
public:
    inline SystemClockCounter()  = default;
    inline ~SystemClockCounter() = default;
    inline SystemClockCounter(const SystemClockCounter&) = default;
    inline SystemClockCounter(SystemClockCounter&&) = default;
    inline SystemClockCounter&operator=(const SystemClockCounter&) = default;

    static uint64_t now()
    {
        return OSApi::system_clock_counter();
    }

    inline void update_start_counter()
    {
        start_counter_ = now();
    }
    inline void update_end_counter()
    {
        end_counter_ = now();
    }
    inline uint64_t get_start_counter() const
    {
        return start_counter_;
    }
    inline uint64_t get_end_counter() const
    {
        return end_counter_;
    }
    inline int64_t diff() const
    {
        return end_counter_ - start_counter_;
    }

private:
    uint64_t start_counter_ = 0;
    uint64_t end_counter_   = 0;
};

template <typename TMeasureCounter, bool owner = true>
class MeasureCounterGuard
{
public:
    inline MeasureCounterGuard(const char *out_prefix = "", 
        const char *out_postfix = "\n", 
        std::ostream &out = std::cout)
        : out_prefix_(out_prefix)
        , out_postfix_(out_postfix)
        , out_(out)
    {
        mc_.update_start_counter();
    }

    inline ~MeasureCounterGuard()
    {
        mc_.update_end_counter();
        out_ << out_prefix_ << mc_.diff() << out_postfix_;
    }

    MeasureCounterGuard() = delete;
    MeasureCounterGuard(const MeasureCounterGuard&) = delete;
    MeasureCounterGuard(MeasureCounterGuard&&) = delete;
    MeasureCounterGuard& operator=(const MeasureCounterGuard&) = delete;
    MeasureCounterGuard& operator=(MeasureCounterGuard&&) = delete;

private:
    TMeasureCounter mc_;
    const char      *out_prefix_;
    const char      *out_postfix_;
    std::ostream    &out_;
};

template <typename TMeasureCounter>
class MeasureCounterGuard<TMeasureCounter, false>
{
public:
    inline MeasureCounterGuard(TMeasureCounter &mc)
        : mc_(mc)
    {
        mc_.update_start_counter();
    }

    inline ~MeasureCounterGuard()
    {
        mc_.update_end_counter();
    }

    MeasureCounterGuard() = delete;
    MeasureCounterGuard(const MeasureCounterGuard&) = delete;
    MeasureCounterGuard(MeasureCounterGuard&&) = delete;
    MeasureCounterGuard& operator=(const MeasureCounterGuard&) = delete;
    MeasureCounterGuard& operator=(MeasureCounterGuard&&) = delete;

private:
    TMeasureCounter &mc_;
};

ZRSOCKET_NAMESPACE_END

#endif
