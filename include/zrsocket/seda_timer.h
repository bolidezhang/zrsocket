#ifndef ZRSOCKET_SEDA_TIMER_H
#define ZRSOCKET_SEDA_TIMER_H
#include "config.h"
#include "base_type.h"

ZRSOCKET_NAMESPACE_BEGIN

class SedaTimerList;
class SedaTimerQueue;
class SedaLRUTimerQueue;

class ZRSOCKET_EXPORT SedaTimer
{
public:
    typedef int64_t TimerParam;

    SedaTimer()
        : param_(0)
        , end_clock_ms_(0)
        , interval_ms_(0)
        , is_active_(false)
    {
        prev_ = this;
        next_ = this;
    }

    ~SedaTimer()
    {
    }

    inline int init()
    {
        prev_           = this;
        next_           = this;
        param_          = 0;
        end_clock_ms_   = 0;
        interval_ms_    = 0;
        is_active_      = false;
        return 0;
    }

    inline SedaTimer * prev() const
    {
        return prev_;
    }

    inline SedaTimer * next() const
    {
        return next_;
    }

    inline bool alone() const
    {
        return next_ == this;
    }

    inline TimerParam param() const
    {
        return param_;
    }

    inline uint_t interval() const
    {
        return interval_ms_;
    }

    virtual int timeout() 
    {
        return 0; 
    }

protected:
    SedaTimer   *prev_;             //初始指向其自身this,防止因NULL而出现异常
    SedaTimer   *next_;             //初始指向其自身this,防止因NULL而出现异常

    TimerParam   param_;            //外部参数
    uint64_t     end_clock_ms_;     //定时器触发时间点(时间单位为ms)
    uint_t       interval_ms_;      //定时间隔(时间单位为ms)
    bool         is_active_;        //表示此定时器是否已激活

    friend class SedaTimerList;
    friend class SedaTimerQueue;
    friend class SedaLRUTimerQueue;
};

struct SedaStageThreadLRUSlotInfo
{
    uint_t  interval_ms;
    uint_t  capacity;
};

ZRSOCKET_NAMESPACE_END

#endif
