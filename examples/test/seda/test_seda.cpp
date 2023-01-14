#include <cstdio>
#include <vector>
#include <chrono>
#include "test_seda.h"

int test_spinlock_stage(int id)
{
    TestApp &app = TestApp::instance();
    while (!app.ready_.load()) {
        std::this_thread::yield();
    }

    Test8SedaEvent test8_event;
    Test8SedaEvent test16_event;
    int push_num = 0;
    auto start_timestamp = zrsocket::OSApi::timestamp();
    printf("test_spinlock_stage thread_id:%d push start_timestamp:%lld\n", id, start_timestamp);
    if (app.seda_event_len_ <= 8) {
        for (int i = 0; i < app.num_times_; ++i) {
            test8_event.sequence = i;
            if (app.spinlock_stage_.push_event(&test8_event) >= 0) {
                ++push_num;
            }
        }
    }
    else {
        for (int i = 0; i < app.num_times_; ++i) {
            test16_event.sequence = i;
            if (app.spinlock_stage_.push_event(&test16_event) >= 0) {
                ++push_num;
            }
        }
    }
    auto end_timestamp = zrsocket::OSApi::timestamp();
    printf("test_spinlock_stage thread_id:%d push num:%d, spend_time:%lld us\n", id, push_num, end_timestamp - start_timestamp);
    app.push_num_.fetch_add(push_num, std::memory_order_relaxed);

    return 0;
}

int test_mutex_stage(int id)
{
    TestApp &app = TestApp::instance();
    while (!app.ready_.load()) {
        std::this_thread::yield();
    }

    Test8SedaEvent test8_event;
    Test8SedaEvent test16_event;
    int push_num = 0;
    auto start_timestamp = zrsocket::OSApi::timestamp();
    printf("test_mutex_stage thread_id:%d push start_timestamp:%lld\n", id, start_timestamp);
    if (app.seda_event_len_ <= 8) {
        for (int i = 0; i < app.num_times_; ++i) {
            test8_event.sequence = i;
            if (app.mutex_stage_.push_event(&test8_event) >= 0) {
                ++push_num;
            }
        }
    }
    else {
        for (int i = 0; i < app.num_times_; ++i) {
            test16_event.sequence = i;
            if (app.mutex_stage_.push_event(&test16_event) >= 0) {
                ++push_num;
            }
        }
    }
    auto end_timestamp = zrsocket::OSApi::timestamp();
    printf("test_mutex_stage thread_id:%d push num:%d, spend_time:%lld us\n", id, push_num, end_timestamp - start_timestamp);
    app.push_num_.fetch_add(push_num, std::memory_order_relaxed);

    return 0;
}

template <typename TTest>
int startup_test(TTest test, int thread_num)
{
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
        threads.emplace_back(new std::thread(test, i));
    }

    app.startup_test_timestamp_ = zrsocket::OSApi::timestamp();
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
    TestApp &app = TestApp::instance();

    printf("please use the format: <thread_num> <num_times> <seda_thread_num> <seda_queue_size> <seda_event_len>\n");
    if (argc > 1) {
        app.thread_num_ = atoi(argv[1]);
    }
    if (argc > 2) {
        app.num_times_ = atoi(argv[2]);
    }
    if (argc > 3) {
        app.seda_thread_num_ = atoi(argv[3]);
    }
    if (argc > 4) {
        app.seda_queue_size_ = atoi(argv[4]);
    }
    if (argc > 5) {
        app.seda_event_len_ = atoi(argv[5]);
    }
    printf("thread_num:%d, num_times:%ld, seda_thread_num:%d, seda_queue_size:%d, seda_event_len:%d\n", 
        app.thread_num_, app.num_times_, app.seda_thread_num_, app.seda_queue_size_, app.seda_event_len_);

    return app.run();
}

int TestApp::do_init()
{
    printf("do_init\n");
    spinlock_stage_.open(seda_thread_num_, seda_queue_size_, seda_event_len_);
    mutex_stage_.open(seda_thread_num_, seda_queue_size_, seda_event_len_);
    startup_test(test_spinlock_stage, thread_num_);
    startup_test(test_mutex_stage, thread_num_);
    return 0;
}
