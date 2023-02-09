//#include <cstdio>
#include <vector>
#include <chrono>
#include <sstream>
#include "test_seda.h"
int test_stage(int id, zrsocket::ISedaStage *stage, const char *out_title)
{
    TestApp& app = TestApp::instance();
    while (!app.ready_.load()) {
        std::this_thread::yield();
    }

    Test8SedaEvent  test8_event;
    Test16SedaEvent test16_event;
    int push_num = 0;
    //printf("%s thread_id:%d push start\n", out_title, id);
    ZRSOCKET_LOG_INFO(out_title << " thread_id:" << id << " push start");
    zrsocket::SteadyClockCounter scc;
    {
        zrsocket::MeasureCounterGuard<zrsocket::SteadyClockCounter, false> mcg(scc);
        if (app.seda_event_len_ <= 8) {
            for (int i = 0; i < app.num_times_; ++i) {
                test8_event.sequence = i;
                if (stage->push_event(&test8_event, -1, 0) >= 0) {
                    ++push_num;
                    ZRSOCKET_LOG_DEBUG(out_title << " thread_id:" << id << " push num:" << i <<" success");
                }
                else {
                    ZRSOCKET_LOG_DEBUG(out_title << " thread_id:" << id << " push num:" << i <<" failure");
                }
            }
        }
        else {
            for (int i = 0; i < app.num_times_; ++i) {
                test16_event.sequence = i;
                if (stage->push_event(&test16_event, -1, 0) >= 0) {
                    ++push_num;
                }
            }
        }
    }
    //printf("%s thread_id:%d push end num:%d spend_time:%lld us\n", out_title, id, push_num, scc.diff() / 1000LL);
    ZRSOCKET_LOG_INFO(out_title << " thread_id:" << id << " push end num:" << push_num << " spend_time:" << static_cast<uint64_t>(scc.diff() / 1000LL) << " us");
    app.push_num_.fetch_add(push_num, std::memory_order_relaxed);

    return 0;
}

template <typename TTest>
int startup_test(TTest test, bool thread_num_flag, int thread_num, zrsocket::ISedaStage *stage, const char *out_title)
{
    if (!thread_num_flag) {
        thread_num = 1;
    }

    TestApp &app = TestApp::instance();
    while (app.push_end_.load()) {
        std::this_thread::yield();
    }

    app.ready_.store(false);
    app.push_num_.store(0, std::memory_order_relaxed);
    app.handle_num_.store(0, std::memory_order_relaxed);
    std::vector<std::thread *> threads;
    threads.reserve(thread_num);
    for (auto i = 0; i < thread_num; ++i) {
        threads.emplace_back(new std::thread(test, i, stage, out_title));
    }

    app.test_counter_.update_start_counter();
    app.ready_.store(true);
    for (auto &t : threads) {
        t->join();
        delete t;
    }    
    threads.clear();
    app.push_end_.store(true, std::memory_order_relaxed);

    return 0;
}

