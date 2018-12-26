#ifndef ZRSOCKET_MALLOC_H_
#define ZRSOCKET_MALLOC_H_

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
    #pragma once
#endif
#include <stdlib.h>
#include <string.h>
#include "config.h"

//�Ƚ�����malloc��ʵ��
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
//7.ptmalloc - a) linux glibc default malloc b) ��Դ�� Doug Lea �� dlmalloc
//#define ZRSOCKET_USE_PTMALLOC
//8.dlmalloc - Doug Lea dlmalloc
//#define ZRSOCKET_USE_DLMALLOC

#if defined(ZRSOCKET_USE_JEMALLOC)
    #define JEMALLOC_NO_DEMANGLE
    #include <jemalloc/jemalloc.h>

    //���¶����ڴ������
    #define zrsocket_malloc(size) je_malloc(size)
    #define zrsocket_calloc(count, size) je_calloc(count, size)
    #define zrsocket_realloc(ptr, size) je_realloc(ptr, size)
    #define zrsocket_free(ptr) je_free(ptr)

    //�滻����ʱ���Ĭ���ڴ������
    #define malloc(size) je_malloc(size)
    #define calloc(count, size) je_calloc(count, size)
    #define realloc(ptr, size) je_realloc(ptr, size)
    #define free(ptr) je_free(ptr)
#elif defined(ZRSOCKET_USE_TCMALLOC)
    #include <google/tcmalloc.h>

    //���¶����ڴ������
    #define zrsocket_malloc(size) tc_malloc(size)
    #define zrsocket_calloc(count, size) tc_calloc(count, size)
    #define zrsocket_realloc(ptr, size) tc_realloc(ptr, size)
    #define zrsocket_free(ptr) tc_free(ptr)

    //�滻����ʱ���Ĭ���ڴ������
    #define malloc(size) tc_malloc(size)
    #define calloc(count, size) tc_calloc(count, size)
    #define realloc(ptr, size) tc_realloc(ptr, size)
    #define free(ptr) tc_free(ptr)
#else
    //���¶����ڴ������
    #define zrsocket_malloc(size) ::malloc(size)
    #define zrsocket_calloc(count, size) ::calloc(count, size)
    #define zrsocket_realloc(ptr, size) ::realloc(ptr, size)
    #define zrsocket_free(ptr) ::free(ptr)
#endif

//Ĭ�ϵ��ڴ���С������ֵ
#define ZRSOCKET_DEFAULT_MEMORY_BLOCK_SIZE  8192

#endif
