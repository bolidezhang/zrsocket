// Some compilers (e.g. VC++) benefit significantly from using this. 
// We've measured 3-4% build speed improvements in apps as a result 
#pragma once

#ifndef ZRSOCKET_TSC_CLOCK_H
#define ZRSOCKET_TSC_CLOCK_H
#include "config.h"
#include "base_type.h"
#include "os_api.h"
#include <atomic>
#include <algorithm>
#include <vector>

ZRSOCKET_NAMESPACE_BEGIN

class TscClock {
public:
    struct Anchor {
        uint64_t base_ns;
        uint64_t base_tsc;
    };

    static TscClock& instance() {
        static TscClock t;
        return t;
    }

    static inline uint64_t now() {
        return OSApi::tsc_clock_counter();
    }

    static inline uint64_t now_ns() {
        return instance().tsc2ns(now());
    }

    //纯整数高精度转换
    //  使用定点整数转换逻辑
    inline uint64_t tsc2ns(uint64_t tsc) const {
#if defined(_MSC_VER) && defined(_M_X64)
        // MSVC 不支持 __int128，使用 _umul128 获取 128 位乘积
        unsigned __int64 high;
        unsigned __int64 low = _umul128(tsc, multiplier_, &high);
        // 右移 32 位：等同于 (high << 32) | (low >> 32)
        return (high << (64 - shift_)) | (low >> shift_);
#else
        // GCC/Clang 支持 __int128
        return (uint64_t)((unsigned __int128)tsc * multiplier_ >> shift_);
#endif
    }

    //获取系统当前精确的纳秒时间
    inline uint64_t current_time_ns() const {
        uint64_t current_tsc    = OSApi::tsc_clock_counter();
        Anchor   current_anchor = anchor_.load(std::memory_order_acquire);
        int64_t  diff_tsc       = static_cast<int64_t>(current_tsc) - static_cast<int64_t>(current_anchor.base_tsc);
        if (diff_tsc > 0) {
            return current_anchor.base_ns + tsc2ns(diff_tsc);
        }

        return current_anchor.base_ns;
    }

    //初始化计算multiplier
    // rounds: odd so median is well-defined
    // interval_ms: 
    void init_calibrate(int rounds = 7, int interval_ms = 50) {
        if (rounds < 0) {
            rounds = 1;
        }
        if (interval_ms < 10) {
            interval_ms = 10;
        }

        const std::chrono::milliseconds sleep_ms(interval_ms); // 50 ms between samples
        std::vector<uint64_t> multipliers(rounds, 0);

        // 进行多次采样取中位数以提高校准精度
        for (int i = 0; i < rounds; ++i) {
            // Make sure to yield to OS before sampling to reduce scheduled-jitter correlation
            std::this_thread::sleep_for(std::chrono::milliseconds(10));

            uint64_t start_ns  = OSApi::steady_clock_counter();
            uint64_t start_tsc = OSApi::tsc_clock_counter();

            // sleep a little to get measurable delta (not too long to avoid drift)
            std::this_thread::sleep_for(sleep_ms);

            int64_t end_tsc = OSApi::tsc_clock_counter();
            int64_t end_ns  = OSApi::steady_clock_counter();

            int64_t diff_ns  = end_ns - start_ns;
            int64_t diff_tsc = end_tsc - start_tsc;
            if (diff_tsc <= 0) {
                continue;
            }

            // 计算 multiplier: (ns << 32) / tsc
#if defined(_MSC_VER)
            // MSVC 兼容写法：
            // 虽然是用 double 计算，但仅在初始化使用，不影响性能
            // 2^32 = 4294967296.0
            uint64_t multiplier = static_cast<uint64_t>((static_cast<double>(diff_ns * 4294967296.0) / static_cast<double>(diff_tsc)));            
#else
            // 这里必须使用 __int128 防止左移 32 位时溢出
            uint64_t multiplier = (uint64_t)(((unsigned __int128)diff_ns << shift) / tsc.diff());
#endif
            multipliers[i] = multiplier;
        }

        //取中位值
        std::sort(multipliers.begin(), multipliers.end());
        multiplier_ = multipliers[multipliers.size() / 2];

        update_anchor();
    }

    //更新锚点，消除与系统时间的长期累计误差
    inline void update_anchor(uint64_t current_ns, uint64_t current_tsc) {
        anchor_.store({ current_ns, current_tsc }, std::memory_order_release);
    }
    inline void update_anchor() {
        uint64_t current_ns  = OSApi::system_clock_counter();
        uint64_t current_tsc = OSApi::tsc_clock_counter();
        update_anchor(current_ns, current_tsc);
    }

private:
    TscClock() {
        init_calibrate();
    }

    static constexpr const int shift_ = 32;
    uint64_t multiplier_ = 0;
    std::atomic<Anchor> anchor_;
};

ZRSOCKET_NAMESPACE_END

#endif
