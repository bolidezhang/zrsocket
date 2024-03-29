﻿#pragma once

#ifndef TEST_SEDA_H
#define TEST_SEDA_H
#include "zrsocket/zrsocket.h"
#include "biz_stage_handler.h"

//#pragma pack(push)  //保存对齐状态
//#pragma pack(1)

struct TestEventType
{
    enum
    {
        EVENT_TEST8  = zrsocket::SedaEventTypeId::USER_START_NUMBER,
        EVENT_TEST16,
    };
};

struct Test8SedaEvent : public zrsocket::SedaEvent
{
    short command  = 0;
    int   sequence = 0;

    inline Test8SedaEvent() 
        : zrsocket::SedaEvent(sizeof(Test8SedaEvent), TestEventType::EVENT_TEST8)
    {
    }

    inline ~Test8SedaEvent() = default;
};

struct Test16SedaEvent : public zrsocket::SedaEvent
{
    short   command   = 0;
    int     sequence  = 0;
    int64_t attach_id = 0;

    inline Test16SedaEvent() 
        : zrsocket::SedaEvent(sizeof(Test16SedaEvent), TestEventType::EVENT_TEST16)
    {
    }

    inline ~Test16SedaEvent() = default;
};

//#pragma pack(pop)   //恢复对齐状态

class TestApp : public zrsocket::Application<TestApp, zrsocket::SpinMutex>
{
public:
    using super = zrsocket::Application<TestApp, zrsocket::SpinMutex>;

    TestApp() = default;
    ~TestApp() = default;

    int do_init();
    int do_fini()
    {
        zrsocket::Logger::instance().fini();
        printf("do_fini\n");
        return 0;
    }

    int do_signal(int signum)
    {
        stop_flag_.store(true, std::memory_order_relaxed);
        main_event_loop_.loop_wakeup();
        printf("do_signal signum:%d\n", signum);
        return 0;
    }

public:
    zrsocket::SedaStage<BizStageHandler, zrsocket::MPSCEventTypeQueue>                                      mpsc_stage_;
    zrsocket::SedaStage<BizStageHandler, zrsocket::MPMCEventTypeQueue>                                      mpmc_stage_;
    zrsocket::SedaStage<BizStageHandler, zrsocket::DoubleBufferEventTypeQueue<zrsocket::SpinlockMutex> >    doublebuffer_spinlock_stage_;
    zrsocket::SedaStage<BizStageHandler, zrsocket::DoubleBufferEventTypeQueue<zrsocket::ThreadMutex> >      doublebuffer_mutex_stage_;
    zrsocket::SedaStage<BizStageHandler, zrsocket::SPSCVolatileEventTypeQueue>                              spsc_volatile_stage_;
    zrsocket::SedaStage<BizStageHandler, zrsocket::SPSCAtomicEventTypeQueue>                                spsc_atomic_stage_;
    zrsocket::SedaStage<BizStageHandler, zrsocket::SPSCNormalEventTypeQueue>                                spsc_normal_stage_;
    
    zrsocket::AtomicBool ready_      = ATOMIC_VAR_INIT(false);
    zrsocket::AtomicBool push_end_   = ATOMIC_VAR_INIT(false);

    zrsocket::AtomicUInt push_num_   = ATOMIC_VAR_INIT(0);
    zrsocket::AtomicUInt handle_num_ = ATOMIC_VAR_INIT(0);

    int seda_thread_num_    = 1;
    int seda_queue_size_    = 1000000;
    int seda_event_len_     = 8;
    int thread_num_         = 1;
    int num_times_          = 10000000;

    zrsocket::SteadyClockCounter test_counter_;
};

#endif
