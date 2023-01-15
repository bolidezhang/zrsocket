#include <cstdio>
#include <vector>
#include <chrono>
#include "test_mutex.h"

#define TEST_MUTEX 1
#define TEST_GET_TIME 1
zrsocket::AtomicBool ready_ = ATOMIC_VAR_INIT(false);

#if TEST_GET_TIME

int test_time(int id, int times)
{
    while (!ready_.load()) {
        std::this_thread::yield();
    }

    int64_t start_timestamp = 0;
    int64_t end_timestamp = 0;

    int64_t update_time = 0;
    start_timestamp = zrsocket::OSApi::timestamp();
    printf("start process_time:osapi_time thread_id:%d, timestamp:%lld\n", id, start_timestamp);
    start_timestamp = zrsocket::OSApi::timestamp();
    for (int i = 0; i < times; ++i) {
        update_time = zrsocket::OSApi::time_ns();
    }
    end_timestamp = zrsocket::OSApi::timestamp();
    printf("end process_time:osapi_time thread_id:%d, update_time:%lld, spend_time:%lld us times:%d\n", id, update_time, end_timestamp-start_timestamp, times);

    int64_t update_timestamp = 0;
    start_timestamp = zrsocket::OSApi::timestamp();
    printf("start process_time:osapi_timestamp id:%d, timestamp:%lld\n", id, start_timestamp);
    start_timestamp = zrsocket::OSApi::timestamp();
    for (int i = 0; i < times; ++i) {
        update_timestamp = zrsocket::OSApi::timestamp_ns();
    }
    end_timestamp = zrsocket::OSApi::timestamp();
    printf("end process_time:osapi_timestamp thread_id:%d, update_timestamp:%lld, spend_time:%lld us times:%ld\n", id, update_timestamp, end_timestamp-start_timestamp, times);

    std::chrono::time_point<std::chrono::system_clock> update_std_time;
    std::chrono::time_point<std::chrono::steady_clock> update_std_timestamp;

    auto start_std_timestamp = std::chrono::steady_clock::now();
    printf("start process_time:std_time thread_id:%d, timestamp:%lld\n", id, start_std_timestamp.time_since_epoch().count());
    start_std_timestamp = std::chrono::steady_clock::now();
    for (int i = 0; i < times; ++i) {
        update_std_time = std::chrono::system_clock::now();
    }
    auto end_std_timestamp = std::chrono::steady_clock::now();
    auto diff = end_std_timestamp - start_std_timestamp;
    printf("end process_time:std_time thread_id:%d, update_std_time:%lld, spend_time:%lld us times:%d\n", id, update_std_time.time_since_epoch().count(), diff.count()/1000, times);

    start_std_timestamp = std::chrono::steady_clock::now();
    printf("start process_time:std_timestamp thread_id:%d, timestamp:%lld\n", id, start_std_timestamp.time_since_epoch().count());
    start_std_timestamp = std::chrono::steady_clock::now();
    for (int i = 0; i < times; ++i) {
        update_std_timestamp = std::chrono::steady_clock::now();
    }
    end_std_timestamp = std::chrono::steady_clock::now();
    diff = end_std_timestamp - start_std_timestamp;
    printf("end process_time:std_timestamp thread_id:%d, update_std_timestamp:%lld, spend_time:%lld us times:%d\n", id, update_std_timestamp.time_since_epoch().count(), diff.count()/1000, times);

    return 0;
}

#endif

#if TEST_MUTEX

int sum_ = 0;
int test_sum(int id, int times) 
{
    while (!ready_.load()) {
        std::this_thread::yield();
    }

    auto start_timestamp = zrsocket::OSApi::timestamp();
    printf("start sum thread_id:%d, timestamp:%lld\n", id, start_timestamp);
    start_timestamp = zrsocket::OSApi::timestamp();
    for (int i=0; i<times; ++i) {
        ++sum_;
    }
    auto end_timesamp = zrsocket::OSApi::timestamp();
    printf("end sum thread_id:%d, sum:%ld, spend_time:%lld us\n", id, sum_, end_timesamp-start_timestamp);

    return 0;
}

