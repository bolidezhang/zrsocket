#include <stdio.h>
#include <vector>
#include <chrono>
#include "echo_server.h"

int TcpHandler::do_message(const char *message, uint_t len)
{
    TestAppServer::instance().recv_messge_count_.fetch_add(1, std::memory_order_relaxed);
    Message *msg = (Message *)message;
    msg->head.type = MessageType::MT_ECHO_RES;
    send(message, len, false);
    return 0;
}

int TestTimer::handle_timeout()
{
    auto now = Time::instance().current_timestamp_ms();

    printf("timer id:%llu, current_timestamp:%llu, recv_messge_count:%llu\n", 
        data_.id, 
        now, 
        TestAppServer::instance().recv_messge_count_.load(std::memory_order_relaxed));

    ////
    //TestAppServer &app = TestAppServer::instance();
    //zrsocket::InetAddr remote_addr;
    //remote_addr.set("192.168.11.150", 6000);
    //app.udp_source_.send("0123456789", 10, remote_addr);

    return 0;
}

int TestAppServer::init()
{
    decoder_config_.length_field_offset_ = 0;
    decoder_config_.length_field_length_ = 2;
    decoder_config_.max_message_length_ = 16384;
    //decoder_config_.max_message_length_ = 4096;
    decoder_config_.update();
    main_event_loop_.buffer_size();
    main_event_loop_.open(1024, 1024, -1, 0);
    recv_messge_count_.store(0, std::memory_order_relaxed);
    sub_event_loop_.init(std::thread::hardware_concurrency(), 10000, 1, 1000, 64);
    sub_event_loop_.open(1024, 1024, -1);
    sub_event_loop_.loop_thread_start(-1);

    handler_object_pool_.init(10000, 100, 10);
    tcp_server_.set_config(std::thread::hardware_concurrency(), 1000000, 4096);
    tcp_server_.set_interface(&handler_object_pool_, &sub_event_loop_, &decoder_config_, &main_event_loop_);
    //tcp_server_.set_interface(&handler_object_pool_, &main_event_loop_, &decoder_config_, &main_event_loop_);
    tcp_server_.open(port_, 1024, nullptr);

    udp_source_.set_config();
    udp_source_.set_interface(&main_event_loop_);
    udp_source_.open(port_);
    //InetAddr dest_addr;
    //udp_source_.send("1", 1, dest_addr);

    timer1_.interval(1000000);
    timer1_.data_.id = 1;
    //main_event_loop_.add_timer(&timer1_);

    /*
    timer2_.interval(100000);
    timer2_.data_.id = 2;
    main_event_loop_.add_timer(&timer2_);

    timer3_.interval(10000);
    timer3_.data_.id = 3;
    main_event_loop_.add_timer(&timer3_);

    timer4_.interval(20000);
    timer4_.data_.id = 4;
    main_event_loop_.add_timer(&timer4_);
    */

    //stage_.open(1, 100000, 64);
    //stage2_.open(1, 100000, 64);
    init_flag_ = true;

    return 0;
}

#define TEST 0
#if TEST
#define TEST_MUTEX 1
#define TEST_GET_TIME 1
zrsocket::AtomicBool ready = ATOMIC_VAR_INIT(false);
#endif // TEST

#if TEST_GET_TIME

