﻿// Some compilers (e.g. VC++) benefit significantly from using this. 
// We've measured 3-4% build speed improvements in apps as a result 
#pragma once

#ifndef ZRSOCKET_DATA_CONVERT_H
#define ZRSOCKET_DATA_CONVERT_H
#include "config.h"
#include "base_type.h"

ZRSOCKET_NAMESPACE_BEGIN

class ZRSOCKET_EXPORT DataConvert
{
public:
    //字符串转4字节整数
    static int32_t   atoi(const char *str);
    static int32_t   atoi(const char *str, uint_t len, char **endptr);
    //字符串转4字节无符号整数
    static uint32_t  atoui(const char *str);
    static uint32_t  atoui(const char *str, uint_t len, char **endptr);
    //字符串转8字节整数
    static int64_t   atoll(const char *str);
    static int64_t   atoll(const char *str, uint_t len, char **endptr);
    //字符串转8字节无符号整数
    static uint64_t  atoull(const char *str);
    static uint64_t  atoull(const char *str, uint_t len, char **endptr);

    //4字节无符号整数转10进制字串
    static int uitoa(uint32_t value, char str[12]);

    //4字节整数转10进制字串
    static inline int itoa(int32_t value, char str[12])
    {
        if (value >= 0) {
            return uitoa(value, str);
        }
        else {
            *str++ = '-';
            return uitoa(-value, str) + 1;
        }
    }

    //8字节无符号整数转10进制字串
    static int ulltoa(uint64_t value, char str[21]);

    //8字节整数转10进制字串
    static inline int lltoa(int64_t value, char str[21])
    {
        if (value >= 0) {
            return ulltoa(value, str);
        }
        else {
            *str++ = '-';
            return ulltoa(-value, str) + 1;
        }
    }

    //计算整数转为10进制字符串的长度
    static inline int digits10(uint32_t value)
    {
        if (value < 10)
            return 1;
        if (value < 100)
            return 2;
        if (value < 1000)
            return 3;
        if (value < 10000)
            return 4;
        if (value < 100000)
            return 5;
        if (value < 1000000)
            return 6;
        if (value < 10000000)
            return 7;
        if (value < 100000000)
            return 8;
        if (value < 1000000000)
            return 9;
        return 10;
    }

    //计算整数转为10进制字符串的长度
    static inline int digits10(uint64_t value)
    {
        if (value < 10ull)
            return 1;
        if (value < 100ull)
            return 2;
        if (value < 1000ull)
            return 3;
        if (value < 10000ull)
            return 4;
        if (value < 100000ull)
            return 5;
        if (value < 1000000ull)
            return 6;
        if (value < 10000000ull)
            return 7;
        if (value < 100000000ull)
            return 8;
        if (value < 1000000000ull)
            return 9;
        if (value < 10000000000ull)
            return 10;
        if (value < 100000000000ull)
            return 11;
        if (value < 1000000000000ull)
            return 12;
        if (value < 10000000000000ull)
            return 13;
        if (value < 100000000000000ull)
            return 14;
        if (value < 1000000000000000ull)
            return 15;
        if (value < 10000000000000000ull)
            return 16;
        if (value < 100000000000000000ull)
            return 17;
        if (value < 1000000000000000000ull)
            return 18;
        if (value < 10000000000000000000ull)
            return 19;
        return 20;
    }

private:
    DataConvert()
    {
    }

    ~DataConvert()
    {
    }

    static char digits_[201];
};

ZRSOCKET_NAMESPACE_END

#endif
