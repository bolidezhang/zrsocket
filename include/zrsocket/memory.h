#ifndef ZRSOCKET_MEMORY_H_
#define ZRSOCKET_MEMORY_H_
#include <cstring>
#include "config.h"

ZRSOCKET_BEGIN

class ZRSOCKET_EXPORT Memory
{
public:
    //�ڴ濽��
    //������:  window7-32 vc2010 cpu: 4*i5-2310 memory: 4G:
    //              1)С���ڴ�ʱ, ��8�ֽ�������ֵ,������������;
    //              2)����ڴ�ʱ, ��8�ֽ�������ֵ,���ܷ����½�;�����ڴ�����256ʱ,ֱ�ӵ���׼��memcpy.
    //         linux:
    //              1)���ܿ��ܺ�Щ,Ҳ���ܲ�Щ(ʹ��ǰ,�������л�������ϸ����)
    //              2)centos6.3      x86-64 kernel:2.6.32 gcc:4.4.6 cpu:4*i5-2310  memory:4G 8�ֽ�������ֵ ������������
    //              3)centos7.0.1406 x86-64 kernel:3.10.0 gcc:4.8.2 cpu:4*i5-3330S memory:8G 8�ֽ�������ֵ �� ��׼memcpy �����Բ�.
    static void* memcpy(void *dest, const void *src, size_t n);

    //�ڴ�����
    static void memclear(void *dest, size_t n);

    //�ڴ�Ƚ�
    static inline int memcmp(const void *s1, const void *s2, size_t n)
    {
        return std::memcmp(s1, s2, n);
    }

    //�ڴ���ֵ
    //��һ��ʵ�ַ�ʽ: Ԥ�ȼ���  8�ֽ�������ֵ(ÿ�ֽڶ���������c����ֵ)
    //                          4�ֽ�������ֵ(ÿ�ֽڶ���������c����ֵ)
    //                          2�ֽ�������ֵ(ÿ�ֽڶ���������c����ֵ)
    static inline void* memset(void *dest, int c, size_t n)
    {
        return std::memset(dest, c, n);
    }

    static inline void* memmove(void *dest, const void *src, size_t n)
    {
        return std::memmove(dest, src, n);
    }

private:
    Memory()
    {
    }

    ~Memory()
    {
    }
};

#ifdef ZRSOCKET_OS_WINDOWS
    #define zrsocket_memcpy(dest, src, n)   ZRSOCKET::Memory::memcpy(dest, src, n)
    #define zrsocket_memclear(dest, n)      ZRSOCKET::Memory::memclear(dest, n)
    #define zrsocket_memcmp(s1, s2, n)      std::memcmp(s1, s2, n)
    #define zrsocket_memset(dest, c, n)     std::memset(dest, c, n)
    #define zrsocket_memmove(dest, src, n)  std::memmove(dest, src, n)
#else
    //��zrsocket::memcpy��ios�¿��ܻ����,
    //�����ڷ�windows����Ҫ�����ZRSOCKET_USE_CUSTOM_MEMCPY������zrsocket_memcpyʵ��Ϊzrsocket::memcpy
    #ifdef ZRSOCKET_USE_CUSTOM_MEMCPY
        #define zrsocket_memcpy(dest, src, n) ZRSOCKET::Memory::memcpy(dest, src, n)
        #define zrsocket_memclear(dest, n)    ZRSOCKET::Memory::memclear(dest, n)
    #else
        #define zrsocket_memcpy(dest, src, n) std::memcpy(dest, src, n)
        #define zrsocket_memclear(dest, n)    std::memset(dest, 0, n)
    #endif

    #define zrsocket_memcmp(s1, s2, n)        std::memcmp(s1, s2, n)
    #define zrsocket_memset(dest, c, n)       std::memset(dest, c, n)
    #define zrsocket_memmove(dest, src, n)    std::memmove(dest, src, n)
#endif

ZRSOCKET_END

#endif
