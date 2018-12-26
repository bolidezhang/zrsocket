#include <list>
#include <deque>
#include <set>
#include <unordered_set>
#include <map>
#include <vector>
#include <random>
#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <cstdlib>
#include <cstdio>
#include <cstdint>
#include <string>
#include <xmmintrin.h>
#include <emmintrin.h>

/*
#include <openssl/rsa.h>
#include <openssl/bn.h>
#include <openssl/pem.h>
#include <openssl/err.h>
*/

#include "SL_Socket_CommonAPI.h"
#include "SL_Crypto_AES.h"
#include "SL_Crypto_RSA.h"
#include "SL_Crypto_CRTRand.h"
#include "SL_Crypto_RaknetRand.h"
#include "SL_Crypto_OSRand.h"
#include "SL_Crypto_HMAC.h"
#include "SL_Crypto_SHA256.h"
#include "SL_Crypto_Hex.h"
#include "SL_Crypto_Base64.h"
#include "SL_Crypto_CRC16.h"
#include "SL_Crypto_Adler32.h"
#include "SL_Crypto_CRC32.h"
#include "SL_Crypto_CRC32C.h"
#include "SL_Crypto_CRC64.h"
#include "SL_Test.h"
#include "SL_Utility_HashTreeMap.h"
#include "SL_Utility_HashTreeSet.h"
#include "SL_Socket_UdpSource_Handler.h"
#include "SL_Rpc_Message.h"
#include "SL_Crypto_MD5.h"
#include "SL_Utility_Math.h"
#include "SL_Sync_SpinMutex.h"
#include "SL_Sync_Mutex.h"
#include "SL_Utility_DataConvert.h"
#include "SL_Utility_StringSearcher.h"


#include "test_server.h"
#include "test_message.h"

#ifdef SOCKETLITE_OS_WINDOWS
    static unsigned int WINAPI send_proc(void *arg)
#else
    static void* send_proc(void *arg)
#endif
{
    TestServerApp *app = (TestServerApp *)arg;

    uint32 test_times = app->test_times_;
    int32 send_error_count = 0;
    int64 start_timestamp = SL_Socket_CommonAPI::util_system_counter();
    for (uint32 i=0; i<test_times; ++i)
    {
        SL_Seda_SignalEvent<5000> event;
        if (app->stage_.push_event(&event) < 0)
        {
            ++send_error_count;
        }
    }
    printf("stage push count:%d, tack tims(us):%lld, send_error_count:%d\n", test_times, SL_Socket_CommonAPI::util_system_counter_time_us(SL_Socket_CommonAPI::util_system_counter(), \
        start_timestamp), send_error_count);

    //std::make_pair()
    std::int16_t i;
    return 0;
}

#ifdef SOCKETLITE_OS_WINDOWS
    static unsigned int WINAPI send_proc_ex(void *arg)
#else
    static void* send_proc_ex(void *arg)
#endif
{
    TestServerApp *app = (TestServerApp *)arg;

    uint32 test_times       = app->test_times_;
    int64 start_timestamp   = SL_Socket_CommonAPI::util_system_counter();
    int32 send_error_count  = 0;
    int node_len = sizeof(SL_Seda_Event) + sizeof(void *);
    char *node;
    SL_Seda_Event *event;

    for (uint32 i=0; i<test_times; ++i)
    {
        node  = (char *)sl_malloc(node_len);
        event = (SL_Seda_Event *)(node + sizeof(void *));
        event->type_ = 5000;
        if (app->stage_ex_.push_event(event) < 0)
        {
            ++send_error_count;
        }

        SL_Seda_SignalEvent<5000> event;
        if (app->stage_ex_.push_event(&event) < 0)
        {
            ++send_error_count;
        }
    }
    printf("stage_ex push count:%d, tack tims(us):%lld, send_error_count:%d\n", test_times, SL_Socket_CommonAPI::util_system_counter_time_us(SL_Socket_CommonAPI::util_system_counter(), \
        start_timestamp), send_error_count);

    return 0;
}