int process_time(int id, int times)
{
    while (!ready.load()) {
        std::this_thread::yield();
    }

    int64_t start_timestamp = 0;
    int64_t end_timestamp = 0;

    int64_t update_time = 0;
    start_timestamp = zrsocket::OSApi::timestamp();
    printf("start process_time:osapi_time id:%d, timestamp:%lld\n", id, start_timestamp);
    start_timestamp = zrsocket::OSApi::timestamp();
    for (int i = 0; i < times; ++i) {
        update_time = zrsocket::OSApi::time_ns();
    }
    end_timestamp = zrsocket::OSApi::timestamp();
    printf("end process_time:osapi_time id:%d, update_time:%lld, time:%lld us times:%d\n", id, update_time, end_timestamp-start_timestamp, times);

    int64_t update_timestamp = 0;
    start_timestamp = zrsocket::OSApi::timestamp();
    printf("start process_time:osapi_timestamp id:%d, timestamp:%lld\n", id, start_timestamp);
    start_timestamp = zrsocket::OSApi::timestamp();
    for (int i = 0; i < times; ++i) {
        update_timestamp = zrsocket::OSApi::timestamp_ns();
    }
    end_timestamp = zrsocket::OSApi::timestamp();
    printf("end process_time:osapi_timestamp id:%d, update_timestamp:%lld, time:%lld us times:%ld\n", id, update_timestamp, end_timestamp-start_timestamp, times);


    std::chrono::time_point<std::chrono::system_clock> update_std_time;
    std::chrono::time_point<std::chrono::steady_clock> update_std_timestamp;

    auto start_std_timestamp = std::chrono::steady_clock::now();
    printf("start process_time:std_time id:%d, timestamp:%lld\n", id, start_std_timestamp.time_since_epoch().count());
    start_std_timestamp = std::chrono::steady_clock::now();
    for (int i = 0; i < times; ++i) {
        update_std_time = std::chrono::system_clock::now();
    }
    auto end_std_timestamp = std::chrono::steady_clock::now();
    auto diff = end_std_timestamp - start_std_timestamp;
    printf("end process_time:std_time id:%d, update_std_time:%lld, time:%lld us times:%d\n", id, update_std_time.time_since_epoch().count(), diff.count()/1000, times);


    start_std_timestamp = std::chrono::steady_clock::now();
    printf("start process_time:std_timestamp id:%d, timestamp:%lld\n", id, start_std_timestamp.time_since_epoch().count());
    start_std_timestamp = std::chrono::steady_clock::now();
    for (int i = 0; i < times; ++i) {
        update_std_timestamp = std::chrono::steady_clock::now();
    }
    end_std_timestamp = std::chrono::steady_clock::now();
    diff = end_std_timestamp - start_std_timestamp;
    printf("end process_time:std_timestamp id:%d, update_std_timestamp:%lld, time:%lld us times:%d\n", id, update_std_timestamp.time_since_epoch().count(), diff.count()/1000, times);

    return 0;
}

#endif

#if TEST_MUTEX

int sum = 0;
int process_sum(int id, int times) {
    while (!ready.load()) {
        std::this_thread::yield();
    }

    auto start_timestamp = zrsocket::OSApi::timestamp();
    printf("start sum id:%d, timestamp:%lld\n", id, start_timestamp);
    start_timestamp = zrsocket::OSApi::timestamp();
    for (int i=0; i<times; ++i) {
        ++sum;
    }
    auto end_timesamp = zrsocket::OSApi::timestamp();
    printf("end sum id:%d, sum:%ld, time:%lld us\n", id, sum, end_timesamp-start_timestamp);

    return 0;
}

zrsocket::AtomicInt atomic_int_sum  = ATOMIC_VAR_INIT(0);
zrsocket::AtomicInt atomic_int_sum2 = ATOMIC_VAR_INIT(0);
int process_atomic_int_sum(int id, int times) {
    while (!ready.load()) {
        std::this_thread::yield();
    }

    auto start_timestamp = zrsocket::OSApi::timestamp();
    printf("start atomic_int_sum id:%d, timestamp:%lld\n", id, start_timestamp);
    start_timestamp = zrsocket::OSApi::timestamp();
    for (int i = 0; i < times; ++i) {
        atomic_int_sum2.fetch_add(1);
    }
    auto end_timestamp = zrsocket::OSApi::timestamp();
    printf("end atomic_int_sum id:%d, sum:%ld, time:%lld us\n", id, atomic_int_sum2.load(), end_timestamp-start_timestamp);

    start_timestamp = zrsocket::OSApi::timestamp();
    printf("start atomic_int_sum(memory_order_relaxed) id:%d, timestamp:%lld\n", id, start_timestamp);
    start_timestamp = zrsocket::OSApi::timestamp();
    for (int i = 0; i < times; ++i) {
        atomic_int_sum.fetch_add(1, std::memory_order_relaxed);
    }
    end_timestamp = zrsocket::OSApi::timestamp();
    printf("end atomic_int_sum(memory_order_relaxed) id:%d, sum:%ld, time:%lld us\n",
        id, atomic_int_sum.load(std::memory_order_relaxed), end_timestamp -start_timestamp);

    return 0;
}

