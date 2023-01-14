// Some compilers (e.g. VC++) benefit significantly from using this. 
// We've measured 3-4% build speed improvements in apps as a result 
#pragma once

#ifndef ZRSOCKET_MALLOC_H
#define ZRSOCKET_MALLOC_H

#include <cstdlib>
#include <cstring>
#include "config.h"

//比较优秀malloc的实现
//memory allocation: malloc implement
//1.jemalloc freebsd/facebook/firefox
//#define ZRSOCKET_USE_JEMALLOC
//2.google tcmalloc
//#define ZRSOCKET_USE_TCMALLOC
//3.hoard - The Hoard Memory Allocator
//#define ZRSOCKET_MALLOC_HOARD
//4.nedmalloc - ned Productions dlmalloc
//#define ZRSOCKET_USE_NEDMALLOC
//5.Lockless malloc - Lockless Inc. Low level software to optimize performance
//#define ZRSOCKET_USE_LOCKLESS
//6.intel tbbmalloc
//#define ZRSOCKET_USE_TBBMALLOC
//7.ptmalloc - a) linux glibc default malloc b) 起源于 Doug Lea 的 dlmalloc
//#define ZRSOCKET_USE_PTMALLOC
//8.dlmalloc - Doug Lea dlmalloc
//#define ZRSOCKET_USE_DLMALLOC

#if defined(ZRSOCKET_USE_JEMALLOC)
    //#define JEMALLOC_NO_DEMANGLE
    #include <jemalloc/jemalloc.h>

    //重新定义内存分配器
    #define zrsocket_malloc(size) je_malloc(size)
    #define zrsocket_calloc(count, size) je_calloc(count, size)
    #define zrsocket_realloc(ptr, size) je_realloc(ptr, size)
    #define zrsocket_free(ptr) je_free(ptr)

    //替换运行时库的默认内存分配器
    #define malloc(size) je_malloc(size)
    #define calloc(count, size) je_calloc(count, size)
    #define realloc(ptr, size) je_realloc(ptr, size)
    #define free(ptr) je_free(ptr)
#elif defined(ZRSOCKET_USE_TCMALLOC)
    #include <google/tcmalloc.h>

    //重新定义内存分配器
    #define zrsocket_malloc(size) tc_malloc(size)
    #define zrsocket_calloc(count, size) tc_calloc(count, size)
    #define zrsocket_realloc(ptr, size) tc_realloc(ptr, size)
    #define zrsocket_free(ptr) tc_free(ptr)

    //替换运行时库的默认内存分配器
    #define malloc(size) tc_malloc(size)
    #define calloc(count, size) tc_calloc(count, size)
    #define realloc(ptr, size) tc_realloc(ptr, size)
    #define free(ptr) tc_free(ptr)
#else
    //重新定义内存分配器
    #define zrsocket_malloc(size) ::malloc(size)
    #define zrsocket_calloc(count, size) ::calloc(count, size)
    #define zrsocket_realloc(ptr, size) ::realloc(ptr, size)
    #define zrsocket_free(ptr) ::free(ptr)
#endif

//默认的内存块大小，经验值
#define ZRSOCKET_DEFAULT_MEMORY_BLOCK_SIZE 8192

#endif