TestServerApp::TestServerApp()
{
}

TestServerApp::~TestServerApp()
{
}

int TestServerApp::run()
{
#ifndef SOCKETLITE_OS_WINDOWS
    signal(SIGPIPE, SIG_IGN);
    signal(SIGSEGV, SIG_IGN);
    signal(SIGALRM, SIG_IGN);
#endif

    test_obj_pool_.init(1000000, 10000, 10);
    test_tcpserver_.set_interface(&test_obj_pool_, &test_runner_);
    test_tcpserver_.set_config(1, true, 1000000, 1024, 1024, 2, 1);
    handler_mgr_.open(1, 65536, 1024, 3600000, 1, 1, 1, true, true);
    //stage_.open(1, 20000000, 16, 1000, true, 0);
    //stage_ex_.open(1, 20000000, 16, 1000, true, 1);
    //test_runner_.set_buffer_size(65536*4, 65536*4);
    test_runner_.set_buffer_size(1024*1024, 65536*4);
    test_runner_.open(SL_Socket_Handler::READ_EVENT_MASK, 1000000, 1, 1);
    test_tcpserver_.open(port_, 8192);

    //disruptor_queue1_.init(20000000, 16, 1, 1);
    //disruptor_queue1_.add_handler(&disruptor_handler_, true, false);
    //disruptor_handler_.open(&disruptor_queue1_, false, 1, false);

    //disruptor_queue2_.init(20000000, 16, 1, 1);
    //disruptor_queue2_.add_handler(&disruptor_handler_, true, false);
    //disruptor_handler_.open(&disruptor_queue2_, false, 1, false);

    //
    this->stage2_.open(10, 1000, 64, 10, true, 0, 8);

    if (is_stage_ex_)
    {
        stage_ex_.open(1, 20000000, 16, 1000, false, 1);
        send_thread_group_.start(send_proc_ex, this, thread_num_, thread_num_);
    }
    else
    {
        stage_.open(1, 20000000, 16, 1000, false, 0);
        send_thread_group_.start(send_proc, this, thread_num_, thread_num_);
    }
    start_timestamp_ = SL_Socket_CommonAPI::util_system_counter();

    //SL_Seda_SignalEvent<5000> event;
    //start_timestamp_ = SL_Socket_CommonAPI::util_system_counter();
    //for (uint32 i=0; i<test_times_; ++i)
    //{
    //    stage_.push_event(&event);
    //    //stage_ex_.push_event(&event);
    //    //disruptor_queue1_.push(&event, -1);
    //    //disruptor_queue2_.push(&event, 1);
    //}
    //printf("stage push count:%d, tack tims(us):%lld\n", test_times_, SL_Socket_CommonAPI::util_system_counter_time_us(SL_Socket_CommonAPI::util_system_counter(), start_timestamp_));

    //start_timestamp_ = SL_Socket_CommonAPI::util_system_counter();
    //for (uint32 i=0; i<test_times_; ++i)
    //{
    //    stage_ex_.push_event(&event);
    //    //disruptor_queue1_.push(&event, -1);
    //    //disruptor_queue2_.push(&event, 1);
    //}
    //printf("stage_ex push count:%d, tack tims(us):%lld\n", test_times_, SL_Socket_CommonAPI::util_system_counter_time_us(SL_Socket_CommonAPI::util_system_counter(), start_timestamp_));

#ifdef SOCKETLITE_OS_WINDOWS
    test_runner_.thread_wait();
    //test_runner_.thread_event_loop(1);
    //test_runner_.thread_event_loop(1);
#else
    for (;;)
    {
        test_runner_.event_loop(1);
    }
#endif

    stage_.close();
    handler_mgr_.close();
    test_runner_.close();
    test_tcpserver_.close();

    return 0;
}

