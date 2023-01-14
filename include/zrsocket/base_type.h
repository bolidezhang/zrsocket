//**********************************************************************
//
// Copyright (C) 2017 Bolide Zhang <bolidezhang@gmail.com>
// All rights reserved.
//
// This copy of zrsoket is licensed to you under the terms described 
// in the LICENSE.txt file included in this distribution.
//
//**********************************************************************

//float128说明
//gcc
// a)long double, sizeof(long double)==16;
// b)但其在linux x86 and x86-64下的有效字节数为10(80-bit x87 floating point type on x86 and x86-64 architectures);
// c)4.7.0以上版本, 有__float128, __float128数学运算函数需加载libquadmath库
//vc
// a)等同于double, sizeof(long double)==8;
// b)__m128(单精度浮点数)/__m128d(双精度浮点数), 不能直接访问, Streaming SIMD Extensions 2(SSE2) instructions intrinsics, is defined in xmmintrin.h

//int128说明
// a)gcc: 4.7.0以上版本，有__int128
// b)vc: __m128i(整数), 不能直接访问, Streaming SIMD Extensions 2(SSE2) instructions intrinsics, is defined in emmintrin.h

// Some compilers (e.g. VC++) benefit significantly from using this. 
// We've measured 3-4% build speed improvements in apps as a result 
#pragma once

#ifndef ZRSOCKET_BASE_TYPE_H
#define ZRSOCKET_BASE_TYPE_H

#include <cstdint>
#include "config.h"

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
    #pragma once
#endif

ZRSOCKET_NAMESPACE_BEGIN

typedef long long           longlong_t;
typedef unsigned char       uchar_t;
typedef unsigned char       byte_t;
typedef unsigned short      ushort_t;
typedef unsigned int        uint_t;
typedef unsigned long       ulong_t;
typedef unsigned long long  ulonglong_t;
typedef float               real32_t;
typedef double              real64_t;
typedef real32_t            float32_t;
typedef real64_t            float64_t;

ZRSOCKET_NAMESPACE_END

#endif
