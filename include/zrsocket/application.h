// Some compilers (e.g. VC++) benefit significantly from using this. 
// We've measured 3-4% build speed improvements in apps as a result 
#pragma once

#ifndef ZRSOCKET_APPLICATION_H
#define ZRSOCKET_APPLICATION_H

#include <csignal>
#include "atomic.h"
#include "config.h"
#include "global.h"
#include "select_event_loop.h"
#include "epoll_event_loop.h"
#include "timer.h"
#include "timer_queue.h"
#include "time.h"
#include "os_api.h"
#include "os_constant.h"

ZRSOCKET_NAMESPACE_BEGIN

template <class TApp, class TMutex>
class Application
{
public:
    static TApp & instance()
    {
        static TApp app;
        return app;
    }

    virtual int do_init()
    {
        return 0;
    }

    virtual int do_fini()
    {
        return 0;
    }

    virtual int do_signal(int signum)
    {
        return 0;
    }

    virtual int init()
    {
        if (do_init() < 0) {
            return -1;
        }
        main_event_loop_.open();
        init_flag_ = true;
        return 0;
    }

    virtual int fini()
    {
        do_fini();
        main_event_loop_.close();
        return 0;
    }

    virtual void handle_signal(int signum)
    {
        if (do_signal(signum) >= 0) {
            stop_flag_.store(true, std::memory_order_relaxed);
        }
    }

    virtual int run()
    {
        if (!init_flag_) {
            if (init() < 0) {
                return -1;
            }
        }

        if (init_flag_) {
            Time &t = Time::instance();
            while (!stop_flag_.load(std::memory_order_relaxed)) {
                t.update_time();
                main_event_loop_.loop(timeout_us_);
            }
        }

        fini();
        return 0;
    }

    EventLoop * main_event_loop() const
    {
        return &main_event_loop_;
    }

protected:
    Application()
    {
        OSApi::socket_init(2, 2);
        OSConstant::instance();
        TscClock::instance();
        Time::instance();
        Global::instance();
        Logger::instance();
        stop_flag_.store(false, std::memory_order_relaxed);
        ::signal(SIGTERM, Application<TApp, TMutex>::signal);
        ::signal(SIGINT,  Application<TApp, TMutex>::signal);
#ifndef ZRSOCKET_OS_WINDOWS
        ::signal(SIGPIPE, SIG_IGN);
#endif
    }

    virtual ~Application()
    {
        OSApi::socket_fini();
    }

private:
    Application(const Application&) = delete;
    Application(Application&&) = delete;
    Application& operator=(const Application&) = delete;
    Application& operator=(Application&&) = delete;

    static void signal(int signum)
    {
        Application<TApp, TMutex> &app = Application<TApp, TMutex>::instance();
        app.handle_signal(signum);
    }

protected:

    //Main EventLoop
#ifdef ZRSOCKET_OS_WINDOWS
    SelectEventLoop<TMutex> main_event_loop_;
#else
    EpollEventLoop<TMutex>  main_event_loop_;
#endif

    //超时间隔(us)
    int64_t timeout_us_ = 10000;

    //退出/停止标记
    AtomicBool stop_flag_;

    //初始化
    bool init_flag_ = false;
};

ZRSOCKET_NAMESPACE_END

#endif
