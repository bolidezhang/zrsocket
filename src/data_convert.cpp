#include <assert.h>
#include "zrsocket/data_convert.h"

ZRSOCKET_NAMESPACE_BEGIN

char DataConvert::digits_[201] = "0001020304050607080910111213141516171819"
"20212223242526272829303132333435363738394041424344454647484950515253545556575859"
"60616263646566676869707172737475767778798081828384858687888990919293949596979899";

//单字符转整数
#define ZRSOCKET_CHAR_CONVERT_INTEGER           \
    if (*str >= '0' && *str <= '9')             \
    {                                           \
        if (first_num_flag)                     \
        {                                       \
            ret = ret * 10 + *str - '0';        \
        }                                       \
        else                                    \
        {                                       \
            first_num_flag = true;              \
            ret = *str - '0';                   \
        }                                       \
    }                                           \
    else if (*str == '-')                       \
    {                                           \
        if (first_num_flag)                     \
        {                                       \
            goto EXIT_PROC;                     \
        }                                       \
        else                                    \
        {                                       \
            first_num_flag = true;              \
            negative_flag  = true;              \
        }                                       \
    }                                           \
    else if (*str == ' ')                       \
    {                                           \
        if (first_num_flag)                     \
        {                                       \
            goto EXIT_PROC;                     \
        }                                       \
    }                                           \
    else if (*str == '+')                       \
    {                                           \
        if (first_num_flag)                     \
        {                                       \
            goto EXIT_PROC;                     \
        }                                       \
        else                                    \
        {                                       \
            first_num_flag = true;              \
        }                                       \
    }                                           \
    else                                        \
    {                                           \
        goto EXIT_PROC;                         \
    }

//单字符转无符号整数
#define ZRSOCKET_CHAR_CONVERT_UNSIGNED_INTEGER  \
    if (*str >= '0' && *str <= '9')             \
    {                                           \
        if (first_num_flag)                     \
        {                                       \
            ret = ret * 10 + *str - '0';        \
        }                                       \
        else                                    \
        {                                       \
            first_num_flag = true;              \
            ret = *str - '0';                   \
        }                                       \
    }                                           \
    else if (*str == ' ')                       \
    {                                           \
        if (first_num_flag)                     \
        {                                       \
            goto EXIT_PROC;                     \
        }                                       \
    }                                           \
    else if (*str == '+')                       \
    {                                           \
        if (first_num_flag)                     \
        {                                       \
            goto EXIT_PROC;                     \
        }                                       \
        else                                    \
        {                                       \
            first_num_flag = true;              \
        }                                       \
    }                                           \
    else                                        \
    {                                           \
        goto EXIT_PROC;                         \
    }

//字符串转4字节整数
int32_t DataConvert::atoi(const char *str)
{
    assert(str);
    int32_t ret = 0;
    bool  negative_flag  = false;   //负数标记
    bool  first_num_flag = false;   //是否找到第一个数字标记

    for (;;) {
        ZRSOCKET_CHAR_CONVERT_INTEGER;
        ++str;
    }

EXIT_PROC:
    if (negative_flag) {
        return -ret;
    }
    return ret;
}

//字符串转4字节整数
int32_t DataConvert::atoi(const char *str, uint_t len, char **endptr)
{
    assert(str);
    int32_t ret = 0;
    bool  negative_flag  = false;   //负数标记
    bool  first_num_flag = false;   //是否找到第一个数字标记

    if (len < 1) {
        for (;;) {
            ZRSOCKET_CHAR_CONVERT_INTEGER;
            ++str;
        }
    }
    else {
        for (uint_t i = 0; i < len; ++i) {
            ZRSOCKET_CHAR_CONVERT_INTEGER;
            ++str;
        }
    }

EXIT_PROC:
    if (nullptr != endptr) {
        *endptr = (char *)str;
    }
    if (negative_flag) {
        return -ret;
    }
    return ret;
}