inline int util_uint32_to_str(uint32 value, char str[20])
{
    static char digits[] = "0001020304050607080910111213141516171819"
        "20212223242526272829303132333435363738394041424344454647484950515253545556575859"
        "60616263646566676869707172737475767778798081828384858687888990919293949596979899";
     
#define LESS10                                      \
    do {                                            \
        str[0] = '0' + value;                       \
    } while (0)

#define LESS100                                     \
    do {                                            \
        i = value << 1;                             \
        str[1]  = digits[i + 1];                    \
        str[0]  = digits[i];                        \
    } while (0)

#define MOD100(x)                                   \
    do {                                            \
        temp        = value / 100;                  \
        i           = (value - temp * 100) << 1;    \
        value       = temp;                         \
        str[x]      = digits[i + 1];                \
        str[x - 1]  = digits[i];                    \
    } while (0)

    uint32 temp;
    uint32 i;

    if (value < 10)
    {
        LESS10;
        return 1;
    }
    else if (value < 100)
    {
        LESS100;
        return 2;
    }
    else if (value < 1000)
    {
        MOD100(2);
        LESS10;
        return 3;
    }
    else if (value < 10000)
    {
        MOD100(3);
        LESS100;
        return 4;
    }
    else if (value < 100000)
    {
        MOD100(4);
        MOD100(2);
        LESS10;
        return 5;
    }
    else if (value < 1000000)
    {
        MOD100(5);
        MOD100(3);
        LESS100;
        return 6;
    }
    else if (value < 10000000)
    {
        MOD100(6);
        MOD100(4);
        MOD100(2);
        LESS10;
        return 7;
    }
    else if (value < 100000000)
    {
        MOD100(7);
        MOD100(5);
        MOD100(3);
        LESS100;
        return 8;
    }
    else if (value < 1000000000)
    {
        MOD100(8);
        MOD100(6);
        MOD100(4);
        MOD100(2);
        LESS10;
        return 9;
    }
    else
    {
        MOD100(9);
        MOD100(7);
        MOD100(5);
        MOD100(3);
        LESS100;
        return 10;
    }

#undef LESS10
#undef LESS100
#undef MOD100

    //if (value < 10000000000ull)
    //{
    //    MOD100(9);
    //    MOD100(7);
    //    MOD100(5);
    //    MOD100(3);
    //    LESS100;
    //    return 10;
    //}

    //if (value < 100000000000ull)
    //    return 11;
    //if (value < 1000000000000ull)
    //    return 12;
    //if (value < 10000000000000ull)
    //    return 13;
    //if (value < 100000000000000ull)
    //    return 14;
    //if (value < 1000000000000000ull)
    //    return 15;
    //if (value < 10000000000000000ull)
    //    return 16;
    //if (value < 100000000000000000ull)
    //    return 17;
    //if (value < 1000000000000000000ull)
    //    return 18;
    //if (value < 10000000000000000000ull)
    //    return 19;

    //return 20;
}

static inline int util_int32_to_str(int32 value, char str[20])
{
    if (value >= 0)
    {
        return util_uint32_to_str(value, str);
    }
    else
    {
        *str++ = '-';
        return util_uint32_to_str(-value, str) + 1;
    }
}

static inline void kmp_next2(char sub[], int next[], int len)
{
    int j = 0;
    next[0] = 0;
    for (int i = 1; i < len; ++i)
    {
        while (j > 0 && sub[j] != sub[i])
        {
            j = next[j - 1];
        }
        if (sub[j] == sub[i])
        {
            ++j;
        }
        next[i] = j;
    }
}

static inline void kmp_next2_1(char sub[], int next[], int len)
{
    //next[0] = 0;
    //int i = 1;
    //int j = 0;
    //while (i < len)
    //{
    //    if (sub[i] == sub[j])
    //    {
    //        ++j;
    //    }
    //    else
    //    {
    //        j = next[j-1];
    //    }
    //    next[i] = j;
    //    ++i;
    //}
}

static inline void kmp_next3(char sub[], int next[], int len)
{
    int j = 0;
    next[0] = -1;
    for (int i = 1; i < len; ++i)
    {
        j = next[i-1];
        while (j > 0 && sub[j+1] != sub[i])
        {
            j = next[j];
        }
        if (sub[j+1] == sub[i])
        {
            next[i] = j + 1;
        }
        else
        {
            next[i] = -1;
        }
    }
}

