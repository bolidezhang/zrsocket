#include "zrsocket/memory.h"
#include "zrsocket/base_type.h"

ZRSOCKET_NAMESPACE_BEGIN

void* Memory::memcpy(void *dest, const void *src, size_t n)
{
#define MOD0                                            \
    do {                                                \
        *dest_64++ = *src_64++;                         \
    } while (0)

#define MOD1                                            \
    do {                                                \
        *((int8_t *)dest_64) = *((int8_t *)src_64);     \
    } while (0)

#define MOD2                                            \
    do {                                                \
        *((int16_t *)dest_64) = *((int16_t *)src_64);   \
    } while (0)

#define MOD3                                            \
    do {                                                \
        int16_t *dest_16 = (int16_t *)dest_64;          \
        int16_t *src_16  = (int16_t *)src_64;           \
        *dest_16++ = *src_16++;                         \
        *((int8_t *)dest_16) = *((int8_t *)src_16);     \
    } while (0)

#define MOD4                                            \
    do {                                                \
        *((int32_t *)dest_64) = *((int32_t *)src_64);   \
    } while (0)

#define MOD5                                            \
    do {                                                \
        int32_t *dest_32 = (int32_t *)dest_64;          \
        int32_t *src_32  = (int32_t *)src_64;           \
        *dest_32++ = *src_32++;                         \
        *((int8_t *)dest_32) = *((int8_t *)src_32);     \
    } while (0)

#define MOD6                                            \
    do {                                                \
        int32_t *dest_32 = (int32_t *)dest_64;          \
        int32_t *src_32  = (int32_t *)src_64;           \
        *dest_32++ = *src_32++;                         \
        *((int16_t *)dest_32) = *((int16_t *)src_32);   \
    } while (0)

#define MOD7                                            \
    do {                                                \
        int32_t *dest_32 = (int32_t *)dest_64;          \
        int32_t *src_32  = (int32_t *)src_64;           \
        *dest_32++ = *src_32++;                         \
        int16_t *dest_16 = (int16_t *)dest_32;          \
        int16_t *src_16  = (int16_t *)src_32;           \
        *dest_16++ = *src_16++;                         \
        *((int8_t *)dest_16) = *((int8_t *)src_16);     \
    } while (0)

    void *ret = dest;

#ifdef ZRSOCKET_OS_WINDOWS

    if (n > 256) {
        return std::memcpy(dest, src, n);
    }

    int64_t *dest_64  = (int64_t *)dest;
    int64_t *src_64   = (int64_t *)src;

    //for (; n > 256; n -= 256)
    //{
    //    MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
    //    MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
    //}

#else   //linux

    int64_t *dest_64  = (int64_t *)dest;
    int64_t *src_64   = (int64_t *)src;

    //方法1
    for (; n > 256; n -= 256) {
        MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
        MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
    }

    //方法2 (经测试, 在某些环境下,性能比方法1差很多)
    //long double特别说明
    //1)linux gcc, sizeof(long double)==16;
    //2)但其在linux x86 and x86-64下的有效字节数为10(80-bit x87 floating point type on x86 and x86-64 architectures)
    //if (n > 260)
    //{
    //    char *dest_8  = (char *)dest;
    //    char *src_8   = (char *)src;
    //    do {
    //        *(long double *)dest_8 = *(long double *)src_8;
    //        dest_8 += 10;
    //        src_8  += 10;
    //        *(long double *)dest_8 = *(long double *)src_8;
    //        dest_8 += 10;
    //        src_8  += 10;
    //        *(long double *)dest_8 = *(long double *)src_8;
    //        dest_8 += 10;
    //        src_8  += 10;
    //        *(long double *)dest_8 = *(long double *)src_8;
    //        dest_8 += 10;
    //        src_8  += 10;
    //        *(long double *)dest_8 = *(long double *)src_8;
    //        dest_8 += 10;
    //        src_8  += 10;
    //        *(long double *)dest_8 = *(long double *)src_8;
    //        dest_8 += 10;
    //        src_8  += 10;
    //        *(long double *)dest_8 = *(long double *)src_8;
    //        dest_8 += 10;
    //        src_8  += 10;
    //        *(long double *)dest_8 = *(long double *)src_8;
    //        dest_8 += 10;
    //        src_8  += 10;
    //        *(long double *)dest_8 = *(long double *)src_8;
    //        dest_8 += 10;
    //        src_8  += 10;
    //        *(long double *)dest_8 = *(long double *)src_8;
    //        dest_8 += 10;
    //        src_8  += 10;
    //        *(long double *)dest_8 = *(long double *)src_8;
    //        dest_8 += 10;
    //        src_8  += 10;
    //        *(long double *)dest_8 = *(long double *)src_8;
    //        dest_8 += 10;
    //        src_8  += 10;
    //        *(long double *)dest_8 = *(long double *)src_8;
    //        dest_8 += 10;
    //        src_8  += 10;
    //        *(long double *)dest_8 = *(long double *)src_8;
    //        dest_8 += 10;
    //        src_8  += 10;
    //        *(long double *)dest_8 = *(long double *)src_8;
    //        dest_8 += 10;
    //        src_8  += 10;
    //        *(long double *)dest_8 = *(long double *)src_8;
    //        dest_8 += 10;
    //        src_8  += 10;
    //        *(long double *)dest_8 = *(long double *)src_8;
    //        dest_8 += 10;
    //        src_8  += 10;
    //        *(long double *)dest_8 = *(long double *)src_8;
    //        dest_8 += 10;
    //        src_8  += 10;
    //        *(long double *)dest_8 = *(long double *)src_8;
    //        dest_8 += 10;
    //        src_8  += 10;
    //        *(long double *)dest_8 = *(long double *)src_8;
    //        dest_8 += 10;
    //        src_8  += 10;
    //        *(long double *)dest_8 = *(long double *)src_8;
    //        dest_8 += 10;
    //        src_8  += 10;
    //        *(long double *)dest_8 = *(long double *)src_8;
    //        dest_8 += 10;
    //        src_8  += 10;
    //        *(long double *)dest_8 = *(long double *)src_8;
    //        dest_8 += 10;
    //        src_8  += 10;
    //        *(long double *)dest_8 = *(long double *)src_8;
    //        dest_8 += 10;
    //        src_8  += 10;
    //        *(long double *)dest_8 = *(long double *)src_8;
    //        dest_8 += 10;
    //        src_8  += 10;
    //        *(long double *)dest_8 = *(long double *)src_8;
    //        dest_8 += 10;
    //        src_8  += 10;
    //        n -= 260;
    //    } while (n > 260);
    //    dest_64 = (int64_t *)dest_8;
    //    src_64  = (int64_t *)src_8;
    //}

#endif  //linux

    switch (n) 
    {
        case 1:
            {
                MOD1;
            }
            break;
        case 2:
            {
                MOD2;
            }
            break;
        case 3:
            {
                MOD3;
            }
            break;
        case 4:
            {
                MOD4;
            }
            break;
        case 5:
            {
                MOD5;
            }
            break;
        case 6:
            {
                MOD6;
            }
            break;
        case 7:
            {
                MOD7;
            }
            break;
        case 8:
            {
                MOD0;
            }
            break;
        case 9:
            {
                MOD0;MOD1;
            }
            break;
        case 10:
            {
                MOD0;MOD2;
            }
            break;
        case 11:
            {
                MOD0;MOD3;
            }
            break;
        case 12:
            {
                MOD0;MOD4;
            }
            break;
        case 13:
            {
                MOD0;MOD5;
            }
            break;
        case 14:
            {
                MOD0;MOD6;
            }
            break;
        case 15:
            {
                MOD0;MOD7;
            }
            break;
        case 16:
            {
                MOD0;MOD0;
            }
            break;
        case 17:
            {
                MOD0;MOD0;MOD1;
            }
            break;
        case 18:
            {
                MOD0;MOD0;MOD2;
            }
            break;
        case 19:
            {
                MOD0;MOD0;MOD3;
            }
            break;
        case 20:
            {
                MOD0;MOD0;MOD4;
            }
            break;
        case 21:
            {
                MOD0;MOD0;MOD5;
            }
            break;
        case 22:
            {
                MOD0;MOD0;MOD6;
            }
            break;
        case 23:
            {
                MOD0;MOD0;MOD7;
            }
            break;
        case 24:
            {
                MOD0;MOD0;MOD0;
            }
            break;
        case 25:
            {
                MOD0;MOD0;MOD0;MOD1;
            }
            break;
        case 26:
            {
                MOD0;MOD0;MOD0;MOD2;
            }
            break;
        case 27:
            {
                MOD0;MOD0;MOD0;MOD3;
            }
            break;
        case 28:
            {
                MOD0;MOD0;MOD0;MOD4;
            }
            break;
        case 29:
            {
                MOD0;MOD0;MOD0;MOD5;
            }
            break;
        case 30:
            {
                MOD0;MOD0;MOD0;MOD6;
            }
            break;
        case 31:
            {
                MOD0;MOD0;MOD0;MOD7;
            }
            break;
        case 32:
            {
                MOD0;MOD0;MOD0;MOD0;
            }
            break;
        case 33:
            {
                MOD0;MOD0;MOD0;MOD0;MOD1;
            }
            break;
        case 34:
            {
                MOD0;MOD0;MOD0;MOD0;MOD2;
            }
            break;
        case 35:
            {
                MOD0;MOD0;MOD0;MOD0;MOD3;
            }
            break;
        case 36:
            {
                MOD0;MOD0;MOD0;MOD0;MOD4;
            }
            break;
        case 37:
            {
                MOD0;MOD0;MOD0;MOD0;MOD5;
            }
            break;
        case 38:
            {
                MOD0;MOD0;MOD0;MOD0;MOD6;
            }
            break;
        case 39:
            {
                MOD0;MOD0;MOD0;MOD0;MOD7;
            }
            break;
        case 40:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;
            }
            break;
        case 41:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD1;
            }
            break;
        case 42:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD2;
            }
            break;
        case 43:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD3;
            }
            break;
        case 44:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD4;
            }
            break;
        case 45:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD5;
            }
            break;
        case 46:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD6;
            }
            break;
        case 47:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD7;
            }
            break;
        case 48:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
            }
            break;
        case 49:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD1;
            }
            break;
        case 50:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD2;
            }
            break;
        case 51:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD3;
            }
            break;
        case 52:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD4;
            }
            break;
        case 53:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD5;
            }
            break;
        case 54:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD6;
            }
            break;
        case 55:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD7;
            }
            break;
        case 56:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
            }
            break;
        case 57:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD1;
            }
            break;
        case 58:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD2;
            }
            break;
        case 59:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD3;
            }
            break;
        case 60:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD4;
            }
            break;
        case 61:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD5;
            }
            break;
        case 62:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD6;
            }
            break;
        case 63:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD7;
            }
            break;
        case 64:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
            }
            break;

        case 65:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD1;
            }
            break;
        case 66:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD2;
            }
            break;
        case 67:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD3;
            }
            break;
        case 68:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD4;
            }
            break;
        case 69:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD5;
            }
            break;
        case 70:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD6;
            }
            break;
        case 71:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD7;
            }
            break;
        case 72:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
            }
            break;
        case 73:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD1;
            }
            break;
        case 74:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD2;
            }
            break;
        case 75:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD3;
            }
            break;
        case 76:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD4;
            }
            break;
        case 77:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD5;
            }
            break;
        case 78:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD6;
            }
            break;
        case 79:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD7;
            }
            break;
        case 80:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
            }
            break;
        case 81:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD1;
            }
            break;
        case 82:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD2;
            }
            break;
        case 83:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD3;
            }
            break;
        case 84:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD4;
            }
            break;
        case 85:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD5;
            }
            break;
        case 86:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD6;
            }
            break;
        case 87:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD7;
            }
            break;
        case 88:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
            }
            break;
        case 89:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD1;
            }
            break;
        case 90:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD2;
            }
            break;
        case 91:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD3;
            }
            break;
        case 92:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD4;
            }
            break;
        case 93:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD5;
            }
            break;
        case 94:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD6;
            }
            break;
        case 95:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD7;
            }
            break;
        case 96:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
            }
            break;
        case 97:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD1;
            }
            break;
        case 98:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD2;
            }
            break;
        case 99:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD3;
            }
            break;
        case 100:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD4;
            }
            break;
        case 101:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD5;
            }
            break;
        case 102:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD6;
            }
            break;
        case 103:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD7;
            }
            break;
        case 104:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
            }
            break;
        case 105:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD1;
            }
            break;
        case 106:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD2;
            }
            break;
        case 107:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD3;
            }
            break;
        case 108:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD4;
            }
            break;
        case 109:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD5;
            }
            break;
        case 110:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD6;
            }
            break;
        case 111:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD7;
            }
            break;
        case 112:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
            }
            break;
        case 113:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD1;
            }
            break;
        case 114:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD2;
            }
            break;
        case 115:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD3;
            }
            break;
        case 116:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD4;
            }
            break;
        case 117:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD5;
            }
            break;
        case 118:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD6;
            }
            break;
        case 119:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD7;
            }
            break;
        case 120:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
            }
            break;
        case 121:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD1;
            }
            break;
        case 122:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD2;
            }
            break;
        case 123:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD3;
            }
            break;
        case 124:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD4;
            }
            break;
        case 125:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD5;
            }
            break;
        case 126:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD6;
            }
            break;
        case 127:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD7;
            }
            break;
        case 128:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
            }
            break;
        case 129:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD1;
            }
            break;
        case 130:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD2;
            }
            break;
        case 131:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD3;
            }
            break;
        case 132:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD4;
            }
            break;
        case 133:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD5;
            }
            break;
        case 134:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD6;
            }
            break;
        case 135:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD7;
            }
            break;
        case 136:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
            }
            break;
        case 137:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD1;
            }
            break;
        case 138:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD2;
            }
            break;
        case 139:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD3;
            }
            break;
        case 140:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD4;
            }
            break;
        case 141:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD5;
            }
            break;
        case 142:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD6;
            }
            break;
        case 143:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD7;
            }
            break;
        case 144:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
            }
            break;
        case 145:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
                MOD0;MOD0;MOD1;
            }
            break;
        case 146:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
                MOD0;MOD0;MOD2;
            }
            break;
        case 147:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
                MOD0;MOD0;MOD3;
            }
            break;
        case 148:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
                MOD0;MOD0;MOD4;
            }
            break;
        case 149:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
                MOD0;MOD0;MOD5;
            }
            break;
        case 150:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
                MOD0;MOD0;MOD6;
            }
            break;
        case 151:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
                MOD0;MOD0;MOD7;
            }
            break;
        case 152:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
                MOD0;MOD0;MOD0;
            }
            break;
        case 153:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
                MOD0;MOD0;MOD0;MOD1;
            }
            break;
        case 154:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
                MOD0;MOD0;MOD0;MOD2;
            }
            break;
        case 155:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
                MOD0;MOD0;MOD0;MOD3;
            }
            break;
        case 156:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
                MOD0;MOD0;MOD0;MOD4;
            }
            break;
        case 157:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
                MOD0;MOD0;MOD0;MOD5;
            }
            break;
        case 158:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
                MOD0;MOD0;MOD0;MOD6;
            }
            break;
        case 159:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
                MOD0;MOD0;MOD0;MOD7;
            }
            break;
        case 160:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
                MOD0;MOD0;MOD0;MOD0;
            }
            break;
        case 161:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
                MOD0;MOD0;MOD0;MOD0;MOD1;
            }
            break;
        case 162:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
                MOD0;MOD0;MOD0;MOD0;MOD2;
            }
            break;
        case 163:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
                MOD0;MOD0;MOD0;MOD0;MOD3;
            }
            break;
        case 164:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
                MOD0;MOD0;MOD0;MOD0;MOD4;
            }
            break;
        case 165:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
                MOD0;MOD0;MOD0;MOD0;MOD5;
            }
            break;
        case 166:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
                MOD0;MOD0;MOD0;MOD0;MOD6;
            }
            break;
        case 167:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
                MOD0;MOD0;MOD0;MOD0;MOD7;
            }
            break;
        case 168:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
                MOD0;MOD0;MOD0;MOD0;MOD0;
            }
            break;
        case 169:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD1;
            }
            break;
        case 170:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD2;
            }
            break;
        case 171:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD3;
            }
            break;
        case 172:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD4;
            }
            break;
        case 173:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD5;
            }
            break;
        case 174:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD6;
            }
            break;
        case 175:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD7;
            }
            break;
        case 176:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
            }
            break;
        case 177:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD1;
            }
            break;
        case 178:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD2;
            }
            break;
        case 179:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD3;
            }
            break;
        case 180:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD4;
            }
            break;
        case 181:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD5;
            }
            break;
        case 182:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD6;
            }
            break;
        case 183:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD7;
            }
            break;
        case 184:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
            }
            break;
        case 185:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD1;
            }
            break;
        case 186:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD2;
            }
            break;
        case 187:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD3;
            }
            break;
        case 188:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD4;
            }
            break;
        case 189:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD5;
            }
            break;
        case 190:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD6;
            }
            break;
        case 191:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD7;
            }
            break;
        case 192:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
            }
            break;
        case 193:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD1;
            }
            break;
        case 194:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD2;
            }
            break;
        case 195:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD3;
            }
            break;
        case 196:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD4;
            }
            break;
        case 197:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD5;
            }
            break;
        case 198:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD6;
            }
            break;
        case 199:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD7;
            }
            break;
        case 200:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
            }
            break;
        case 201:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD1;
            }
            break;
        case 202:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD2;
            }
            break;
        case 203:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD3;
            }
            break;
        case 204:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD4;
            }
            break;
        case 205:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD5;
            }
            break;
        case 206:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD6;
            }
            break;
        case 207:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD7;
            }
            break;
        case 208:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
            }
            break;
        case 209:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD1;
            }
            break;
        case 210:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD2;
            }
            break;
        case 211:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD3;
            }
            break;
        case 212:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD4;
            }
            break;
        case 213:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD5;
            }
            break;
        case 214:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD6;
            }
            break;
        case 215:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD7;
            }
            break;
        case 216:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
            }
            break;
        case 217:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD1;
            }
            break;
        case 218:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD2;
            }
            break;
        case 219:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD3;
            }
            break;
        case 220:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD4;
            }
            break;
        case 221:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD5;
            }
            break;
        case 222:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD6;
            }
            break;
        case 223:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD7;
            }
            break;
        case 224:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
            }
            break;
        case 225:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD1;
            }
            break;
        case 226:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD2;
            }
            break;
        case 227:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD3;
            }
            break;
        case 228:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD4;
            }
            break;
        case 229:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD5;
            }
            break;
        case 230:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD6;
            }
            break;
        case 231:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD7;
            }
            break;
        case 232:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
            }
            break;
        case 233:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD1;
            }
            break;
        case 234:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD2;
            }
            break;
        case 235:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD3;
            }
            break;
        case 236:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD4;
            }
            break;
        case 237:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD5;
            }
            break;
        case 238:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD6;
            }
            break;
        case 239:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD7;
            }
            break;
        case 240:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
            }
            break;
        case 241:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD1;
            }
            break;
        case 242:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD2;
            }
            break;
        case 243:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD3;
            }
            break;
        case 244:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD4;
            }
            break;
        case 245:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD5;
            }
            break;
        case 246:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD6;
            }
            break;
        case 247:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD7;
            }
            break;
        case 248:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
            }
            break;
        case 249:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD1;
            }
            break;
        case 250:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD2;
            }
            break;
        case 251:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD3;
            }
            break;
        case 252:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD4;
            }
            break;
        case 253:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD5;
            }
            break;
        case 254:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD6;
            }
            break;
        case 255:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD7;
            }
            break;
        case 256:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
            }
            break;
    }

    return ret;