//字符串转4字节无符号整数
uint32_t DataConvert::atoui(const char *str)
{
    assert(str);
    uint32_t ret = 0;
    bool   first_num_flag = false;  //是否找到第一个数字标记

    for (;;) {
        ZRSOCKET_CHAR_CONVERT_UNSIGNED_INTEGER;
        ++str;
    }

EXIT_PROC:
    return ret;
}

uint32_t DataConvert::atoui(const char *str, uint_t len, char **endptr)
{
    assert(str);
    uint32_t ret = 0;
    bool   first_num_flag = false;  //是否找到第一个数字标记

    if (len < 1) {
        for (;;) {
            ZRSOCKET_CHAR_CONVERT_UNSIGNED_INTEGER;
            ++str;
        }
    }
    else {
        for (uint_t i = 0; i < len; ++i) {
            ZRSOCKET_CHAR_CONVERT_UNSIGNED_INTEGER;
            ++str;
        }
    }

EXIT_PROC:
    if (nullptr != endptr) {
        *endptr = (char *)str;
    }
    return ret;
}

//字符串转8字节整数
int64_t DataConvert::atoll(const char *str)
{
    assert(str);
    int64_t ret = 0;
    bool  negative_flag  = false;   //负数标记
    bool  first_num_flag = false;   //是否找到第一个数字标记

    for (;;) {
        ZRSOCKET_CHAR_CONVERT_INTEGER;
        ++str;
    }

EXIT_PROC:
    if (negative_flag) {
        return -ret;
    }
    return ret;
}

int64_t DataConvert::atoll(const char *str, uint_t len, char **endptr)
{
    assert(str);
    int64_t ret = 0;
    bool  negative_flag  = false;   //负数标记
    bool  first_num_flag = false;   //是否找到第一个数字标记

    if (len < 1) {
        for (;;) {
            ZRSOCKET_CHAR_CONVERT_INTEGER;
            ++str;
        }
    }
    else
    {
        for (uint_t i = 0; i < len; ++i) {
            ZRSOCKET_CHAR_CONVERT_INTEGER;
            ++str;
        }
    }

EXIT_PROC:
    if (nullptr != endptr) {
        *endptr = (char *)str;
    }
    if (negative_flag) {
        return -ret;
    }
    return ret;
}

//字符串转8字节无符号整数
uint64_t DataConvert::atoull(const char *str)
{
    assert(str);
    uint64_t ret = 0;
    bool first_num_flag = false;  //是否找到第一个数字标记

    for (;;) {
        ZRSOCKET_CHAR_CONVERT_UNSIGNED_INTEGER;
        ++str;
    }

EXIT_PROC:
    return ret;
}

uint64_t DataConvert::atoull(const char *str, uint_t len, char **endptr)
{
    assert(str);
    uint64_t ret = 0;
    bool   first_num_flag = false;  //是否找到第一个数字标记

    if (len < 1) {
        for (;;) {
            ZRSOCKET_CHAR_CONVERT_UNSIGNED_INTEGER;
            ++str;
        }
    }
    else {
        for (uint_t i = 0; i < len; ++i) {
            ZRSOCKET_CHAR_CONVERT_UNSIGNED_INTEGER;
            ++str;
        }
    }

EXIT_PROC:
    if (nullptr != endptr) {
        *endptr = (char *)str;
    }
    return ret;
}

//整数转字符串
//经测试:  window7-32 vc2010 cpu: 4*i5-2310 memory: 4G:
//              将if条件展开与while循环相比较 性能提升明显
//         linux:
//              将if条件展开与while循环相比较 性能反而下降

#ifdef ZRSOCKET_OS_WINDOWS