static inline void kmp_next4(char sub[], int next[], int len)
{
    next[0] = -1;
    int k = -1;
    int j = 0;
    while (j < (len - 1))
    {
        if ((k == -1) || (sub[j] == sub[k]))
        {
            ++k;
            ++j;
            if (sub[j] != sub[k])
            {
                next[j] = k;
            }
            else
            {
                next[j] = next[k];
            }
        }
        else
        {
            k = next[k];
        }
    }
}

static inline void kmp_next5(char sub[], int next[], int len)
{
    next[0] = -1;
    int k = -1;
    int j = 0;
    while (j < (len-1))
    {
        if ((k == -1) || (sub[j] == sub[k]))
        {
            ++k;
            ++j;
            next[j] = k;
        }
        else
        {
            k = next[k];
        }
    }
}

static inline void kmp_next6(char sub[], int next[], int len)
{
    next[0] = 0;
    next[1] = 0;
    int i = 1;
    int j = 0;
    while (i < len-1)
    {
        if (sub[i] != sub[j])
        {
            if (j == 0)
            {
                ++i;
                next[i] = 0;
            }
            j = next[j];
        }
        else
        {
            ++j;
            ++i;
            next[i] = j;
        }
    }
}

static inline int kmp_search23(const char *str, int str_len, const char *sub, int sub_len, const int *next)
{
    int str_i = 0;
    int sub_j = 0;

    //方法1
    for (str_i = 0; str_i < str_len; ++str_i)
    {
        while (sub_j > 0 && sub[sub_j] != str[str_i])
        {
            sub_j = next[sub_j - 1];
        }

        if (sub[sub_j] == str[str_i])
        {
            ++sub_j;
        }

        if (sub_j == sub_len)
        {
            return str_i - sub_j + 1;
        }
    }

    ////方法2
    //while (str_i < str_len && sub_j < sub_len)
    //{
    //    if (str[str_i] != sub[sub_j])
    //    {
    //        if (sub_j == 0)
    //        {
    //            ++str_i;
    //        }
    //        else
    //        {
    //            sub_j = next[sub_j - 1];
    //        }
    //    }
    //    else
    //    {
    //        ++str_i;
    //        ++sub_j;
    //    }
    //}
    //if (sub_j == sub_len)
    //{
    //    return str_i - sub_j;
    //}

    //for (; (sub_len <= str_len && sub_j < sub_len); )
    //{
    //    if (str[str_i] != sub[sub_j])
    //    {
    //        if (sub_j == 0)
    //        {
    //            ++str_i;
    //        }
    //        else
    //        {
    //            sub_j = next[sub_j - 1];
    //        }
    //    }
    //    else
    //    {
    //        ++str_i;
    //        ++sub_j;
    //        --str_len;
    //    }
    //}
    //if (sub_j == sub_len)
    //{
    //    return str_i - sub_j;
    //}

    //for (; sub_len <= str_len; ++str_i, --str_len)
    //{
    //    for (int temp_i = str_i, sub_j = 0; ; ++temp_i)
    //    {            
    //        if (str[temp_i] != sub[sub_j])
    //        {
    //            if (sub_j == 0)
    //            {
    //                ++str_i;
    //            }
    //            else
    //            {
    //                sub_j = next[sub_j - 1];
    //            }
    //        }
    //        else
    //        {
    //            ++sub_j;
    //            if (sub_j == sub_len)
    //            {
    //                return str_i;
    //            }
    //        }
    //    }
    //}

    return -1;
}