zrsocket::AtomicInt atomic_int_sum_  = ATOMIC_VAR_INIT(0);
zrsocket::AtomicInt atomic_int_sum2_ = ATOMIC_VAR_INIT(0);
int test_atomic_int_sum(int id, int times) 
{
    while (!ready_.load()) {
        std::this_thread::yield();
    }

    auto start_timestamp = zrsocket::OSApi::timestamp();
    printf("start atomic_int_sum thread_id:%d, timestamp:%lld\n", id, start_timestamp);
    start_timestamp = zrsocket::OSApi::timestamp();
    for (int i = 0; i < times; ++i) {
        atomic_int_sum_.fetch_add(1);
    }
    auto end_timestamp = zrsocket::OSApi::timestamp();
    printf("end atomic_int_sum thread_id:%d, sum:%ld, time:%lld us\n", id, atomic_int_sum_.load(), end_timestamp-start_timestamp);

    start_timestamp = zrsocket::OSApi::timestamp();
    printf("start atomic_int_sum(memory_order_relaxed) thread_id:%d, timestamp:%lld\n", id, start_timestamp);
    start_timestamp = zrsocket::OSApi::timestamp();
    for (int i = 0; i < times; ++i) {
        atomic_int_sum2_.fetch_add(1, std::memory_order_relaxed);
    }
    end_timestamp = zrsocket::OSApi::timestamp();
    printf("end atomic_int_sum(memory_order_relaxed) thread_id:%d, sum:%ld, spend_time:%lld us\n",
        id, atomic_int_sum2_.load(std::memory_order_relaxed), end_timestamp-start_timestamp);

    return 0;
}

zrsocket::NullMutex null_mutex_;
int null_mutex_sum_ = 0;
int test_null_mutex_sum(int id, int times) 
{
    while (!ready_.load()) {
        std::this_thread::yield();
    }

    auto start_timestamp = zrsocket::OSApi::timestamp();
    printf("start null_mutex_sum thread_id:%d, timestamp:%lld\n", id, start_timestamp);
    start_timestamp = zrsocket::OSApi::timestamp();
    for (int i = 0; i < times; ++i) {
        null_mutex_.lock();
        ++null_mutex_sum_;
        null_mutex_.unlock();
    }
    auto end_timestamp = zrsocket::OSApi::timestamp();
    printf("end null_mutex_sum thread_id:%d, sum:%ld, spend_time:%lld us\n", id, null_mutex_sum_, end_timestamp -start_timestamp);

    return 0;
}

zrsocket::ThreadMutex thread_mutex_;
int thread_mutex_sum_ = 0;
int test_tread_mutex_sum(int id, int times) 
{
    while (!ready_.load()) {
        std::this_thread::yield();
    }

    auto start_timestamp = zrsocket::OSApi::timestamp();
    printf("start thread_mutex_sum thread_id:%d, timestamp:%lld\n", id, start_timestamp);
    start_timestamp = zrsocket::OSApi::timestamp();
    for (int i = 0; i < times; ++i) {
        thread_mutex_.lock();
        ++thread_mutex_sum_;
        thread_mutex_.unlock();
    }
    auto end_timestamp = zrsocket::OSApi::timestamp();
    printf("end thread_mutex_sum thread_id:%d, sum:%ld, spend_time:%lld us\n", id, thread_mutex_sum_, end_timestamp-start_timestamp);

    return 0;
}

zrsocket::AtomicFlagSpinlockMutex atomic_flag_spinlock_mutex_;
int atomic_flag_spinlock_mutex_sum_ = 0;
int test_atomic_flag_spinlock_mutex_sum(int id, int times) 
{
    while (!ready_.load()) {
        std::this_thread::yield();
    }

    auto start_timestamp = zrsocket::OSApi::timestamp();
    printf("start atomic_flag_spinlock_mutex_sum thread_id:%d, timestamp:%lld\n", id, start_timestamp);
    start_timestamp = zrsocket::OSApi::timestamp();
    for (int i = 0; i < times; ++i) {
        atomic_flag_spinlock_mutex_.lock();
        ++atomic_flag_spinlock_mutex_sum_;
        atomic_flag_spinlock_mutex_.unlock();
    }
    auto end_timestamp = zrsocket::OSApi::timestamp();
    printf("end atomic_flag_spinlock_mutex_sum thread_id:%d, sum:%ld, spend_time:%lld us\n", id, atomic_flag_spinlock_mutex_sum_, end_timestamp-start_timestamp);

    return 0;
}