#undef MOD0
#undef MOD1
#undef MOD2
#undef MOD3
#undef MOD4
#undef MOD5
#undef MOD6
#undef MOD7

};

void Memory::memclear(void *dest, size_t n)
{
#define MOD0                                        \
    do {                                            \
        *dest_64++ = 0;                             \
    } while (0)

#define MOD1                                        \
    do {                                            \
        *((int8_t *)dest_64) = 0;                   \
    } while (0)

#define MOD2                                        \
    do {                                            \
        *((int16_t *)dest_64) = 0;                  \
    } while (0)

#define MOD3                                        \
    do {                                            \
        int16_t *dest_16 = (int16_t *)dest_64;      \
        *dest_16++ = 0;                             \
        *((int8_t *)dest_16) = 0;                   \
    } while (0)

#define MOD4                                        \
    do {                                            \
        *((int32_t *)dest_64) = 0;                  \
    } while (0)

#define MOD5                                        \
    do {                                            \
        int32_t *dest_32 = (int32_t *)dest_64;      \
        *dest_32++ = 0;                             \
        *((int8_t *)dest_32) = 0;                   \
    } while (0)

#define MOD6                                        \
    do {                                            \
        int32_t *dest_32 = (int32_t *)dest_64;      \
        *dest_32++ = 0;                             \
        *((int16_t *)dest_32) = 0;                  \
    } while (0)

#define MOD7                                        \
    do {                                            \
        int32_t *dest_32 = (int32_t *)dest_64;      \
        *dest_32++ = 0;                             \
        int16_t *dest_16 = (int16_t *)dest_32;      \
        *dest_16++ = 0;                             \
        *((int8_t *)dest_16) = 0;                   \
    } while (0)

#ifdef ZRSOCKET_OS_WINDOWS

    if (n > 256) {
        std::memset(dest, 0, n);
        return;
    }
    int64_t *dest_64 = (int64_t *)dest;

    //for (; n > 256; n -= 256)
    //{
    //    MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
    //    MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
    //}

#else   //linux

    int64_t *dest_64 = (int64_t *)dest;
    for (; n > 256; n -= 256) {
        MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
        MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
    }

#endif  //linux

    switch (n)
    {
        case 1:
            {
                MOD1;
            }
            break;
        case 2:
            {
                MOD2;
            }
            break;
        case 3:
            {
                MOD3;
            }
            break;
        case 4:
            {
                MOD4;
            }
            break;
        case 5:
            {
                MOD5;
            }
            break;
        case 6:
            {
                MOD6;
            }
            break;
        case 7:
            {
                MOD7;
            }
            break;
        case 8:
            {
                MOD0;
            }
            break;
        case 9:
            {
                MOD0;MOD1;
            }
            break;
        case 10:
            {
                MOD0;MOD2;
            }
            break;
        case 11:
            {
                MOD0;MOD3;
            }
            break;
        case 12:
            {
                MOD0;MOD4;
            }
            break;
        case 13:
            {
                MOD0;MOD5;
            }
            break;
        case 14:
            {
                MOD0;MOD6;
            }
            break;
        case 15:
            {
                MOD0;MOD7;
            }
            break;
        case 16:
            {
                MOD0;MOD0;
            }
            break;
        case 17:
            {
                MOD0;MOD0;MOD1;
            }
            break;
        case 18:
            {
                MOD0;MOD0;MOD2;
            }
            break;
        case 19:
            {
                MOD0;MOD0;MOD3;
            }
            break;
        case 20:
            {
                MOD0;MOD0;MOD4;
            }
            break;
        case 21:
            {
                MOD0;MOD0;MOD5;
            }
            break;
        case 22:
            {
                MOD0;MOD0;MOD6;
            }
            break;
        case 23:
            {
                MOD0;MOD0;MOD7;
            }
            break;
        case 24:
            {
                MOD0;MOD0;MOD0;
            }
            break;
        case 25:
            {
                MOD0;MOD0;MOD0;MOD1;
            }
            break;
        case 26:
            {
                MOD0;MOD0;MOD0;MOD2;
            }
            break;
        case 27:
            {
                MOD0;MOD0;MOD0;MOD3;
            }
            break;
        case 28:
            {
                MOD0;MOD0;MOD0;MOD4;
            }
            break;
        case 29:
            {
                MOD0;MOD0;MOD0;MOD5;
            }
            break;
        case 30:
            {
                MOD0;MOD0;MOD0;MOD6;
            }
            break;
        case 31:
            {
                MOD0;MOD0;MOD0;MOD7;
            }
            break;
        case 32:
            {
                MOD0;MOD0;MOD0;MOD0;
            }
            break;
        case 33:
            {
                MOD0;MOD0;MOD0;MOD0;MOD1;
            }
            break;
        case 34:
            {
                MOD0;MOD0;MOD0;MOD0;MOD2;
            }
            break;
        case 35:
            {
                MOD0;MOD0;MOD0;MOD0;MOD3;
            }
            break;
        case 36:
            {
                MOD0;MOD0;MOD0;MOD0;MOD4;
            }
            break;
        case 37:
            {
                MOD0;MOD0;MOD0;MOD0;MOD5;
            }
            break;
        case 38:
            {
                MOD0;MOD0;MOD0;MOD0;MOD6;
            }
            break;
        case 39:
            {
                MOD0;MOD0;MOD0;MOD0;MOD7;
            }
            break;
        case 40:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;
            }
            break;
        case 41:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD1;
            }
            break;
        case 42:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD2;
            }
            break;
        case 43:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD3;
            }
            break;
        case 44:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD4;
            }
            break;
        case 45:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD5;
            }
            break;
        case 46:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD6;
            }
            break;
        case 47:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD7;
            }
            break;
        case 48:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
            }
            break;
        case 49:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD1;
            }
            break;
        case 50:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD2;
            }
            break;
        case 51:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD3;
            }
            break;
        case 52:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD4;
            }
            break;
        case 53:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD5;
            }
            break;
        case 54:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD6;
            }
            break;
        case 55:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD7;
            }
            break;
        case 56:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
            }
            break;
        case 57:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD1;
            }
            break;
        case 58:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD2;
            }
            break;
        case 59:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD3;
            }
            break;
        case 60:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD4;
            }
            break;
        case 61:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD5;
            }
            break;
        case 62:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD6;
            }
            break;
        case 63:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD7;
            }
            break;
        case 64:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
            }
            break;

        case 65:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD1;
            }
            break;
        case 66:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD2;
            }
            break;
        case 67:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD3;
            }
            break;
        case 68:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD4;
            }
            break;
        case 69:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD5;
            }
            break;
        case 70:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD6;
            }
            break;
        case 71:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD7;
            }
            break;
        case 72:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
            }
            break;
        case 73:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD1;
            }
            break;
        case 74:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD2;
            }
            break;
        case 75:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD3;
            }
            break;
        case 76:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD4;
            }
            break;
        case 77:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD5;
            }
            break;
        case 78:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD6;
            }
            break;
        case 79:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD7;
            }
            break;
        case 80:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
            }
            break;
        case 81:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD1;
            }
            break;
        case 82:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD2;
            }
            break;
        case 83:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD3;
            }
            break;
        case 84:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD4;
            }
            break;
        case 85:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD5;
            }
            break;
        case 86:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD6;
            }
            break;
        case 87:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD7;
            }
            break;
        case 88:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
            }
            break;
        case 89:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD1;
            }
            break;
        case 90:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD2;
            }
            break;
        case 91:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD3;
            }
            break;
        case 92:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD4;
            }
            break;
        case 93:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD5;
            }
            break;
        case 94:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD6;
            }
            break;
        case 95:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD7;
            }
            break;
        case 96:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
            }
            break;
        case 97:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD1;
            }
            break;
        case 98:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD2;
            }
            break;
        case 99:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD3;
            }
            break;
        case 100:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD4;
            }
            break;
        case 101:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD5;
            }
            break;
        case 102:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD6;
            }
            break;
        case 103:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD7;
            }
            break;
        case 104:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
            }
            break;
        case 105:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD1;
            }
            break;
        case 106:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD2;
            }
            break;
        case 107:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD3;
            }
            break;
        case 108:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD4;
            }
            break;
        case 109:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD5;
            }
            break;
        case 110:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD6;
            }
            break;
        case 111:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD7;
            }
            break;
        case 112:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
            }
            break;
        case 113:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD1;
            }
            break;
        case 114:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD2;
            }
            break;
        case 115:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD3;
            }
            break;
        case 116:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD4;
            }
            break;
        case 117:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD5;
            }
            break;
        case 118:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD6;
            }
            break;
        case 119:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD7;
            }
            break;
        case 120:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
            }
            break;
        case 121:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD1;
            }
            break;
        case 122:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD2;
            }
            break;
        case 123:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD3;
            }
            break;
        case 124:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD4;
            }
            break;
        case 125:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD5;
            }
            break;
        case 126:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD6;
            }
            break;
        case 127:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD7;
            }
            break;
        case 128:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
            }
            break;
        case 129:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD1;
            }
            break;
        case 130:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD2;
            }
            break;
        case 131:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD3;
            }
            break;
        case 132:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD4;
            }
            break;
        case 133:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD5;
            }
            break;
        case 134:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD6;
            }
            break;
        case 135:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD7;
            }
            break;
        case 136:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
            }
            break;
        case 137:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD1;
            }
            break;
        case 138:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD2;
            }
            break;
        case 139:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD3;
            }
            break;
        case 140:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD4;
            }
            break;
        case 141:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD5;
            }
            break;
        case 142:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD6;
            }
            break;
        case 143:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD7;
            }
            break;
        case 144:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
            }
            break;
        case 145:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
                MOD0;MOD0;MOD1;
            }
            break;
        case 146:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
                MOD0;MOD0;MOD2;
            }
            break;
        case 147:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
                MOD0;MOD0;MOD3;
            }
            break;
        case 148:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
                MOD0;MOD0;MOD4;
            }
            break;
        case 149:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
                MOD0;MOD0;MOD5;
            }
            break;
        case 150:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
                MOD0;MOD0;MOD6;
            }
            break;
        case 151:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
                MOD0;MOD0;MOD7;
            }
            break;
        case 152:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
                MOD0;MOD0;MOD0;
            }
            break;
        case 153:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
                MOD0;MOD0;MOD0;MOD1;
            }
            break;
        case 154:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
                MOD0;MOD0;MOD0;MOD2;
            }
            break;
        case 155:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
                MOD0;MOD0;MOD0;MOD3;
            }
            break;
        case 156:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
                MOD0;MOD0;MOD0;MOD4;
            }
            break;
        case 157:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
                MOD0;MOD0;MOD0;MOD5;
            }
            break;
        case 158:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
                MOD0;MOD0;MOD0;MOD6;
            }
            break;
        case 159:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
                MOD0;MOD0;MOD0;MOD7;
            }
            break;
        case 160:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
                MOD0;MOD0;MOD0;MOD0;
            }
            break;
        case 161:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
                MOD0;MOD0;MOD0;MOD0;MOD1;
            }
            break;
        case 162:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
                MOD0;MOD0;MOD0;MOD0;MOD2;
            }
            break;
        case 163:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
                MOD0;MOD0;MOD0;MOD0;MOD3;
            }
            break;
        case 164:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
                MOD0;MOD0;MOD0;MOD0;MOD4;
            }
            break;
        case 165:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
                MOD0;MOD0;MOD0;MOD0;MOD5;
            }
            break;
        case 166:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
                MOD0;MOD0;MOD0;MOD0;MOD6;
            }
            break;
        case 167:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
                MOD0;MOD0;MOD0;MOD0;MOD7;
            }
            break;
        case 168:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
                MOD0;MOD0;MOD0;MOD0;MOD0;
            }
            break;
        case 169:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD1;
            }
            break;
        case 170:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD2;
            }
            break;
        case 171:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD3;
            }
            break;
        case 172:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD4;
            }
            break;
        case 173:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD5;
            }
            break;
        case 174:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD6;
            }
            break;
        case 175:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD7;
            }
            break;
        case 176:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
            }
            break;
        case 177:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD1;
            }
            break;
        case 178:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD2;
            }
            break;
        case 179:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD3;
            }
            break;
        case 180:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD4;
            }
            break;
        case 181:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD5;
            }
            break;
        case 182:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD6;
            }
            break;
        case 183:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD7;
            }
            break;
        case 184:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
            }
            break;
        case 185:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD1;
            }
            break;
        case 186:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD2;
            }
            break;
        case 187:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD3;
            }
            break;
        case 188:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD4;
            }
            break;
        case 189:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD5;
            }
            break;
        case 190:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD6;
            }
            break;
        case 191:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD7;
            }
            break;
        case 192:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
            }
            break;
        case 193:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD1;
            }
            break;
        case 194:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD2;
            }
            break;
        case 195:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD3;
            }
            break;
        case 196:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD4;
            }
            break;
        case 197:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD5;
            }
            break;
        case 198:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD6;
            }
            break;
        case 199:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD7;
            }
            break;
        case 200:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
            }
            break;
        case 201:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD1;
            }
            break;
        case 202:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD2;
            }
            break;
        case 203:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD3;
            }
            break;
        case 204:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD4;
            }
            break;
        case 205:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD5;
            }
            break;
        case 206:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD6;
            }
            break;
        case 207:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD7;
            }
            break;
        case 208:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
            }
            break;
        case 209:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD1;
            }
            break;
        case 210:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD2;
            }
            break;
        case 211:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD3;
            }
            break;
        case 212:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD4;
            }
            break;
        case 213:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD5;
            }
            break;
        case 214:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD6;
            }
            break;
        case 215:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD7;
            }
            break;
        case 216:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
            }
            break;
        case 217:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD1;
            }
            break;
        case 218:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD2;
            }
            break;
        case 219:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD3;
            }
            break;
        case 220:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD4;
            }
            break;
        case 221:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD5;
            }
            break;
        case 222:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD6;
            }
            break;
        case 223:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD7;
            }
            break;
        case 224:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
            }
            break;
        case 225:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD1;
            }
            break;
        case 226:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD2;
            }
            break;
        case 227:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD3;
            }
            break;
        case 228:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD4;
            }
            break;
        case 229:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD5;
            }
            break;
        case 230:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD6;
            }
            break;
        case 231:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD7;
            }
            break;
        case 232:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
            }
            break;
        case 233:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD1;
            }
            break;
        case 234:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD2;
            }
            break;
        case 235:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD3;
            }
            break;
        case 236:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD4;
            }
            break;
        case 237:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD5;
            }
            break;
        case 238:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD6;
            }
            break;
        case 239:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD7;
            }
            break;
        case 240:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
            }
            break;
        case 241:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD1;
            }
            break;
        case 242:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD2;
            }
            break;
        case 243:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD3;
            }
            break;
        case 244:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD4;
            }
            break;
        case 245:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD5;
            }
            break;
        case 246:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD6;
            }
            break;
        case 247:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD7;
            }
            break;
        case 248:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
            }
            break;
        case 249:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD1;
            }
            break;
        case 250:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD2;
            }
            break;
        case 251:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD3;
            }
            break;
        case 252:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD4;
            }
            break;
        case 253:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD5;
            }
            break;
        case 254:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD6;
            }
            break;
        case 255:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD7;
            }
            break;
        case 256:
            {
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
                MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;MOD0;
            }
            break;
    }

#undef MOD0
#undef MOD1
#undef MOD2
#undef MOD3
#undef MOD4
#undef MOD5
#undef MOD6
#undef MOD7

}

ZRSOCKET_NAMESPACE_END
