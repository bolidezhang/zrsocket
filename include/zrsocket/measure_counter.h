// Some compilers (e.g. VC++) benefit significantly from using this. 
// We've measured 3-4% build speed improvements in apps as a result 
#pragma once

#ifndef ZRSOCKET_MEASURE_COUNTER_H
#define ZRSOCKET_MEASURE_COUNTER_H
#include <iostream>
#include "os_api.h"
#include "tsc_clock.h"

ZRSOCKET_NAMESPACE_BEGIN

template<uint64_t (*TClock)()>
class ClockCounter
{
public:
    inline ClockCounter() = default;
    inline ~ClockCounter() = default;
    inline ClockCounter(const ClockCounter&) = default;
    inline ClockCounter(ClockCounter&&) = default;
    inline ClockCounter& operator=(const ClockCounter&) = default;

    static inline uint64_t now()
    {
        return TClock();
    }

    inline void reset()
    {
        start_counter_ = 0;
        end_counter_   = 0;
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

using SystemClockCounter   = ClockCounter<OSApi::system_clock_counter>;
using SteadyClockCounter   = ClockCounter<OSApi::steady_clock_counter>;
using TscClockCounter      = ClockCounter<TscClock::rdtsc>;
using TscNsClockCounter    = ClockCounter<TscClock::now_ns>;

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