zrsocket::AtomicBoolSpinlockMutex atomic_bool_spinlock_mutex_;
int atomic_bool_spinlock_mutex_sum_ = 0;
int test_atomic_bool_spinlock_mutex_sum(int id, int times) 
{
    while (!ready_.load()) {
        std::this_thread::yield();
    }

    auto start_timestamp = zrsocket::OSApi::timestamp();
    printf("start atomic_bool_spinlock_mutex_sum thread_id:%d, timestamp:%lld\n", id, start_timestamp);
    start_timestamp = zrsocket::OSApi::timestamp();
    for (int i = 0; i < times; ++i) {
        atomic_bool_spinlock_mutex_.lock();
        ++atomic_bool_spinlock_mutex_sum_;
        atomic_bool_spinlock_mutex_.unlock();
    }
    auto end_timestamp = zrsocket::OSApi::timestamp();
    printf("end atomic_bool_spinlock_mutex_sum thread_id:%d, sum:%ld, spend_time:%lld us\n", id, atomic_bool_spinlock_mutex_sum_, end_timestamp-start_timestamp);

    return 0;
}

zrsocket::AtomicIntSpinlockMutex atomic_int_spinlock_mutex_;
int atomic_int_spinlock_mutex_sum_ = 0;
int test_atomic_int_spinlock_mutex_sum(int id, int times) 
{
    while (!ready_.load()) {
        std::this_thread::yield();
    }

    auto start_timestamp = zrsocket::OSApi::timestamp();
    printf("start atomic_int_spinlock_mutex_sum thread_id:%d, timestamp:%lld\n", id, start_timestamp);
    start_timestamp = zrsocket::OSApi::timestamp();
    for (int i = 0; i < times; ++i) {
        atomic_int_spinlock_mutex_.lock();
        ++atomic_int_spinlock_mutex_sum_;
        atomic_int_spinlock_mutex_.unlock();
    }
    auto end_timestamp = zrsocket::OSApi::timestamp();
    printf("end atomic_int_spinlock_mutex_sum thread_id:%d, sum:%ld, spend_time:%lld us\n", id, atomic_int_spinlock_mutex_sum_, end_timestamp-start_timestamp);

    return 0;
}

#ifndef ZRSOCKET_OS_WINDOWS
zrsocket::PthreadSpinlockMutex pthread_spinlock_mutex_;
int pthread_spinlock_mutex_sum_ = 0;
int test_pthread_spinlock_mutex_sum(int id, int times) 
{
    while (!ready_.load()) {
        std::this_thread::yield();
    }

    auto start_timestamp = zrsocket::OSApi::timestamp();
    printf("start pthread_spinlock_mutex_sum thread_id:%d, timestamp:%lld\n", id, start_timestamp);
    start_timestamp = zrsocket::OSApi::timestamp();
    for (int i = 0; i < times; ++i) {
        pthread_spinlock_mutex_.lock();
        ++pthread_spinlock_mutex_sum_;
        pthread_spinlock_mutex_.unlock();
    }
    auto end_timestamp = zrsocket::OSApi::timestamp();
    printf("end pthread_spinlock_mutex_sum thread_id:%d, sum:%ld, spend_time:%lld us\n", id, pthread_spinlock_mutex_sum_, end_timestamp-start_timestamp);

    return 0;
}
#endif //!ZRSOCKET_OS_WINDOWS

#endif //TEST_MUTEX

template <typename TTest>
int startup_test(TTest test, int thread_num, int max_times)
{
    ready_.store(false);

    std::vector<std::thread *> threads;
    threads.reserve(thread_num);
    for (auto i = 0; i < thread_num; ++i) {
        threads.emplace_back(new std::thread(test, i, max_times));
    }

    ready_.store(true);
    for (auto &t : threads) {
        t->join();
        delete t;
    }
    threads.clear();

    return 0;
}

int main(int argc, char *argv[])
{
    printf("please use format: <thread number> <number_times>\n");
    int thread_num = 2;
    int times = 10000000;
    if (argc > 2) {
        thread_num = atoi(argv[1]);
        times = atoi(argv[2]);
    } else if (argc > 1) {
        thread_num = atoi(argv[1]);
    }

    printf("thread number:%d, number_times:%ld\n", thread_num, times);

#if TEST_GET_TIME
    startup_test(test_time, thread_num, times);
#endif // TEST_GET_TIME

 #if TEST_MUTEX
    startup_test(test_sum, thread_num, times);
    startup_test(test_atomic_int_sum, thread_num, times);
    startup_test(test_null_mutex_sum, thread_num, times);
    startup_test(test_tread_mutex_sum, thread_num, times);
    startup_test(test_atomic_bool_spinlock_mutex_sum, thread_num, times);
    startup_test(test_atomic_int_spinlock_mutex_sum, thread_num, times);
    startup_test(test_atomic_flag_spinlock_mutex_sum, thread_num, times);

#ifndef ZRSOCKET_OS_WINDOWS
    startup_test(test_pthread_spinlock_mutex_sum, thread_num, times);
#endif

#endif  //TEST_MUTEX

    return 0;
}
