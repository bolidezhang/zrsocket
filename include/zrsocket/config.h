//**********************************************************************
//
// Copyright (C) 2005-2007 Bolide Zhang(bolidezhang@gmail.com)
// All rights reserved.
//
// This copy of zrsoket is licensed to you under the terms described 
// in the LICENSE.txt file included in this distribution.
//
//**********************************************************************

// Some compilers (e.g. VC++) benefit significantly from using this. 
// We've measured 3-4% build speed improvements in apps as a result 
#pragma once

#ifndef ZRSOCKET_CONFIG_H
#define ZRSOCKET_CONFIG_H

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
    #pragma once
#endif

#ifdef _DEBUG
    #define ZRSOCKET_DEBUG
#else
    #define ZRSOCKET_NDEBUG
#endif

#if !defined(ZRSOCKET_SHARED_LIBRARY)
    #undef ZRSOCKET_STATIC
    #define ZRSOCKET_STATIC
#endif

//os type
//#define ZRSOCKET_OS_WINDOWS
//#define ZRSOCKET_OS_LINUX
//#define ZRSOCKET_OS_FREEBSD
//#define ZRSOCKET_OS_TYPE

//c++ compiler type
//#define ZRSOCKET_CC_MSVC
//#define ZRSOCKET_CC_GCC
//#define ZRSOCKET_CC_LLVM_CLANG
//#define ZRSOCKET_CC_INTEL
//#define ZRSOCKET_CC_TYPE

//C 标准
//1.C89
//#define ZRSOCKET_C_STD_1989
//2.C90
//#define ZRSOCKET_C_STD_1990
//3.C99
//#define ZRSOCKET_C_STD_1999
//4.C11
//#define ZRSOCKET_C_STD_2011
//5.C14
//#define ZRSOCKET_C_STD_2014
//6.C17
//#define ZRSOCKET_C_STD_2017

//C++ 标准
//1.C++98
//#define ZRSOCKET_CPLUSPLUS_STD_1998
//2.C++03
//#define ZRSOCKET_CPLUSPLUS_STD_2003
//3.C++TR1
//#define ZRSOCKET_CPLUSPLUS_STD_TR1
//4.C++TR2
//#define ZRSOCKET_CPLUSPLUS_STD_TR2
//5.C++11
//#define ZRSOCKET_CPLUSPLUS_STD_2011
//6.C++14
//#define ZRSOCKET_CPLUSPLUS_STD_2014
//7.C++17
//#define ZRSOCKET_CPLUSPLUS_STD_2017

//STL 实现提供商
//#define ZRSOCKET_STL_GNU_LIBSTDC++
//#define ZRSOCKET_STL_MS
//#define ZRSOCKET_STL_LLVM_LIBC++
//#define ZRSOCKET_STL_STLPORT
//#define ZRSOCKET_STL_EASTL
//#define ZRSOCKET_STL_SGI
//#define ZRSOCKET_STL_APACHE_STDCXX

//STL Extension
#define ZRSOCKET_USE_STLEXTENSION

//字节序相关
//#define ZRSOCKET_BIG_ENDIAN
//#define ZRSOCKET_LITTLE_ENDIAN
//#define ZRSOCKET_BYTE_ORDER
#define __ZRSOCKET_LITTLE_ENDIAN 1
#define __ZRSOCKET_BIG_ENDIAN    2
#if defined(ZRSOCKET_LITTLE_ENDIAN)
    #define ZRSOCKETLITE_BYTE_ORDER __ZRSOCKET_LITTLE_ENDIAN
#endif
#if defined(SOCKETLITE_BIG_ENDIAN)
    #define SOCKETLITE_BYTE_ORDER __ZRSOCKET_BIG_ENDIAN
#endif
#ifndef ZRSOCKET_BYTE_ORDER
    #define ZRSOCKET_BYTE_ORDER __ZRSOCKET_LITTLE_ENDIAN
#endif

#if defined(_WIN32) || defined(_WIN64) || defined(WIN32)
    #include "config/windows.h"
#else
    #include "config/linux.h"
#endif

#define ZRSOCKET_NAMESPACE_BEGIN namespace zrsocket {
#define ZRSOCKET_NAMESPACE_END }
#define ZRSOCKET zrsocket

#define ZRSOCKET_MAX(a,b)   (((a) > (b)) ? (a) : (b))
#define ZRSOCKET_MIN(a,b)   (((a) < (b)) ? (a) : (b))

#endif
