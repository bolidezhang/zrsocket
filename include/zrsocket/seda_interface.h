#ifndef ZRSOCKET_SEDA_INTERFACE_H
#define ZRSOCKET_SEDA_INTERFACE_H

#include "config.h"
#include "base_type.h"
#include "seda_event.h"
#include "seda_timer.h"

ZRSOCKET_NAMESPACE_BEGIN

class ISedaEventQueue;
class ISedaStage;
class ISedaStageHandler;
class ISedaStageThread;

#define ZRSOCKET_SEDA_DEFAULT_IDLE_INTERVAL       1000      //millisecond(ms)
#define ZRSOCKET_SEDA_DEFAULT_TIMEDWAIT_INTERVAL  2000      //microsecond(us)
#define ZRSOCKET_SEDA_TIMER_EVENT_FACTOR          10000     //每处理SEDA_TIMER_EVENT_FACTOR个事件后，检测定时器

//优先级
struct SedaPriority
{
    enum type
    {
        UNKNOWN_PRIOITY = 0,
        LOW_PRIORITY    = 1,
        HIGH_PRIORITY   = 2,
    };
};

class ISedaEventQueue
{
public:
    ISedaEventQueue() = default;
    virtual ~ISedaEventQueue() = default;

    virtual int         init(uint_t capacity, uint_t event_len) = 0;
    virtual void        clear() = 0;
    virtual int         push(const SedaEvent *event) = 0;
    virtual SedaEvent * pop() = 0;
    virtual uint_t      capacity() const = 0;
    virtual uint_t      size() const = 0;
    virtual uint_t      free_size() const = 0;
    virtual bool        empty() const = 0;
};

class ISedaStageHandler
{
public:
    ISedaStageHandler() = default;
    virtual ~ISedaStageHandler() = default;

    virtual int handle_open()  = 0;
    virtual int handle_close() = 0;
    virtual int handle_event(const SedaEvent *event) = 0;
};

class ISedaStage
{
public:
    ISedaStage() = default;
    virtual ~ISedaStage() = default;
    virtual int     open(uint_t thread_number, uint_t queue_max_size, uint_t event_len,
                        uint_t timedwait_interval_us, bool timedwait_signal, int type,
                        uint_t batch_size, bool is_priority, bool is_shared_queue) = 0;
    virtual int     close() = 0;
    virtual int     push_event(const SedaEvent *event, int thread_number, int priority) = 0;
    virtual int     type() const = 0;
    virtual int     event_count() const = 0;
    virtual uint_t  batch_size() const = 0;

    virtual int     pop_event(void *push_queue, uint_t batch_size)
    {
        return 0;
    }
};

class ISedaStageThread
{
public:
    ISedaStageThread() = default;
    virtual ~ISedaStageThread() = default;

    virtual int start() = 0;
    virtual int stop() = 0;
    virtual int join() = 0;
    virtual int push_event(const SedaEvent *event) = 0;

    virtual SedaTimer * set_timer(uint_t interval_ms, SedaTimer::TimerParam param) = 0;
    virtual int cancel_timer(SedaTimer *timer) = 0;
    virtual int update_timer(SedaTimer *timer) = 0;
    virtual int enable_timer_event(uint_t capacity) = 0;

    virtual SedaTimer * set_lru_timer(int slot, SedaTimer::TimerParam param) = 0;
    virtual int cancel_lru_timer(int slot, SedaTimer *timer) = 0;
    virtual int update_lru_timer(int slot, SedaTimer *timer) = 0;
    virtual int enable_lru_timers(SedaStageThreadLRUSlotInfo *slots, int slot_count) = 0;

    virtual int enable_idle_event(bool enabled) = 0;
    virtual int set_idle_interval(uint_t idle_interval_ms) = 0;
    virtual int set_sleep_interval(uint_t sleep_interval_ms) = 0;
    virtual int set_timedwait_interval(uint_t timedwait_interval_ms) = 0;

    virtual ISedaStage * get_stage() const = 0;
    virtual uint_t get_thread_index() const = 0;
};

ZRSOCKET_NAMESPACE_END

#endif