zrsocket::NullMutex null_mutex;
int null_mutex_sum = 0;
int process_null_mutex_sum(int id, int times) {
    while (!ready.load()) {
        std::this_thread::yield();
    }

    auto start_timestamp = zrsocket::OSApi::timestamp();
    printf("start null_mutex_sum id:%d, timestamp:%lld\n", id, start_timestamp);
    start_timestamp = zrsocket::OSApi::timestamp();
    for (int i = 0; i < times; ++i) {
        null_mutex.lock();
        ++null_mutex_sum;
        null_mutex.unlock();
    }
    auto end_timestamp = zrsocket::OSApi::timestamp();
    printf("end null_mutex_sum id:%d, sum:%ld, time:%lld us\n", id, null_mutex_sum, end_timestamp -start_timestamp);

    return 0;
}

zrsocket::ThreadMutex thread_mutex;
int thread_mutex_sum = 0;
int process_tread_mutex_sum(int id, int times) {
    while (!ready.load()) {
        std::this_thread::yield();
    }

    auto start_timestamp = zrsocket::OSApi::timestamp();
    printf("start thread_mutex_sum id:%d, timestamp:%lld\n", id, start_timestamp);
    start_timestamp = zrsocket::OSApi::timestamp();
    for (int i = 0; i < times; ++i) {
        thread_mutex.lock();
        ++thread_mutex_sum;
        thread_mutex.unlock();
    }
    auto end_timestamp = zrsocket::OSApi::timestamp();
    printf("end thread_mutex_sum id:%d, sum:%ld, time:%lld us\n", id, thread_mutex_sum, end_timestamp-start_timestamp);

    return 0;
}

AtomicFlagSpinlockMutex atomic_flag_spinlock_mutex;
int atomic_flag_spinlock_mutex_sum = 0;
int process_atomic_flag_spinlock_mutex_sum(int id, int times) {
    while (!ready.load()) {
        std::this_thread::yield();
    }

    auto start_timestamp = zrsocket::OSApi::timestamp();
    printf("start atomic_flag_spinlock_mutex_sum id:%d, timestamp:%lld\n", id, start_timestamp);
    start_timestamp = zrsocket::OSApi::timestamp();
    for (int i = 0; i < times; ++i) {
        atomic_flag_spinlock_mutex.lock();
        ++atomic_flag_spinlock_mutex_sum;
        atomic_flag_spinlock_mutex.unlock();
    }
    auto end_timestamp = zrsocket::OSApi::timestamp();
    printf("end atomic_flag_spinlock_mutex_sum id:%d, sum:%ld, time:%lld us\n", id, atomic_flag_spinlock_mutex_sum, end_timestamp-start_timestamp);

    return 0;
}

AtomicBoolSpinlockMutex atomic_bool_spinlock_mutex;
int atomic_bool_spinlock_mutex_sum = 0;
int process_atomic_bool_spinlock_mutex_sum(int id, int times) {
    while (!ready.load()) {
        std::this_thread::yield();
    }

    auto start_timestamp = zrsocket::OSApi::timestamp();
    printf("start atomic_bool_spinlock_mutex_sum id:%d, timestamp:%lld\n", id, start_timestamp);
    start_timestamp = zrsocket::OSApi::timestamp();
    for (int i = 0; i < times; ++i) {
        atomic_bool_spinlock_mutex.lock();
        ++atomic_bool_spinlock_mutex_sum;
        atomic_bool_spinlock_mutex.unlock();
    }
    auto end_timestamp = zrsocket::OSApi::timestamp();
    printf("end atomic_bool_spinlock_mutex_sum id:%d, sum:%ld, time:%lld us\n", id, atomic_bool_spinlock_mutex_sum, end_timestamp-start_timestamp);

    return 0;
}

AtomicIntSpinlockMutex atomic_int_spinlock_mutex;
int atomic_int_spinlock_mutex_sum = 0;
int process_atomic_int_spinlock_mutex_sum(int id, int times) {
    while (!ready.load()) {
        std::this_thread::yield();
    }

    auto start_timestamp = zrsocket::OSApi::timestamp();
    printf("start atomic_int_spinlock_mutex_sum id:%d, timestamp:%lld\n", id, start_timestamp);
    start_timestamp = zrsocket::OSApi::timestamp();
    for (int i = 0; i < times; ++i) {
        atomic_int_spinlock_mutex.lock();
        ++atomic_int_spinlock_mutex_sum;
        atomic_int_spinlock_mutex.unlock();
    }
    auto end_timestamp = zrsocket::OSApi::timestamp();
    printf("end atomic_int_spinlock_mutex_sum id:%d, sum:%ld, time:%lld us\n", id, atomic_int_spinlock_mutex_sum, end_timestamp-start_timestamp);

    return 0;
}

