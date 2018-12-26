//**********************************************************************
//
// Copyright (C) 2017 Bolide Zhang <bolidezhang@gmail.com>
// All rights reserved.
//
// This copy of zrsoket is licensed to you under the terms described 
// in the LICENSE.txt file included in this distribution.
//
//**********************************************************************

//float128˵��
//gcc
// a)long double, sizeof(long double)==16;
// b)������linux x86 and x86-64�µ���Ч�ֽ���Ϊ10(80-bit x87 floating point type on x86 and x86-64 architectures);
// c)4.7.0���ϰ汾, ��__float128, __float128��ѧ���㺯�������libquadmath��
//vc
// a)��ͬ��double, sizeof(long double)==8;
// b)__m128(�����ȸ�����)/__m128d(˫���ȸ�����), ����ֱ�ӷ���, Streaming SIMD Extensions 2(SSE2) instructions intrinsics, is defined in xmmintrin.h

//int128˵��
// a)gcc: 4.7.0���ϰ汾����__int128
// b)vc: __m128i(����), ����ֱ�ӷ���, Streaming SIMD Extensions 2(SSE2) instructions intrinsics, is defined in emmintrin.h

#ifndef ZRSOCKET_BASE_TYPE_H_
#define ZRSOCKET_BASE_TYPE_H_
#include <stdint.h>
#include "config.h"

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
    #pragma once
#endif

ZRSOCKET_BEGIN

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

ZRSOCKET_END

#endif