int DataConvert::uitoa(uint32_t value, char str[12])
{
#define LESS10                                      \
    do {                                            \
        str[0] = '0' + (char)value;                 \
    } while (0)

#define LESS100                                     \
    do {                                            \
        i = value << 1;                             \
        str[1]  = digits_[i + 1];                   \
        str[0]  = digits_[i];                       \
    } while (0)

#define MOD100(x)                                   \
    do {                                            \
        temp        = value / 100;                  \
        i           = (value - temp * 100) << 1;    \
        value       = temp;                         \
        str[x]      = digits_[i + 1];               \
        str[x - 1]  = digits_[i];                   \
    } while (0)

    if (value < 10) {
        LESS10;
        str[1] = '\0';
        return 1;
    }

    uint32_t i;
    if (value < 100) {
        LESS100;
        str[2] = '\0';
        return 2;
    }
    uint32_t temp;
    if (value < 1000) {
        MOD100(2);
        LESS10;
        str[3] = '\0';
        return 3;
    }
    if (value < 10000) {
        MOD100(3);
        LESS100;
        str[4] = '\0';
        return 4;
    }
    if (value < 100000) {
        MOD100(4);
        MOD100(2);
        LESS10;
        str[5] = '\0';
        return 5;
    }
    if (value < 1000000) {
        MOD100(5);
        MOD100(3);
        LESS100;
        str[6] = '\0';
        return 6;
    }
    if (value < 10000000) {
        MOD100(6);
        MOD100(4);
        MOD100(2);
        LESS10;
        str[7] = '\0';
        return 7;
    }
    if (value < 100000000) {
        MOD100(7);
        MOD100(5);
        MOD100(3);
        LESS100;
        str[8] = '\0';
        return 8;
    }
    if (value < 1000000000) {
        MOD100(8);
        MOD100(6);
        MOD100(4);
        MOD100(2);
        LESS10;
        str[9] = '\0';
        return 9;
    }

    MOD100(9);
    MOD100(7);
    MOD100(5);
    MOD100(3);
    LESS100;
    str[10] = '\0';
    return 10;

#undef LESS10
#undef LESS100
#undef MOD100
}