int main(int argc, char *argv[])
{
    printf("please use the format: <thread_num> <num_times> <seda_thread_num> <seda_queue_size> <seda_event_len>\n\
<logLevel 0:Trace 1:Debug 2:Info 3Warn 4:Error 5Fatal> <logAppenderType 0:Console 1:File> <logWorkMode 0:Sync 1:Async> <logLockType 0:None 1:Spinlock 2:Mutex> <bufferSize:1024*1024*4>\n");

    if (argc < 2) {
        return 0;
    }

    TestApp &app = TestApp::instance();
    if (argc > 6) {
        auto level = static_cast<zrsocket::LogLevel>(std::atoi(argv[6]));
        ZRSOCKET_LOG_SET_LOGLEVEL(level);
    }
    if (argc > 7) {
        auto type = static_cast<zrsocket::LogAppenderType>(std::atoi(argv[7]));
        if (type == zrsocket::LogAppenderType::FILE) {
            ZRSOCKET_LOG_SET_FILENAME("./test_seda.log");
        }
    }
    if (argc > 8) {
        auto mode = static_cast<zrsocket::LogWorkMode>(std::atoi(argv[8]));
        ZRSOCKET_LOG_SET_WORKMODE(mode);
    }
    if (argc > 9) {
        auto type = static_cast<zrsocket::LogLockType>(std::atoi(argv[9]));
        ZRSOCKET_LOG_SET_LOCKTYPE(type);
    }
    if (argc > 10) {
        auto buffersize = std::atoi(argv[10]);
        ZRSOCKET_LOG_SET_BUFFERSIZE(buffersize);
    }
    ZRSOCKET_LOG_INIT;
    ZRSOCKET_LOG_INFO("start...");
  
    //TestApp &app = TestApp::instance();
    //ZRSOCKET_LOG_INFO("please use the format: <thread_num> <num_times> <seda_thread_num> <seda_queue_size> <seda_event_len>");
    if (argc > 5) {
        app.seda_event_len_     = atoi(argv[5]);
        app.seda_queue_size_    = atoi(argv[4]);
        app.seda_thread_num_    = atoi(argv[3]);
        app.num_times_          = atoi(argv[2]);
        app.thread_num_         = atoi(argv[1]);
    }
    else {
        switch (argc) {
        case 5:
            app.seda_queue_size_ = atoi(argv[4]);
        case 4:
            app.seda_thread_num_ = atoi(argv[3]);
        case 3:
            app.num_times_ = atoi(argv[2]);
        case 2:
            app.thread_num_ = atoi(argv[1]);
            break;
        default:
            break;
        }
    }
    //std::printf("thread_num:%d, num_times:%ld, seda_thread_num:%d, seda_queue_size:%d, seda_event_len:%d\n", 
    //    app.thread_num_, app.num_times_, app.seda_thread_num_, app.seda_queue_size_, app.seda_event_len_);
    ZRSOCKET_LOG_INFO("thread_num:" << app.thread_num_ << " num_times:" << app.num_times_ << " seda_thread_num:" << app.seda_thread_num_
        << " seda_queue_size:" << app.seda_queue_size_ << " seda_event_len:" << app.seda_event_len_);

    return app.run();
}

int TestApp::do_init()
{
    ZRSOCKET_LOG_INFO("do_init start");
    //mpsc_stage_.open(seda_thread_num_, seda_queue_size_, seda_event_len_);
    //mpmc_stage_.open(seda_thread_num_, seda_queue_size_, seda_event_len_);
    doublebuffer_spinlock_stage_.open(seda_thread_num_, seda_queue_size_, seda_event_len_);
    //doublebuffer_mutex_stage_.open(seda_thread_num_, seda_queue_size_, seda_event_len_);
    //spsc_steady_stage_.open(seda_thread_num_, seda_queue_size_, seda_event_len_);
    //spsc_atomic_stage_.open(seda_thread_num_, seda_queue_size_, seda_event_len_);
    //spsc_normal_stage_.open(seda_thread_num_, seda_queue_size_, seda_event_len_);

    //startup_test(test_stage, true,  thread_num_, &mpsc_stage_, "mpsc_stage");
    //startup_test(test_stage, true, thread_num_, &mpmc_stage_, "mpmc_stage");
    startup_test(test_stage, true,  thread_num_, &doublebuffer_spinlock_stage_, "doublebuffer_spinlock_stage");
    //startup_test(test_stage, true,  thread_num_, &doublebuffer_mutex_stage_, "doublebuffer_mutex_stage");
    //startup_test(test_stage, false, thread_num_, &spsc_steady_stage_, "spsc_steady_stage");
    //startup_test(test_stage, false, thread_num_, &spsc_atomic_stage_, "spsc_atomic_stage");
    //startup_test(test_stage, false, thread_num_, &spsc_normal_stage_, "spsc_normal_stage");

    return 0;
}
