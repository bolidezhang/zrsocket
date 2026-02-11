// Some compilers (e.g. VC++) benefit significantly from using this. 
// We've measured 3-4% build speed improvements in apps as a result 
#pragma once

#ifndef ZRSOCKET_TSC_CLOCK_H
#define ZRSOCKET_TSC_CLOCK_H
#include <atomic>
#include <algorithm>
#include <chrono>
#include <thread>
#include <vector>
#include "config.h"
#include "base_type.h"
#include "os_api.h"

// 平台相关的头文件引入
#ifdef _MSC_VER
#include <intrin.h>
#else
#include <x86intrin.h>
#endif

ZRSOCKET_NAMESPACE_BEGIN

//基于OS的tsc计时器的时钟
class TscClock {
public:
    TscClock(const TscClock&) = delete;
    TscClock& operator=(const TscClock&) = delete;
    TscClock(TscClock&&) = delete;
    TscClock& operator=(TscClock&&) = delete;

    struct Anchor {
        uint64_t base_ns  = 0;  //系统时间基准
        uint64_t base_tsc = 0;  //TSC基准值
    };

    static TscClock& instance() {
        static TscClock t;
        return t;
    }

    // 当前rdtsc
    static inline uint64_t rdtsc() {
        return OSApi::tsc_clock_counter();
    }

    // 获取当前时间（纳秒）
    static inline uint64_t now_ns() {
        return instance().current_time_ns();
    }

    // 核心函数：获取高精度时间
    inline uint64_t current_time_ns() const {
        uint64_t tsc = OSApi::tsc_clock_counter();
        uint32_t seq;
        Anchor anc;

        // SeqLock 读操作：无锁，通过版本号重试保证一致性
        do {
            seq = seq_.load(std::memory_order_acquire);
            anc = anchor_;
            std::atomic_thread_fence(std::memory_order_acquire); // 保证读取 anchor 在读取 seq 之后
        } while (seq != seq_.load(std::memory_order_relaxed) || (seq & 1));

        return anc.base_ns + tsc2ns(tsc - anc.base_tsc);
    }

    // 将 TSC 差值转换为纳秒
    inline uint64_t tsc2ns(uint64_t tsc_diff) const {
#if defined(_MSC_VER) && defined(_M_X64)
        // MSVC x64 优化路径
        unsigned __int64 high;
        unsigned __int64 low = _umul128(tsc_diff, multiplier_, &high);
        return (high << (64 - shift_)) | (low >> shift_);
#elif defined(__SIZEOF_INT128__)
        // GCC/Clang 128位整数路径
        return (uint64_t)((unsigned __int128)tsc_diff * multiplier_ >> shift_);
#else
        // 32位系统或旧编译器回退路径：使用浮点数（会有性能损失，但在32位机上通常可接受）
        // 或者拆分乘法。这里为了代码简洁使用 double，如果需要极致性能需手写32位拆分乘法
        return static_cast<uint64_t>(tsc_diff * multiplier_d_);
#endif
    }

    // 重新校准（通常不需要频繁调用）
    bool calibrate(int rounds = 5, int interval_ms = 20) {
        std::vector<double> rates;
        rates.reserve(rounds);

        for (int i = 0; i < rounds; ++i) {
            auto t1     = OSApi::steady_clock_counter();
            uint64_t c1 = OSApi::tsc_clock_counter();

            std::this_thread::sleep_for(std::chrono::milliseconds(interval_ms));

            uint64_t c2 = OSApi::tsc_clock_counter();
            auto t2 = OSApi::steady_clock_counter();

            double ns_delta = (double)(t2 - t1);
            double tsc_delta = (double)(c2 - c1);

            if (tsc_delta > 0) {
                rates.push_back(ns_delta / tsc_delta);
            }
        }

        if (rates.empty()) {
            return false;
        }

        // 取中位数
        std::sort(rates.begin(), rates.end());
        double rate = rates[rates.size() / 2];

        // 写入保护
        uint32_t seq = seq_.load(std::memory_order_relaxed);
        seq_.store(seq + 1, std::memory_order_release); // 变为奇数，阻塞读者

        multiplier_ = static_cast<uint64_t>(rate * (1ULL << shift_));
#if !defined(_MSC_VER) || !defined(_M_X64)
#if !defined(__SIZEOF_INT128__)
        multiplier_d_ = rate; // 32位环境备用
#endif
#endif

        // 更新基准点
        sync_anchor_unlocked();
        seq_.store(seq + 2, std::memory_order_release); // 变为偶数，释放读者

        return true;
    }

    // 同步系统时间锚点
    void sync_system_time() {
        uint32_t seq = seq_.load(std::memory_order_relaxed);
        seq_.store(seq + 1, std::memory_order_release);
        sync_anchor_unlocked();
        seq_.store(seq + 2, std::memory_order_release);
    }

private:
    TscClock() {
        if (!calibrate()) {
            // 如果校准彻底失败，设置一个保底值（假设 1GHz）
            multiplier_ = 1ULL << shift_;
        }
    }

    void sync_anchor_unlocked() {
        // 获取高精度的系统时间作为基准        
        anchor_.base_ns  = OSApi::system_clock_counter();
        anchor_.base_tsc = OSApi::tsc_clock_counter();
    }

    static constexpr int shift_ = 32;
    uint64_t multiplier_ = 0;
#if !defined(_MSC_VER) || !defined(_M_X64)
#if !defined(__SIZEOF_INT128__)
    double multiplier_d_ = 1.0;
#endif
#endif

    // 使用 SeqLock 替代 atomic<Anchor>
    // seq_ 为偶数时表示数据稳定，奇数时表示正在写入
    std::atomic<uint32_t> seq_{ 0 };
    Anchor anchor_{ 0, 0 };
};

ZRSOCKET_NAMESPACE_END

#endif