#ifndef ZRSOCKET_OS_WINDOWS
PthreadSpinlockMutex pthread_spinlock_mutex;
int pthread_spinlock_mutex_sum = 0;
int process_pthread_spinlock_mutex_sum(int id, int times) {
    while (!ready.load()) {
        std::this_thread::yield();
    }

    auto start_timestamp = zrsocket::OSApi::timestamp();
    printf("start pthread_spinlock_mutex_sum id:%d, timestamp:%lld\n", id, start_timestamp);
    start_timestamp = zrsocket::OSApi::timestamp();
    for (int i = 0; i < times; ++i) {
        pthread_spinlock_mutex.lock();
        ++pthread_spinlock_mutex_sum;
        pthread_spinlock_mutex.unlock();
    }
    auto end_timestamp = zrsocket::OSApi::timestamp();
    printf("end pthread_spinlock_mutex_sum id:%d, sum:%ld, time:%lld us\n", id, pthread_spinlock_mutex_sum, end_timestamp-start_timestamp);

    return 0;
}
#endif //!ZRSOCKET_OS_WINDOWS

#endif //TEST_MUTEX

#if TEST
template <typename TfnProcess>
int startup_process(TfnProcess process, int thread_num, int max_times)
{
    ready.store(false);

    std::vector<std::thread*> threads;
    threads.reserve(thread_num);
    for (auto i = 0; i < thread_num; ++i) {
        threads.emplace_back(new std::thread(process, i, max_times));
    }

    ready.store(true);
    for (auto& p : threads) {
        p->join();
        delete p;
    }
    threads.clear();

    return 0;
}
#endif

