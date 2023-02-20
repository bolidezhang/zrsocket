#include <atomic>
#include <chrono>
#include <cstddef>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <new>
#include <thread>
#include <vector>
#include <chrono>
#include <sstream>
#include "test_seda.h"

int test_stage(int id, zrsocket::ISedaStage *stage, const char *out_title)
{
    TestApp &app = TestApp::instance();
    while (!app.ready_.load()) {
        std::this_thread::yield();
    }

#if 0
    //测试thread_local/__thread
    {
        const int TIMES = 100000000;
        zrsocket::SteadyClockCounter scc;

        volatile uint64_t v_sum = 0;
        scc.update_start_counter();
        for (int i = 0; i < TIMES; ++i) {
            v_sum += i;
        }
        scc.update_end_counter();
        printf("%s thread_id:%d volatile sum:%lld times:%ld diff %lld ns\n", out_title, id, v_sum, TIMES, scc.diff());

        static zrsocket_fast_thread_local uint64_t sum_ = 0;
        scc.update_start_counter();
        for (int i = 0; i < TIMES; ++i) {
            sum_ += i;
        }
        scc.update_end_counter();
        printf("%s thread_id:%d zrsocket_fast_thread_local sum:%lld times:%ld diff %lld ns\n", out_title, id, sum_, TIMES, scc.diff());

        static thread_local uint64_t tl_sum_ = 0;
        scc.update_start_counter();
        for (int i = 0; i < TIMES; ++i) {
            tl_sum_ += i;
        }
        scc.update_end_counter();
        printf("%s thread_id:%d thread_local sum:%lld times:%ld diff %lld ns\n", out_title, id, tl_sum_, TIMES, scc.diff());

        static thread_local zrsocket::ByteBuffer tl_buf_;
        scc.update_start_counter();
        tl_buf_.reserve(50);
        for (int i = 0; i < TIMES; ++i) {
            tl_buf_.reset();
            tl_buf_.write(i);
        }
        scc.update_end_counter();
        printf("%s thread_id:%d thread_local tl_buf_size:%d times:%ld diff %lld ns\n", out_title, id, tl_buf_.data_size(), TIMES, scc.diff());


        static thread_local zrsocket::ByteBuffer tl_buf2_;
        zrsocket::ByteBuffer &buf2 = tl_buf2_;
        scc.update_start_counter();
        buf2.reserve(50);
        for (int i = 0; i < TIMES; ++i) {
            buf2.reset();
            buf2.write(i);
        }
        scc.update_end_counter();
        printf("%s thread_id:%d thread_local tl_buf2_size:%d times:%ld diff %lld ns\n", out_title, id, buf2.data_size(), TIMES, scc.diff());

        static zrsocket_fast_thread_local zrsocket::ByteBuffer ftl_buf_;
        scc.update_start_counter();
        ftl_buf_.reserve(50);
        for (int i = 0; i < TIMES; ++i) {
            ftl_buf_.reset();
            ftl_buf_.write(i);
        }
        scc.update_end_counter();
        printf("%s thread_id:%d fast_thread_local ftl_buf_size:%d times:%ld diff %lld ns\n", out_title, id, ftl_buf_.data_size(), TIMES, scc.diff());

        zrsocket::ByteBuffer buf;
        scc.update_start_counter();
        buf.reserve(50);
        for (int i = 0; i < TIMES; ++i) {
            buf.reset();
            buf.write(i);
        }
        scc.update_end_counter();
        printf("%s thread_id:%d buf_size:%d times:%ld diff %lld ns\n", out_title, id, buf.data_size(), TIMES, scc.diff());

        return 1;
    }
#endif

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
                    ZRSOCKET_LOG_DEBUG(out_title << " thread_id:" << id << " push num:" << push_num <<" success");
                }
                else {
                    ZRSOCKET_LOG_DEBUG(out_title << " thread_id:" << id << " push num:" << push_num <<" failure");
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
    //printf("%s thread_id:%d push end num:%d spend_time:%lld ns\n", out_title, id, push_num, scc.diff());
    ZRSOCKET_LOG_INFO(out_title << " thread_id:" << id << " push end num:" << push_num << " spend_time:" << scc.diff() << " ns");
    app.push_num_.fetch_add(push_num, std::memory_order_relaxed);

    return 0;
}