int DataConvert::ulltoa(uint64_t value, char str[21])
{
#define LESS10                                              \
    do {                                                    \
        str[0] = '0' + (char)value;                         \
    } while (0)

#define LESS100                                             \
    do {                                                    \
        i = ((uint_t)value) << 1;                           \
        str[1]  = digits_[i + 1];                           \
        str[0]  = digits_[i];                               \
    } while (0)

#define MOD100(x)                                           \
    do {                                                    \
        temp        = value / 100;                          \
        i           = ((uint_t)(value - temp * 100)) << 1;  \
        value       = temp;                                 \
        str[x]      = digits_[i + 1];                       \
        str[x - 1]  = digits_[i];                           \
    } while (0)

    if (value < 10ull)
    {
        LESS10;
        str[1] = '\0';
        return 1;
    }

    uint_t i;
    if (value < 100ull) {
        LESS100;
        str[2] = '\0';
        return 2;
    }

    uint64_t temp;
    if (value < 1000ull) {
        MOD100(2);
        LESS10;
        str[3] = '\0';
        return 3;
    }
    if (value < 10000ull) {
        MOD100(3);
        LESS100;
        str[4] = '\0';
        return 4;
    }
    if (value < 100000ull) {
        MOD100(4);
        MOD100(2);
        LESS10;
        str[5] = '\0';
        return 5;
    }
    if (value < 1000000ull) {
        MOD100(5);
        MOD100(3);
        LESS100;
        str[6] = '\0';
        return 6;
    }
    if (value < 10000000ull) {
        MOD100(6);
        MOD100(4);
        MOD100(2);
        LESS10;
        str[7] = '\0';
        return 7;
    }
    if (value < 100000000ull) {
        MOD100(7);
        MOD100(5);
        MOD100(3);
        LESS100;
        str[8] = '\0';
        return 8;
    }
    if (value < 1000000000ull) {
        MOD100(8);
        MOD100(6);
        MOD100(4);
        MOD100(2);
        LESS10;
        str[9] = '\0';
        return 9;
    }
    if (value < 10000000000ull) {
        MOD100(9);
        MOD100(7);
        MOD100(5);
        MOD100(3);
        LESS100;
        str[10] = '\0';
        return 10;
    }
    if (value < 100000000000ull) {
        MOD100(10);
        MOD100(8);
        MOD100(6);
        MOD100(4);
        MOD100(2);
        LESS10;
        str[11] = '\0';
        return 11;
    }
    if (value < 1000000000000ull) {
        MOD100(11);
        MOD100(9);
        MOD100(7);
        MOD100(5);
        MOD100(3);
        LESS100;
        str[12] = '\0';
        return 12;
    }
    if (value < 10000000000000ull) {
        MOD100(12);
        MOD100(10);
        MOD100(8);
        MOD100(6);
        MOD100(4);
        MOD100(2);
        LESS10;
        str[13] = '\0';
        return 13;
    }
    if (value < 100000000000000ull) {
        MOD100(13);
        MOD100(11);
        MOD100(9);
        MOD100(7);
        MOD100(5);
        MOD100(3);
        LESS100;
        str[14] = '\0';
        return 14;
    }
    if (value < 1000000000000000ull) {
        MOD100(14);
        MOD100(12);
        MOD100(10);
        MOD100(8);
        MOD100(6);
        MOD100(4);
        MOD100(2);
        LESS10;
        str[15] = '\0';
        return 15;
    }
    if (value < 10000000000000000ull) {
        MOD100(15);
        MOD100(13);
        MOD100(11);
        MOD100(9);
        MOD100(7);
        MOD100(5);
        MOD100(3);
        LESS100;
        str[16] = '\0';
        return 16;
    }
    if (value < 100000000000000000ull) {
        MOD100(16);
        MOD100(14);
        MOD100(12);
        MOD100(10);
        MOD100(8);
        MOD100(6);
        MOD100(4);
        MOD100(2);
        LESS10;
        str[17] = '\0';
        return 17;
    }
    if (value < 1000000000000000000ull) {
        MOD100(17);
        MOD100(15);
        MOD100(13);
        MOD100(11);
        MOD100(9);
        MOD100(7);
        MOD100(5);
        MOD100(3);
        LESS100;
        str[18] = '\0';
        return 18;
    }
    if (value < 10000000000000000000ull) {
        MOD100(18);
        MOD100(16);
        MOD100(14);
        MOD100(12);
        MOD100(10);
        MOD100(8);
        MOD100(6);
        MOD100(4);
        MOD100(2);
        LESS10;
        str[19] = '\0';
        return 19;
    }

    MOD100(19);
    MOD100(17);
    MOD100(15);
    MOD100(13);
    MOD100(11);
    MOD100(9);
    MOD100(7);
    MOD100(5);
    MOD100(3);
    LESS100;
    str[20] = '\0';
    return 20;

#undef LESS10
#undef LESS100
#undef MOD100
}

#else //linux

int DataConvert::uitoa(uint32_t value, char str[12])
{
    int len  = digits10(value);
    int next = len - 1;
    uint32_t i, temp;
    while (value >= 100) {
        temp    = value / 100;
        i       = (value - temp * 100) << 1;
        value   = temp;

        str[next]       = digits_[i + 1];
        str[next - 1]   = digits_[i];
        next -= 2;
    }

    if (value < 10) {
        str[next] = '0' + value;
    }
    else {
        i = value << 1;
        str[next]       = digits_[i + 1];
        str[next - 1]   = digits_[i];
    }
    str[len] = '\0';
    return len;
}

int DataConvert::ulltoa(uint64_t value, char str[21])
{
    int len  = digits10(value);
    int next = len - 1;
    uint64_t i, temp;
    while (value >= 100) {
        temp    = value / 100;
        i       = (value - temp * 100) << 1;
        value   = temp;

        str[next]       = digits_[i + 1];
        str[next - 1]   = digits_[i];
        next -= 2;
    }

    if (value < 10) {
        str[next] = '0' + value;
    }
    else {
        i = value << 1;
        str[next]       = digits_[i + 1];
        str[next - 1]   = digits_[i];
    }
    str[len] = '\0';
    return len;
}

#endif //linux

ZRSOCKET_NAMESPACE_END