static inline int kmp_search45(const char *str, int str_len, const char *sub, int sub_len, const int *next)
{
    int str_i = 0;
    int sub_j = 0;
    while (str_i < str_len && sub_j < sub_len)
    {
        ////方法1
        //if ( (sub_j == -1) || (str[str_i] == sub[sub_j]) )
        //{
        //    ++str_i;
        //    ++sub_j;
        //}
        //else
        //{
        //    sub_j = next[sub_j];
        //}

        //方法2
        if (str[str_i] != sub[sub_j])
        {
            if (sub_j >= 0)
            {
                sub_j = next[sub_j];
            }
            else
            {
                ++str_i;
                sub_j = 0;
            }
        }
        else
        {
            ++str_i;
            ++sub_j;
        }
    }
    if (sub_j == sub_len)
    {
        return str_i - sub_j;
    }
    return -1;
}

inline int bf_search(const char *str, int str_len, const char *sub, int sub_len)
{
    int str_i = 0;
    int sub_j = 0;
    //while (str_i < str_len && sub_j < sub_len)
    //{
    //    if (str[str_i] != sub[sub_j])
    //    {
    //        str_i -= (sub_j - 1);
    //        sub_j = 0;
    //    }
    //    else
    //    {
    //        ++str_i;
    //        ++sub_j;
    //    }
    //}

    //VC: std::search()
    //// TEMPLATE FUNCTION search WITH PRED
    //template<class _FwdIt1,
    //class _FwdIt2,
    //class _Diff1,
    //class _Diff2,
    //class _Pr> inline
    //    _FwdIt1 _Search(_FwdIt1 _First1, _FwdIt1 _Last1,
    //    _FwdIt2 _First2, _FwdIt2 _Last2, _Pr _Pred, _Diff1 *, _Diff2 *)
    //{	// find first [_First2, _Last2) satisfying _Pred
    //    _Diff1 _Count1 = 0;
    //    _Distance(_First1, _Last1, _Count1);
    //    _Diff2 _Count2 = 0;
    //    _Distance(_First2, _Last2, _Count2);

    //    for (; _Count2 <= _Count1; ++_First1, --_Count1)
    //    {	// room for match, try it
    //        _FwdIt1 _Mid1 = _First1;
    //        for (_FwdIt2 _Mid2 = _First2; ; ++_Mid1, ++_Mid2)
    //            if (_Mid2 == _Last2)
    //                return (_First1);
    //            else if (!_Pred(*_Mid1, *_Mid2))
    //                break;
    //    }
    //    return (_Last1);
    //}

    for (; sub_len <= str_len; ++str_i, --str_len)
    {
        for (int temp_i = str_i, sub_j = 0; ; ++temp_i)
        {            
            if (str[temp_i] != sub[sub_j])
            {
                break;
            }
            else
            {
                ++sub_j;
                if (sub_j == sub_len)
                {
                    return str_i;
                }
            }
        }
    }


    //gcc:std::find()
    ///// This is an overload used by find() for the RAI case.
    //template<typename _RandomAccessIterator, typename _Tp>
    //_RandomAccessIterator
    //    __find(_RandomAccessIterator __first, _RandomAccessIterator __last,
    //    const _Tp& __val, random_access_iterator_tag)
    //{
    //    typename iterator_traits<_RandomAccessIterator>::difference_type
    //        __trip_count = (__last - __first) >> 2;

    //    for (; __trip_count > 0; --__trip_count)
    //    {
    //        if (*__first == __val)
    //            return __first;
    //        ++__first;

    //        if (*__first == __val)
    //            return __first;
    //        ++__first;

    //        if (*__first == __val)
    //            return __first;
    //        ++__first;

    //        if (*__first == __val)
    //            return __first;
    //        ++__first;
    //    }

    //    switch (__last - __first)
    //    {
    //    case 3:
    //        if (*__first == __val)
    //            return __first;
    //        ++__first;
    //    case 2:
    //        if (*__first == __val)
    //            return __first;
    //        ++__first;
    //    case 1:
    //        if (*__first == __val)
    //            return __first;
    //        ++__first;
    //    case 0:
    //    default:
    //        return __last;
    //    }
    //}

    //gcc:std::search()
    //template<typename _ForwardIterator1, typename _ForwardIterator2>
    //_ForwardIterator1
    //    search(_ForwardIterator1 __first1, _ForwardIterator1 __last1,
    //    _ForwardIterator2 __first2, _ForwardIterator2 __last2)
    //{
    //    // concept requirements
    //    __glibcxx_function_requires(_ForwardIteratorConcept<_ForwardIterator1>)
    //        __glibcxx_function_requires(_ForwardIteratorConcept<_ForwardIterator2>)
    //        __glibcxx_function_requires(_EqualOpConcept<
    //        typename iterator_traits<_ForwardIterator1>::value_type,
    //        typename iterator_traits<_ForwardIterator2>::value_type>)
    //        __glibcxx_requires_valid_range(__first1, __last1);
    //    __glibcxx_requires_valid_range(__first2, __last2);

    //    // Test for empty ranges
    //    if (__first1 == __last1 || __first2 == __last2)
    //        return __first1;

    //    // Test for a pattern of length 1.
    //    _ForwardIterator2 __p1(__first2);
    //    if (++__p1 == __last2)
    //        return _GLIBCXX_STD_P::find(__first1, __last1, *__first2);

    //    // General case.
    //    _ForwardIterator2 __p;
    //    _ForwardIterator1 __current = __first1;

    //    for (;;)
    //    {
    //        __first1 = _GLIBCXX_STD_P::find(__first1, __last1, *__first2);
    //        if (__first1 == __last1)
    //            return __last1;

    //        __p = __p1;
    //        __current = __first1;
    //        if (++__current == __last1)
    //            return __last1;

    //        while (*__current == *__p)
    //        {
    //            if (++__p == __last2)
    //                return __first1;
    //            if (++__current == __last1)
    //                return __last1;
    //        }
    //        ++__first1;
    //    }
    //    return __first1;
    //}

    if (sub_j == sub_len)
    {
        return str_i - sub_j;
    }
    return -1;
}

