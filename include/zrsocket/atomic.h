#ifndef ZRSOCKET_ATOMIC_H_
#define ZRSOCKET_ATOMIC_H_
#include <atomic>
#include "config.h"
#include "base_type.h"

ZRSOCKET_BEGIN

typedef std::atomic<char>        AtomicChar;
typedef std::atomic<uchar_t>     AtomicUChar;
typedef std::atomic<short>       AtomicShort;
typedef std::atomic<ushort_t>    AtomicUShort;
typedef std::atomic<int>         AtomicInt;
typedef std::atomic<uint_t>      AtomicUInt;
typedef std::atomic<long>        AtomicLong;
typedef std::atomic<ulong_t>     AtomicULong;
typedef std::atomic<longlong_t>  AtomicLLong;
typedef std::atomic<ulonglong_t> AtomicULLong;

typedef std::atomic<int8_t>      AtomicInt8;
typedef std::atomic<uint8_t>     AtomicUint8;
typedef std::atomic<int16_t>     AtomicInt16;
typedef std::atomic<uint16_t>    AtomicUInt16;
typedef std::atomic<int32_t>     AtomicInt32;
typedef std::atomic<uint32_t>    AtomicUInt32;
typedef std::atomic<int64_t>     AtomicInt64;
typedef std::atomic<uint64_t>    AtomicUInt64;

ZRSOCKET_END

#endif