template <typename TTestFunc>
int startup_test(TTestFunc test_func, bool thread_num_flag, int thread_num, zrsocket::ISedaStage *stage, const char *out_title)
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
        threads.emplace_back(new std::thread(test_func, i, stage, out_title));
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

int send_to_remote(void *context, const char *log, zrsocket::uint_t len)
{
    return 0;
}

#if 0

#ifdef __cpp_lib_hardware_interference_size
using std::hardware_constructive_interference_size;
using std::hardware_destructive_interference_size;
#else
// 64 bytes on x86-64 │ L1_CACHE_BYTES │ L1_CACHE_SHIFT │ __cacheline_aligned │ ...
constexpr std::size_t hardware_constructive_interference_size = 64;
constexpr std::size_t hardware_destructive_interference_size = 64;
#endif

std::mutex cout_mutex;

constexpr int max_write_iterations{ 10'000'000 }; // the benchmark time tuning

struct alignas(hardware_constructive_interference_size)
    OneCacheLiner { // occupies one cache line
    std::atomic_uint64_t x{};
    std::atomic_uint64_t y{};
} oneCacheLiner;

struct TwoCacheLiner { // occupies two cache lines
    alignas(hardware_destructive_interference_size) std::atomic_uint64_t x{};
    alignas(hardware_destructive_interference_size) std::atomic_uint64_t y{};
} twoCacheLiner;

inline auto now() noexcept { return std::chrono::high_resolution_clock::now(); }

template<bool xy>
void oneCacheLinerThread() {
    const auto start{ now() };

    for (uint64_t count{}; count != max_write_iterations; ++count)
        if constexpr (xy)
            oneCacheLiner.x.fetch_add(1, std::memory_order_relaxed);
        else oneCacheLiner.y.fetch_add(1, std::memory_order_relaxed);

    const std::chrono::duration<double, std::milli> elapsed{ now() - start };
    std::lock_guard lk{ cout_mutex };
    std::cout << "oneCacheLinerThread() spent " << elapsed.count() << " ms\n";
    if constexpr (xy)
        oneCacheLiner.x = elapsed.count();
    else oneCacheLiner.y = elapsed.count();
}

template<bool xy>
void twoCacheLinerThread() {
    const auto start{ now() };

    for (uint64_t count{}; count != max_write_iterations; ++count)
        if constexpr (xy)
            twoCacheLiner.x.fetch_add(1, std::memory_order_relaxed);
        else twoCacheLiner.y.fetch_add(1, std::memory_order_relaxed);

    const std::chrono::duration<double, std::milli> elapsed{ now() - start };
    std::lock_guard lk{ cout_mutex };
    std::cout << "twoCacheLinerThread() spent " << elapsed.count() << " ms\n";
    if constexpr (xy)
        twoCacheLiner.x = elapsed.count();
    else twoCacheLiner.y = elapsed.count();
}

#endif // 0