#define BF_SEARCH(str, str_len, sub, sub_len, ret)  \
{                                                   \
    int str_i = 0;                                  \
    int sub_j = 0;                                  \
    while (str_i < str_len && sub_j < sub_len)      \
    {                                               \
        if (str[str_i] != sub[sub_j])               \
        {                                           \
            str_i -= (sub_j - 1);                   \
            sub_j = 0;                              \
        }                                           \
        else                                        \
        {                                           \
            ++str_i;                                \
            ++sub_j;                                \
        }                                           \
    }                                               \
    if (sub_j == sub_len)                           \
        ret = str_i - sub_j;                        \
    else                                            \
        ret = -1;                                   \
}

#define BF_SEARCH2(str, str_len, sub, sub_len, ret)         \
{                                                           \
    int str_len_1 = str_len;                                \
    int str_i = 0;                                          \
    int sub_j = 0;                                          \
    for (; sub_len <= str_len_1; ++str_i, --str_len_1)      \
    {                                                       \
        for (int temp_i = str_i, sub_j = 0; ; ++temp_i)     \
        {                                                   \
            if (str[temp_i] != sub[sub_j])                  \
            {                                               \
                break;                                      \
            }                                               \
            else                                            \
            {                                               \
                ++sub_j;                                    \
                if (sub_j == sub_len)                       \
                {                                           \
                    goto EXIT_PROC;                         \
                }                                           \
            }                                               \
        }                                                   \
    }                                                       \
EXIT_PROC:                                                  \
    ret = str_i;                                            \
}

class TestA
{
public:
    TestA(){};
    virtual ~TestA(){};

    void M1()
    {
    };
    void M2()
    {
    };
};

