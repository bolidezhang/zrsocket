#include <cstdio>
#include <vector>
#include <chrono>
#include "test_seda.h"

int test_stage(int id, zrsocket::ISedaStage *stage, const char *out_title)
{
    TestApp& app = TestApp::instance();
    while (!app.ready_.load()) {
        std::this_thread::yield();
    }

    Test8SedaEvent test8_event;
    Test8SedaEvent test16_event;
    int push_num = 0;
    printf("%s thread_id:%d push start\n", out_title, id);
    zrsocket::SteadyClockCounter scc;
    {
        zrsocket::MeasureCounterGuard<zrsocket::SteadyClockCounter, false> mcg(scc);
        if (app.seda_event_len_ <= 8) {
            for (int i = 0; i < app.num_times_; ++i) {
                test8_event.sequence = i;
                if (stage->push_event(&test8_event, -1, 0) >= 0) {
                    ++push_num;
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
    printf("%s thread_id:%d push end num:%d spend_time:%lld us\n", out_title, id, push_num, scc.diff() / 1000LL);
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
    while (app.push_end_.load(std::memory_order_relaxed)) {
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
    /*
    {
        zrsocket::OSApiFile f;
        int ret = f.open("a.txt", O_WRONLY|O_APPEND|O_CREAT, S_IWRITE);
        int err = zrsocket::OSApi::get_errno();
        ret = f.write("abcd", sizeof("abcd")-1);

        const char *p  = "abcd";
        const char *p2 = "efg";
        ZRSOCKET_IOVEC iov[2];
        iov[0].iov_base = const_cast<char *>(p);
        iov[0].iov_len  = 4;
        iov[1].iov_base = const_cast<char *>(p2);
        iov[1].iov_len  = 3;
        ret = f.writev(iov, 2);
        ret = f.writev(iov, 2);
        err = zrsocket::OSApi::get_errno();
        f.close();
        std::cout << "open file ret:" << ret << " error:" << err << "\n";
    }
    */

    //show MeasureCounterGuard
    printf("show MeasureCounterGuard\n");
    zrsocket::SteadyClockCounter scc;
    {
        zrsocket::MeasureCounterGuard<zrsocket::SteadyClockCounter, false> mcg(scc);
        zrsocket::OSApi::sleep_ms(10);
    }
    printf("MeasureCounterGuard<zrsocket::SteadyClockCounter, false> sleep_ms(10) spend time:%lld ms\n", scc.diff() / 1000000LL);
    {
        zrsocket::MeasureCounterGuard<zrsocket::SteadyClockCounter> mcg("MeasureCounterGuard<zrsocket::SteadyClockCounter> sleep_ms(10) spend time:", " ns\n\n");
        zrsocket::OSApi::sleep_ms(10);
    }

    TestApp &app = TestApp::instance();
    printf("please use the format: <thread_num> <num_times> <seda_thread_num> <seda_queue_size> <seda_event_len>\n");
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
    printf("thread_num:%d, num_times:%ld, seda_thread_num:%d, seda_queue_size:%d, seda_event_len:%d\n", 
        app.thread_num_, app.num_times_, app.seda_thread_num_, app.seda_queue_size_, app.seda_event_len_);

    return app.run();
}

int TestApp::do_init()
{
    printf("do_init start\n");
    //mpsc_stage_.open(seda_thread_num_, seda_queue_size_, seda_event_len_);
    doublebuffer_spinlock_stage_.open(seda_thread_num_, seda_queue_size_, seda_event_len_);
    doublebuffer_mutex_stage_.open(seda_thread_num_, seda_queue_size_, seda_event_len_);
    spsc_volatile_stage_.open(seda_thread_num_, seda_queue_size_, seda_event_len_);
    spsc_atomic_stage_.open(seda_thread_num_, seda_queue_size_, seda_event_len_);
    //mpmc_stage_.open(seda_thread_num_, seda_queue_size_, seda_event_len_);

    //startup_test(test_stage, true,  thread_num_, &mpsc_stage_, "mpsc_stage");
    startup_test(test_stage, true,  thread_num_, &doublebuffer_spinlock_stage_, "doublebuffer_spinlock_stage");
    startup_test(test_stage, true,  thread_num_, &doublebuffer_mutex_stage_, "doublebuffer_mutex_stage");
    startup_test(test_stage, false, thread_num_, &spsc_volatile_stage_, "spsc_volatile_stage");
    startup_test(test_stage, false, thread_num_, &spsc_atomic_stage_, "spsc_atomic_stage");
    //startup_test(test_stage, true, thread_num_, &mpmc_stage_, "mpmc_stage");

    return 0;
}