int main(int argc, char *argv[])
{
#if TEST
    {
        int thread_num = 2;
        if (argc > 1) {
            thread_num = atoi(argv[1]);
        }

        int max_times = 10000000;
        if (argc > 2) {
            max_times = atoi(argv[2]);
        }

#if TEST_GET_TIME
        startup_process(process_time, thread_num, max_times);
#endif // TEST_GET_TIME

 #if TEST_MUTEX
        startup_process(process_sum, thread_num, max_times);
        startup_process(process_atomic_int_sum, thread_num, max_times);
        startup_process(process_null_mutex_sum, thread_num, max_times);
        startup_process(process_tread_mutex_sum, thread_num, max_times);
        startup_process(process_atomic_bool_spinlock_mutex_sum, thread_num, max_times);
        startup_process(process_atomic_int_spinlock_mutex_sum, thread_num, max_times);
        startup_process(process_atomic_flag_spinlock_mutex_sum, thread_num, max_times);

#ifndef ZRSOCKET_OS_WINDOWS
        startup_process(process_pthread_spinlock_mutex_sum, thread_num, max_times);
#endif

#endif  //TEST_MUTEX
        
        int64_t sum = 0;
        int64_t start_timestamp = 0;

        start_timestamp = zrsocket::OSApi::timestamp();
        for (int i = 0; i < max_times; ++i) {
            ++sum;
        }
        printf("total sum:%lld time: %lld us\n", sum, zrsocket::OSApi::timestamp() - start_timestamp);


        sum = 0;
        zrsocket::NullMutex null_mutex;
        start_timestamp = zrsocket::OSApi::timestamp();
        for (int i = 0; i < max_times; ++i) {
            null_mutex.lock();
            ++sum;
            null_mutex.unlock();
        }
        printf("NullMutex sum:%lld time: %lld us\n", sum, zrsocket::OSApi::timestamp()-start_timestamp);

        sum = 0;
        zrsocket::AtomicFlagSpinlockMutex atomic_flag_spinlock_mutex;
        start_timestamp = zrsocket::OSApi::timestamp();
        for (int i = 0; i < max_times; ++i) {
            atomic_flag_spinlock_mutex.lock();
            ++sum;
            atomic_flag_spinlock_mutex.unlock();
        }
        printf("AtomicFlagSpinlockMutex sum:%lld time: %lld us\n", sum, zrsocket::OSApi::timestamp()-start_timestamp);


#ifndef ZRSOCKET_OS_WINDOWS
        sum = 0;
        zrsocket::PthreadSpinlockMutex pthread_spinlock_mutex;
        start_timestamp = zrsocket::OSApi::timestamp();
        for (int i = 0; i < max_times; ++i) {
            pthread_spinlock_mutex.lock();
            ++sum;
            pthread_spinlock_mutex.unlock();
        }
        printf("PthreadSpinlockMutex sum:%lld time: %lld us\n", sum, zrsocket::OSApi::timestamp()-start_timestamp);
#endif // !ZRSOCKET_OS_WINDOWS

        sum = 0;
        zrsocket::AtomicBoolSpinlockMutex atomic_bool_spinlock_mutex;
        start_timestamp = zrsocket::OSApi::timestamp();
        for (int i = 0; i < max_times; ++i) {
            atomic_bool_spinlock_mutex.lock();
            ++sum;
            atomic_bool_spinlock_mutex.unlock();
        }
        printf("AtomicBoolSpinlockMutex sum:%lld time: %lld us\n", sum, zrsocket::OSApi::timestamp()-start_timestamp);

        sum = 0;
        zrsocket::AtomicCharSpinlockMutex atomic_char_spinlock_mutex;
        start_timestamp = zrsocket::OSApi::timestamp();
        for (int i = 0; i < max_times; ++i) {
            atomic_char_spinlock_mutex.lock();
            ++sum;
            atomic_char_spinlock_mutex.unlock();
        }
        printf("AtomicCharSpinlockMutex sum:%lld time: %lld us\n", sum, zrsocket::OSApi::timestamp()-start_timestamp);

        sum = 0;
        zrsocket::AtomicIntSpinlockMutex atomic_int_spinlock_mutex;
        start_timestamp = zrsocket::OSApi::timestamp();
        for (int i = 0; i < max_times; ++i) {
            atomic_int_spinlock_mutex.lock();
            ++sum;
            atomic_int_spinlock_mutex.unlock();
        }
        printf("AtomicIntSpinlockMutex sum:%lld time: %lld us\n", sum, zrsocket::OSApi::timestamp()-start_timestamp);

        sum = 0;
        zrsocket::AtomicLongSpinlockMutex atomic_long_spinlock_mutex;
        start_timestamp = zrsocket::OSApi::timestamp();
        for (int i = 0; i < max_times; ++i) {
            atomic_long_spinlock_mutex.lock();
            ++sum;
            atomic_long_spinlock_mutex.unlock();
        }
        printf("AtomicLongSpinlockMutex sum:%lld time: %lld us\n", sum, zrsocket::OSApi::timestamp()-start_timestamp);

#ifndef ZRSOCKET_OS_WINDOWS
        sum = 0;
        zrsocket::PthreadSpinlockMutex pthread_spinlock_mutex2;
        start_timestamp = zrsocket::OSApi::timestamp();
        for (int i = 0; i < max_times; ++i) {
            pthread_spinlock_mutex2.lock();
            ++sum;
            pthread_spinlock_mutex2.unlock();
        }
        printf("PthreadSpinlockMutex2 sum:%lld time: %lld us\n", sum, zrsocket::OSApi::timestamp()-start_timestamp);
#endif // !ZRSOCKET_OS_WINDOWS

        sum = 0;
        zrsocket::ThreadMutex thread_mutex;
        start_timestamp = zrsocket::OSApi::timestamp();
        for (int i = 0; i < max_times; ++i) {
            thread_mutex.lock();
            ++sum;
            thread_mutex.unlock();
        }
        printf("ThreadMutex sum:%lld time: %lld us\n", sum, zrsocket::OSApi::timestamp()-start_timestamp);

#ifndef ZRSOCKET_OS_WINDOWS
        sum = 0;
        zrsocket::PthreadSpinlockMutex pthread_spinlock_mutex3;
        start_timestamp = zrsocket::OSApi::timestamp();
        for (int i = 0; i < max_times; ++i) {
            pthread_spinlock_mutex3.lock();
            ++sum;
            pthread_spinlock_mutex3.unlock();
        }
        printf("PthreadSpinlockMutex3 sum:%lld time: %lld us\n", sum, zrsocket::OSApi::timestamp()-start_timestamp);
#endif // !ZRSOCKET_OS_WINDOWS

        return 0;
    }
#endif

    TestAppServer &app = TestAppServer::instance();
    if (argc > 1) {
        app.port_ = atoi(argv[1]);
    }

    printf("zrsocket version:%s\n", ZRSOCKET_VERSION_STR);
    printf("please use the format: port (e.g.: 6611)\n");
    printf("server port:%d\n", app.port_);
    return app.run();
}