#define _FORMAT(n) "%0"#n"d"
#define FORMAT(n)  _FORMAT(n)
#define EXE_VERSION "3.7.23.20150410"
int main(int argc, char *argv[])
{
    {
        std::vector<int> vec = { 2, 3, 4, -2, 2, 4, 3 };
        std::unordered_set<int> exist_set;

        for (auto it = vec.begin(); it != vec.end(); ++it) {
            exist_set.emplace(*it);
        }
        vec.clear();
        vec.insert(vec.end(), exist_set.begin(), exist_set.end());
        exist_set.clear();        
    }

    //{
    //    float f = 14.68;
    //    float f1 = f + 0.5;
    //    int   i = f1;
    //    unsigned int   i1 = (unsigned int)f1;

    //    printf("f:%f, f1:%f, i:%d, i1:%d\n", f, f1, i, i1);
    //}

    //{
    //    TestA a;
    //    int len = sizeof(a);
    //    printf("sizoef(A):%d, sizoef(a):%d\n", sizeof(TestA),len);
    //}


    //printf("test_server version: %s\n", EXE_VERSION);
    //int i5 = 99;
    //int j6 = 4;
    ////printf(FORMAT(INT_MAX), i5);

    //{
    //    int8 i = 1;
    //    int8 j = 1;
    //    printf("i:%d,j:%d",i,j);
    //}

    //SL_Socket_CommonAPI::socket_init(2, 2);

    //{
    //    SL_SharedBuffer buf1;
    //    buf1.write("123456789",9);
    //    {
    //        SL_SharedBuffer buf2 = buf1;
    //        //buf2.clear();
    //        buf2.write("123");
    //    }
    //}

    //{
    //    int  ret_len = 0;
    //    char buf[1024] = {0};
    //    int  id = 500000;
    //    ret_len = SL_Socket_CommonAPI::util_snprintf(buf, 1024, "id:%ld",id);
    //    ret_len = -1;
    //}

    //int   ret = 0;
    //int32 i = 0;
    //int64 j = 0;
    //int32 copy_count = atoi(argv[1]);
    //int32 test_times = atol(argv[2]);
    //uint64 temp_timestamp = 0;
    //uint64 temp_timestamp_end = 0;
    //printf("source:%s, atoi:%d, SL_Utility_DataConvert::atoi:%d, SL_Utility_DataConvert::atoui:%u\n", 
    //    argv[1], test_times, SL_Utility_DataConvert::atoi(argv[1]), SL_Utility_DataConvert::atoui(argv[1]));

    //char *dest = NULL;
    //char *src_char = NULL;
    //char *cmp_char = NULL;
    //dest        = (char *)sl_malloc(copy_count+1);
    //src_char    = (char *)sl_malloc(copy_count);
    //cmp_char    = (char *)sl_malloc(copy_count);
    //int src_size = copy_count;
    //memset(dest, 0, copy_count+1);
    //memset(src_char, '9', copy_count);
    //memset(cmp_char, 0, copy_count);

    //memset(dest, 0, copy_count);
    //i = 1;
    //temp_timestamp = SL_Socket_CommonAPI::util_system_counter();
    //for (; i<test_times; ++i)
    //{
    //    std::memcpy(dest, src_char, src_size);
    //}
    //temp_timestamp_end = SL_Socket_CommonAPI::util_system_counter();
    //printf("std::memcpy tack time:%lld\ndest:%s\n", SL_Socket_CommonAPI::util_system_counter_time_us(temp_timestamp_end,temp_timestamp), dest);

    //memset(dest, 0, copy_count);
    //i = 1;
    //temp_timestamp = SL_Socket_CommonAPI::util_system_counter();
    //for (; i<test_times; ++i)
    //{
    //    sl_memcpy(dest, src_char, src_size);
    //}
    //temp_timestamp_end = SL_Socket_CommonAPI::util_system_counter();
    //printf("sl_memcpy tack time:%lld\ndest:%s\n", SL_Socket_CommonAPI::util_system_counter_time_us(temp_timestamp_end,temp_timestamp), dest);

    if (argc < 3)
    {
        printf("please use the format: port to launch server(e.g.: 2000)\n");
        return 0;
    }

    TestServerApp *app  = TestServerApp::instance();
    app->port_          = atoi(argv[1]);
    app->test_times_    = atoi(argv[2]);
    app->thread_num_    = atoi(argv[3]);
    //app->is_stage_ex_   = atoi(argv[4]);
    app->is_stage_ex_ = 0;
    return app->run();
}