int main(int argc, char* argv[])
{
#if 1
    std::cout << "this_thread::get_id():" << std::this_thread::get_id() << " osapi::this_thread_id:" << zrsocket::OSApi::this_thread_id() << std::endl;
    //return 0;
#endif

#if 0
    {
        auto i = sizeof(zrsocket::SPSCSteadyEventTypeQueue);
        assert(i == 152);
    }

    {
        std::cout << "__cpp_lib_hardware_interference_size "
#   ifdef __cpp_lib_hardware_interference_size
            " = " << __cpp_lib_hardware_interference_size << '\n';
#   else
            "is not defined, use " << hardware_destructive_interference_size << " as fallback\n";
#   endif

        std::cout
            << "hardware_destructive_interference_size == "
            << hardware_destructive_interference_size << '\n'
            << "hardware_constructive_interference_size == "
            << hardware_constructive_interference_size << "\n\n";

        std::cout
            << std::fixed << std::setprecision(2)
            << "sizeof( OneCacheLiner ) == " << sizeof(OneCacheLiner) << '\n'
            << "sizeof( TwoCacheLiner ) == " << sizeof(TwoCacheLiner) << "\n\n";

        constexpr int max_runs{ 4 };

        int oneCacheLiner_average{ 0 };
        for (auto i{ 0 }; i != max_runs; ++i) {
            std::thread th1{ oneCacheLinerThread<0> };
            std::thread th2{ oneCacheLinerThread<1> };
            th1.join(); 
            th2.join();
            oneCacheLiner_average += oneCacheLiner.x + oneCacheLiner.y;
        }
        std::cout << "Average T1 time: " << (oneCacheLiner_average / max_runs / 2) << " ms\n\n";

        int twoCacheLiner_average{ 0 };
        for (auto i{ 0 }; i != max_runs; ++i) {
            std::thread th1{ twoCacheLinerThread<0> };
            std::thread th2{ twoCacheLinerThread<1> };
            th1.join(); 
            th2.join();
            twoCacheLiner_average += twoCacheLiner.x + twoCacheLiner.y;
        }
        std::cout << "Average T2 time: " << (twoCacheLiner_average / max_runs / 2) << " ms\n\n";
        std::cout << "Ratio T1/T2:~ " << 1. * oneCacheLiner_average / twoCacheLiner_average << '\n';

        return 1;
    }
#endif

    printf("please use the format: <thread_num> <num_times> <seda_thread_num> <seda_queue_size> <seda_event_len>\n\
<logLevel 0:Trace 1:Debug 2:Info 3Warn 4:Error 5Fatal> <logAppenderType 0:Console 1:File 2:null 3:callback> <logWorkMode 0:Sync 1:Async> <logLockType 0:None 1:Spinlock 2:Mutex> <bufferSize:1024*1024*16>\n");

    if (argc < 2) {
        return 0;
    }

    //zrsocket::Logger logger2;

    TestApp &app = TestApp::instance();
    if (argc > 6) {
        auto level = static_cast<zrsocket::LogLevel>(std::atoi(argv[6]));
        ZRSOCKET_LOG_SET_LOG_LEVEL(level);
        //ZRSOCKET_LOG_SET_LOG_LEVEL2(logger2, level);
    }
    if (argc > 7) {
        auto type = static_cast<zrsocket::LogAppenderType>(std::atoi(argv[7]));
        ZRSOCKET_LOG_SET_APPENDER_TYPE(type);
        //ZRSOCKET_LOG_SET_APPENDER_TYPE2(logger2, type);
        if (type == zrsocket::LogAppenderType::kFILE) {
            ZRSOCKET_LOG_SET_FILE_NAME("./test_seda.log");
            //ZRSOCKET_LOG_SET_FILE_NAME2(logger2, "./test_seda.log");
        }
        if (type == zrsocket::LogAppenderType::kCALLBACK) {
            ZRSOCKET_LOG_SET_CALLBACK_FUNC(send_to_remote, nullptr);
            //ZRSOCKET_LOG_SET_CALLBACK_FUNC2(logger2, send_to_remote, nullptr);
        }
    }
    if (argc > 8) {
        auto mode = static_cast<zrsocket::LogWorkMode>(std::atoi(argv[8]));
        ZRSOCKET_LOG_SET_WORK_MODE(mode);
        //ZRSOCKET_LOG_SET_WORK_MODE2(logger2, mode);
    }
    if (argc > 9) {
        auto type = static_cast<zrsocket::LogLockType>(std::atoi(argv[9]));
        ZRSOCKET_LOG_SET_LOCK_TYPE(type);
        //ZRSOCKET_LOG_SET_LOCK_TYPE2(logger2, type);
    }
    if (argc > 10) {
        auto size = std::atoi(argv[10]);
        ZRSOCKET_LOG_SET_BUFFER_SIZE(size);
        //ZRSOCKET_LOG_SET_BUFFER_SIZE2(logger2, size);
    }
    ZRSOCKET_LOG_INIT;

    //ZRSOCKET_LOG_SET_FORMAT_TYPE2(logger2, zrsocket::LogFormatType::kBINARY);
    //ZRSOCKET_LOG_INIT2(logger2);
    
#if 0
    //测试log性能
    {
        //const int LOG_TIMES = 1000000;
        static const int LOG_TIMES = 1000000;
        zrsocket::SteadyClockCounter scc;

        scc.update_start_counter();
        for (int i = 0; i < LOG_TIMES; ++i) {
            //ZRSOCKET_LOG_DEBUG(i << "-" << i + 1);
            //ZRSOCKET_LOG_INFO(i);
            //ZRSOCKET_LOG_INFO("start...["<<i<<"-"<<i+1<<"]01234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789");
            ZRSOCKET_LOG_INFO("start...[" << i << "]0123456789");
            //ZRSOCKET_LOG_INFO("start...01234567890");
        }
        scc.update_end_counter();
        printf("text log_times:%ld diff %lld ns\n", LOG_TIMES, scc.diff());

        //scc.update_start_counter();
        //for (int i = 0; i < LOG_TIMES; ++i) {
        //    ZRSOCKET_LOG_INFO2(logger2, i << "-" << i + 1);
        //    //ZRSOCKET_LOG_INFO2(logger2, i);
        //    //ZRSOCKET_LOG_INFO2(logger2, "start...["<<i<<"] 01234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789");
        //    //ZRSOCKET_LOG_INFO2(logger2, "start...[" << i << "] 0123456789");
        //    //ZRSOCKET_LOG_INFO2(logger2, "start...01234567890");
        //}
        //scc.update_end_counter();
        //printf("binary log_times:%ld diff %lld ns\n", LOG_TIMES, scc.diff());
        
        zrsocket::OSApi::sleep_s(5);
        return 0;
    }

#endif
  
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
    mpsc_stage_.open(seda_thread_num_, seda_queue_size_, seda_event_len_);
    //mpmc_stage_.open(seda_thread_num_, seda_queue_size_, seda_event_len_);
    //doublebuffer_spinlock_stage_.open(seda_thread_num_, seda_queue_size_, seda_event_len_);
    //doublebuffer_mutex_stage_.open(seda_thread_num_, seda_queue_size_, seda_event_len_);
    //spsc_volatile_stage_.open(seda_thread_num_, seda_queue_size_, seda_event_len_);
    //spsc_atomic_stage_.open(seda_thread_num_, seda_queue_size_, seda_event_len_);
    //spsc_normal_stage_.open(seda_thread_num_, seda_queue_size_, seda_event_len_);

    startup_test(test_stage, true,  thread_num_, &mpsc_stage_, "mpsc_stage");
    //startup_test(test_stage, true, thread_num_, &mpmc_stage_, "mpmc_stage");
    //startup_test(test_stage, true,  thread_num_, &doublebuffer_spinlock_stage_, "doublebuffer_spinlock_stage");
    //startup_test(test_stage, true,  thread_num_, &doublebuffer_mutex_stage_, "doublebuffer_mutex_stage");
    //startup_test(test_stage, false, thread_num_, &spsc_volatile_stage_, "spsc_volatile_stage");
    //startup_test(test_stage, false, thread_num_, &spsc_atomic_stage_, "spsc_atomic_stage");
    //startup_test(test_stage, false, thread_num_, &spsc_normal_stage_, "spsc_normal_stage");

    return 0;
}

