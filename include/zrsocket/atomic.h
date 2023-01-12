// Some compilers (e.g. VC++) benefit significantly from using this. 
// We've measured 3-4% build speed improvements in apps as a result 
#pragma once

#ifndef ZRSOCKET_ATOMIC_H
#define ZRSOCKET_ATOMIC_H
#include <atomic>
#include "config.h"
#include "base_type.h"

ZRSOCKET_NAMESPACE_BEGIN

typedef std::atomic_flag        AtomicFlag;
typedef std::atomic_bool        AtomicBool;
typedef std::atomic_char        AtomicChar;
typedef std::atomic_uchar       AtomicUChar;
typedef std::atomic_short       AtomicShort;
typedef std::atomic_ushort      AtomicUShort;
typedef std::atomic_int         AtomicInt;
typedef std::atomic_uint        AtomicUInt;
typedef std::atomic_long        AtomicLong;
typedef std::atomic_ulong       AtomicULong;
typedef std::atomic_llong       AtomicLLong;
typedef std::atomic_ullong      AtomicULLong;
typedef std::atomic_wchar_t     AtomicWChar;
typedef std::atomic_char16_t    AtomicChar16;
typedef std::atomic_char32_t    AtomicChar32;

typedef std::atomic_int8_t      AtomicInt8;
typedef std::atomic_uint8_t     AtomicUint8;
typedef std::atomic_int16_t     AtomicInt16;
typedef std::atomic_uint16_t    AtomicUInt16;
typedef std::atomic_int32_t     AtomicInt32;
typedef std::atomic_uint32_t    AtomicUInt32;
typedef std::atomic_int64_t     AtomicInt64;
typedef std::atomic_uint64_t    AtomicUInt64;

ZRSOCKET_NAMESPACE_END

#endif
