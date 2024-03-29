﻿// Some compilers (e.g. VC++) benefit significantly from using this. 
// We've measured 3-4% build speed improvements in apps as a result 
#pragma once

#ifndef ZRSOCKET_OS_API_H
#define ZRSOCKET_OS_API_H

#include <ctime>
#include <cstdio>
#include <cstdarg>         // for va_list, va_start, va_end
#include <thread>
#include <functional>
#include <sstream>

#include "config.h"
#include "base_type.h"
#include "memory.h"
#include "data_convert.h"

#ifdef ZRSOCKET_OS_WINDOWS
    #include <io.h>
    #include <Shlwapi.h>
    #pragma comment(lib, "shlwapi.lib")

    /* timeb.h is actually xsi legacy functionality */
    #include <sys/timeb.h>
    #include <xtimec.h>

    //windows多媒体库(主要使用其中timeGetTime())
    #include <MMSystem.h>
    #pragma comment(lib, "Winmm.lib")
#else
    #include <sys/time.h>
    #include <sys/ioctl.h>
    #include <byteswap.h>
#endif

ZRSOCKET_NAMESPACE_BEGIN

//小端字节序 : 整数0x12345678 字节内容 78 56 34 12 与内存地址顺序一致(低字节 存在 低内存地址) 
//             x86体系结构上的字节序为 小端字节序
//大端字节序 : 整数0x12345678 字节内容 12 34 56 78 与内存地址顺序相反(低字节 存在 高内存地址)
//             整数的网络字节序     为 大端字节序
enum class ByteOrder
{
    BYTE_ORDER_UNKNOWN_ENDIAN   = 0,    //未知
    BYTE_ORDER_LITTLE_ENDIAN    = 1,    //小端字节序
    BYTE_ORDER_BIG_ENDIAN       = 2,    //大端字节序
};

class ZRSOCKET_EXPORT OSApi
{  
public:
    //socket_init
    //参数timer_resolution含义:
    //  1) =0 自动设置OS允许定时精度的最小值
    //  2) >0手动设置定时精度(精度范围: 1-5 ms)
    static int socket_init(int version_high, int version_low, int timer_resolution_ms = 0)
    {
        #ifdef ZRSOCKET_OS_WINDOWS
            //initialize winsock
            WSADATA wsaData;
            WORD version_requested = MAKEWORD(version_high, version_low);
            int ret  = WSAStartup(version_requested, &wsaData);
            if (ret == 0) {
                return 0;

                //LPFN_ACCEPTEX lpfnAcceptEx = NULL;
                //GUID GuidAcceptEx = WSAID_ACCEPTEX;
                //// Load the AcceptEx function into memory using WSAIoctl.
                // // The WSAIoctl function is an extension of the ioctlsocket()
                // // function that can use overlapped I/O. The function's 3rd
                // // through 6th parameters are input and output buffers where
                // // we pass the pointer to our AcceptEx function. This is used
                // // so that we can call the AcceptEx function directly, rather
                // // than refer to the Mswsock.lib library.
                //iResult = WSAIoctl(ListenSocket, SIO_GET_EXTENSION_FUNCTION_POINTER,
                //    &GuidAcceptEx, sizeof(GuidAcceptEx),
                //    &lpfnAcceptEx, sizeof(lpfnAcceptEx),
                //    &dwBytes, NULL, NULL);
            }
            else {
                return ret;
            }
        #else
            return 1;
        #endif
    }

    static int socket_fini()
    {
        #ifdef ZRSOCKET_OS_WINDOWS
            return WSACleanup();
        #else
            return 1;
        #endif
    }

    static inline int socket_get_lasterror()
    {
        #ifdef ZRSOCKET_OS_WINDOWS
            return WSAGetLastError();
        #else
            return errno;
        #endif
    }

    static inline int socket_get_lasterror_wsa()
    {
        return socket_get_lasterror();
    }

    static inline int socket_get_error(ZRSOCKET_SOCKET fd)
    {
        int socket_error = 0;
        int socket_error_len = sizeof(int);
        if (getsockopt(fd, SOL_SOCKET, SO_ERROR, (char *)&socket_error, (socklen_t *)&socket_error_len) < 0) {
            return -1;
        }
        return socket_error;
    }

    static inline ZRSOCKET_SOCKET socket_open(int address_family, int type, int protocol)
    {
        return socket(address_family, type, protocol);
    }

    static inline int socket_close(ZRSOCKET_SOCKET fd)
    {
        if ((fd < 0) || (ZRSOCKET_INVALID_SOCKET == fd)) {
            return -1;
        }
        #ifdef ZRSOCKET_OS_WINDOWS
            return closesocket(fd);
        #else
            return close(fd);
        #endif
    }

    static inline int socket_shutdown(ZRSOCKET_SOCKET fd, int how)
    {
        if ((fd < 0) || (ZRSOCKET_INVALID_SOCKET == fd)) {
            return -1;
        }
        return shutdown(fd, how);
    }

    static inline ZRSOCKET_SOCKET socket_accept(ZRSOCKET_SOCKET fd, struct sockaddr *addr, int *addrlen, int flags = 0)
    {
        #ifdef ZRSOCKET_OS_WINDOWS
            return WSAAccept(fd, addr, addrlen, nullptr, 0);
        #else
            #ifdef ZRSOCKET_HAVE_ACCEPT4
                return accept4(fd, addr, (socklen_t *)addrlen, flags);
            #else
                return accept(fd, addr, (socklen_t *)addrlen);
            #endif
        #endif
    }

    static inline int socket_bind(ZRSOCKET_SOCKET fd, const struct sockaddr *addr, int addrlen)
    {
        return bind(fd, addr, addrlen);
    }

    static inline int socket_listen(ZRSOCKET_SOCKET fd, int backlog)
    {
        return listen(fd, backlog);
    }

    static inline int socket_connect(ZRSOCKET_SOCKET fd, const struct sockaddr *addr, int addrlen)
    {
        return connect(fd, addr, addrlen);
    }

    static inline int socket_set_block(ZRSOCKET_SOCKET fd, bool block)
    {
        #ifdef ZRSOCKET_OS_WINDOWS
            unsigned long arg = (block ? 0:1);
            return ioctlsocket(fd, FIONBIO, &arg);
        #else
            //方法1
            int flags = fcntl(fd, F_GETFL);
            if (block) {
                flags &= ~O_NONBLOCK;
            }
            else {
                flags |= O_NONBLOCK;
            }
            if (fcntl(fd, F_SETFL, flags) == -1) {
                return -1;
            }
            return 0;

            //方法2
            //int arg = (block ? 0:1);
            //return ::ioctl(fd, FIONBIO, &arg);
        #endif
    }

    static inline int socket_set_tcpnodelay(ZRSOCKET_SOCKET fd, int flag)
    {
        return setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (char *)&flag, sizeof(int));
    }

    static inline int socket_set_keepalive(ZRSOCKET_SOCKET fd, int flag)
    {
        return setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, (char *)&flag, sizeof(int));
    }

    static inline int socket_set_keepalivevalue(ZRSOCKET_SOCKET fd, ulong_t keepalivetime = 7200000, ulong_t keepaliveinterval = 1000)
    {
        #ifdef ZRSOCKET_OS_WINDOWS 
            #if (_WIN32_WINNT>=0x0500)    //window 2000 or later version
                    tcp_keepalive in_keepalive  = {0};
                    tcp_keepalive out_keepalive = {0};
                    ulong_t inlen  = sizeof(tcp_keepalive);
                    ulong_t outlen = sizeof(tcp_keepalive);
                    ulong_t bytesreturn = 0;

                    //设置socket的keepalivetime,keepaliveinterval
                    in_keepalive.onoff = 1;
                    in_keepalive.keepalivetime = keepalivetime;
                    in_keepalive.keepaliveinterval = keepaliveinterval;

                    //为选定的SOCKET设置Keep Alive，成功后SOCKET可通过KeepAlive自动检测连接是否断开 
                    if ( SOCKET_ERROR == WSAIoctl(fd, SIO_KEEPALIVE_VALS,(LPVOID)&in_keepalive, inlen,(LPVOID)&out_keepalive, outlen, &bytesreturn, nullptr, nullptr) ) {
                        return -1;
                    }
            #endif
        #elif defined(SOCKETLITE_OS_LINUX)
                ///* set first test time */ 
                //setsockopt(fd, SOL_TCP, TCP_KEEPIDLE , (const char *)&iIdleTime , sizeof(iIdleTime)); 
                ///* set test idle time */ 
                //setsockopt(fd, SOL_TCP, TCP_KEEPINTVL, (const char *)&iInterval, sizeof(iInterval)); 
                ///* set test count */ 
                //setsockopt(fd, SOL_TCP, TCP_KEEPCNT, (const char *)&iCount, sizeof(iCount)); 
        #endif
        return 0;
    }

    static inline int socket_set_reuseaddr(ZRSOCKET_SOCKET fd, int flag)
    {
        return setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (char *)&flag, sizeof(int));
    }

    static inline int socket_set_reuseport(ZRSOCKET_SOCKET fd, int flag)
    {
#ifndef ZRSOCKET_OS_WINDOWS 
        return setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, (char *)&flag, sizeof(int));
#else
        return 0;
#endif
    }

    static inline int socket_set_broadcast(ZRSOCKET_SOCKET fd, int flag)
    {
        return setsockopt(fd, SOL_SOCKET, SO_BROADCAST, (char *)&flag, sizeof(int));
    }

    static inline int socket_set_send_buffer_size(ZRSOCKET_SOCKET fd, int sz)
    {
        return setsockopt(fd, SOL_SOCKET, SO_SNDBUF, (char *)&sz, sizeof(int));
    }

    static inline int socket_get_send_buffer_size(ZRSOCKET_SOCKET fd)
    {
        int sz  = 0;
        int len = sizeof(int);
        if (getsockopt(fd, SOL_SOCKET, SO_SNDBUF, (char *)&sz, (socklen_t *)&len) == ZRSOCKET_SOCKET_ERROR) {
            return -1;
        }
        return sz;
    }

    static inline int socket_get_recv_buffer_size(ZRSOCKET_SOCKET fd)
    {
        int sz  = 0;
        int len = sizeof(int);
        if (getsockopt(fd, SOL_SOCKET, SO_RCVBUF, (char *)&sz, (socklen_t *)&len) == ZRSOCKET_SOCKET_ERROR) {
            return -1;
        }
        return sz;
    }

    static inline int socket_set_recv_buffer_size(ZRSOCKET_SOCKET fd, int sz)
    {
        return setsockopt(fd, SOL_SOCKET, SO_RCVBUF, (char *)&sz, sizeof(int));
    }

    static inline int socket_set_recv_timeout(ZRSOCKET_SOCKET fd, int timeout)
    {
        return setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(int));
    }

    static inline int socket_set_send_timeout(ZRSOCKET_SOCKET fd, int timeout)
    {
        return setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeout, sizeof(int));
    }

    static inline int socket_set_linger(ZRSOCKET_SOCKET fd, int onoff, int linger_time)
    {
        linger l;
        l.l_onoff  = onoff;
        l.l_linger = linger_time;
        return setsockopt(fd, SOL_SOCKET, SO_LINGER, reinterpret_cast<char *>(&l), sizeof(l));
    }

    static inline int socket_set_dontlinger(ZRSOCKET_SOCKET fd, int onoff)
    {
        #ifdef ZRSOCKET_OS_WINDOWS
            return  setsockopt(fd, SOL_SOCKET, SO_DONTLINGER, reinterpret_cast<char *>(&onoff), sizeof(int));
        #else
            return 0;
        #endif
    }

    static inline int socket_recv(ZRSOCKET_SOCKET fd, char *buf, uint_t len, int flags, ZRSOCKET_OVERLAPPED *overlapped, int &error)
    {
        #ifdef ZRSOCKET_OS_WINDOWS
            long recv_bytes = 0;
            WSABUF wsabuf;
            wsabuf.len = len;
            wsabuf.buf = (char *)buf;
            int ret = WSARecv(fd, &wsabuf, 1, (ulong_t *)&recv_bytes, (LPDWORD)&flags, overlapped, 0);
            if (ret < 0) {
                error = socket_get_lasterror();
                return ret;
            }
            return recv_bytes;
        #else 
            int ret = recv(fd, buf, len, flags);
            if (ret < 0) {
                error = socket_get_lasterror();
            }
            return ret;
        #endif
    }

    static inline int socket_recvn(ZRSOCKET_SOCKET fd, char *buf, uint_t len, int flags, ZRSOCKET_OVERLAPPED *overlapped, int &error)
    {
        uint_t recv_bytes = 0;
        int  ret;
        while (recv_bytes < len) {
            ret = recv(fd, buf+ recv_bytes, len- recv_bytes, flags);
            if (ret < 0) {
                error = socket_get_lasterror();
                return recv_bytes;
            }
            else
            {
                recv_bytes += ret;
            }
        }
        return recv_bytes;
    }

    static inline int socket_recvv(ZRSOCKET_SOCKET fd, ZRSOCKET_IOVEC *iov, uint_t iovcnt, int flags, 
        ZRSOCKET_OVERLAPPED *overlapped, int &error)
    {
        #ifdef ZRSOCKET_OS_WINDOWS
            long recv_bytes = 0;
            int ret = WSARecv(fd, (WSABUF *)iov, iovcnt, (ulong_t *)&recv_bytes, (LPDWORD)&flags, overlapped, 0);
            if (ret < 0) {
                error = socket_get_lasterror();
                return ret;
            }
            return recv_bytes;
        #else
            int ret = readv(fd, iov, iovcnt);
            if (ret < 0) {
                error = socket_get_lasterror();
            }
            return ret;
        #endif
    }

    static inline int socket_send(ZRSOCKET_SOCKET fd, const char *buf, uint_t len, int flags = 0, 
        ZRSOCKET_OVERLAPPED *overlapped = nullptr, int *error = nullptr)
    {
        #ifdef ZRSOCKET_OS_WINDOWS
            long send_bytes = 0;
            WSABUF wsabuf;
            wsabuf.len = len;
            wsabuf.buf = (char *)buf;
            int ret = WSASend(fd, &wsabuf, 1, (ulong_t *)&send_bytes, flags, overlapped, 0);
            if (ret < 0) {
                if (nullptr != error) {
                    *error = socket_get_lasterror();
                }
                return ret;
            }
            return send_bytes;
        #else 
            int ret = send(fd, buf, len, flags);
            if (ret < 0) {
                if (nullptr != error) {
                    *error = socket_get_lasterror();
                }
            }
            return ret;
        #endif
    }

    static inline int socket_sendn(ZRSOCKET_SOCKET fd, const char *buf, uint_t len, int flags = 0, 
        ZRSOCKET_OVERLAPPED *overlapped = nullptr, int *error = nullptr)
    {
        uint_t send_bytes = 0;
        int  ret;
        do {
            ret = send(fd, buf+ send_bytes, len-send_bytes, flags);
            if (ret < 0) {
                if (nullptr != error) {
                    *error = socket_get_lasterror();
                }
                return send_bytes;
            }
            else {
                send_bytes += ret;
            }
        } while (send_bytes < len);

        return send_bytes;
    }

    static inline int socket_sendv(ZRSOCKET_SOCKET fd, const ZRSOCKET_IOVEC *iov, int iovcnt, int flags, 
        ZRSOCKET_OVERLAPPED *overlapped, int &error)
    {
        #ifdef ZRSOCKET_OS_WINDOWS
            long send_bytes = 0;
            int ret = WSASend(fd, (WSABUF *)iov, iovcnt, (ulong_t *)&send_bytes, flags, overlapped, 0);
            if (ret < 0) {
                error = socket_get_lasterror();
                return ret;
            }
            return send_bytes;
        #else
            int ret = writev(fd, iov, iovcnt);
            if (ret < 0) {
                error = socket_get_lasterror();
            }
            return ret;
        #endif
    }

    static inline int socket_recvfrom(ZRSOCKET_SOCKET fd, char *buf, uint_t len, int flags, struct sockaddr *src_addr, int *addrlen, 
        ZRSOCKET_OVERLAPPED *overlapped, int &error)
    {
        #ifdef ZRSOCKET_OS_WINDOWS
            long recv_bytes = 0;
            WSABUF wsabuf;
            wsabuf.len = len;
            wsabuf.buf = (char *)buf;
            int ret = WSARecvFrom(fd, &wsabuf, 1, (ulong_t *)&recv_bytes, (LPDWORD)&flags, src_addr, addrlen, overlapped, 0);
            if (ret < 0) {
                error = socket_get_lasterror();
                return ret;
            }
            return recv_bytes;
        #else 
            int ret = recvfrom(fd, buf, len, flags, src_addr, (socklen_t *)addrlen);
            if (ret < 0) {
                error = socket_get_lasterror();
            }
            return ret;
        #endif
    }

    static inline int socket_sendto(ZRSOCKET_SOCKET  fd, const char *buf, uint_t len, int flags, const struct sockaddr *dest_addr, int addrlen, 
        ZRSOCKET_OVERLAPPED *overlapped, int *error)
    {
        #ifdef ZRSOCKET_OS_WINDOWS
            long send_bytes = 0;
            WSABUF wsabuf;
            wsabuf.len = len;
            wsabuf.buf = (char *)buf;
            int ret = WSASendTo(fd, &wsabuf, 1, (ulong_t *)&send_bytes, flags, dest_addr, addrlen, overlapped, 0);
            if (ret < 0) {
                if (nullptr != error) {
                    *error = socket_get_lasterror();
                }
                return ret;
            }
            return send_bytes;
        #else 
            int ret = sendto(fd, buf, len, flags, dest_addr, addrlen);
            if (ret < 0) {
                if (nullptr != error) {
                    *error = socket_get_lasterror();
                }
            }
            return ret;
        #endif
    }

    static inline int socket_sendtov(ZRSOCKET_SOCKET  fd, const ZRSOCKET_IOVEC *iov, int iovcnt, int flags, const struct sockaddr *dest_addr, int addrlen,
        ZRSOCKET_OVERLAPPED *overlapped, int *error)
    {
#ifdef ZRSOCKET_OS_WINDOWS
        long send_bytes = 0;
        int ret = WSASendTo(fd, (WSABUF *)iov, iovcnt, (ulong_t *)&send_bytes, flags, dest_addr, addrlen, overlapped, 0);
        if (ret < 0) {
            if (nullptr != error) {
                *error = socket_get_lasterror();
            }
            return ret;
        }
        return send_bytes;
#else
        /*
        struct msghdr {
            void            *msg_name;        // protocol address
            socklen_t        msg_namelen;     // size of protocol address
            struct iovec    *msg_iov;         // scatter/gather array
            int              msg_iovlen;      // elements in msg_iov
            void            *msg_control;     // ancillary data (cmsghdr struct)
            socklen_t        msg_controllen;  // length of ancillary data
            int              msg_flags;       // flags returned by recvmsg()
        };
        */
        struct msghdr msg;
        msg.msg_name        = (struct sockaddr *)dest_addr;
        msg.msg_namelen     = addrlen;
        msg.msg_iov         = (ZRSOCKET_IOVEC *)iov;
        msg.msg_iovlen      = iovcnt;
        msg.msg_control     = nullptr;
        msg.msg_controllen  = 0;
        msg.msg_flags       = flags;
        int ret = sendmsg(fd, &msg, flags);
        if (ret < 0) {
            if (nullptr != error) {
                *error = socket_get_lasterror();
            }
        }
        return ret;
#endif
    }

    //sleep以毫秒为单位
    static inline void sleep(uint_t timeout_ms)
    {
        #ifdef ZRSOCKET_OS_WINDOWS
            Sleep(timeout_ms);
        #else
            //方法1
            //select();

            //方法2
            //usleep(timeout_ms * 1000);

            //方法3
            struct timespec ts;
            ts.tv_sec  = timeout_ms / 1000;
            ts.tv_nsec = (timeout_ms - ts.tv_sec * 1000) * 1000000L;
            nanosleep(&ts, 0);
        #endif
    }

    //sleep以秒为单位
    static inline void sleep_s(uint_t timeout_s)
    {
        #ifdef ZRSOCKET_OS_WINDOWS
            Sleep(timeout_s * 1000);
        #else
            //方法1
            //select函数

            //方法2
            //sleep(timeout_s);

            //方法3
            //usleep(timeout_s * 10000000L);

            //方法4
            struct timespec ts;
            ts.tv_sec  = timeout_s;
            ts.tv_nsec = 0;
            nanosleep(&ts, nullptr);
        #endif
    }

    //sleep以毫秒为单位
    static inline void sleep_ms(uint_t timeout_ms)
    {
        #ifdef ZRSOCKET_OS_WINDOWS
            Sleep(timeout_ms);
        #else
            //方法1
            //select函数

            //方法2
            //usleep(timeout_ms * 1000);

            //方法3
            struct timespec ts;
            ts.tv_sec  = timeout_ms / 1000;
            ts.tv_nsec = (timeout_ms - ts.tv_sec * 1000) * 1000000L;
            nanosleep(&ts, nullptr);
        #endif
    }

    //sleep以微秒为单位
    static inline void sleep_us(uint64_t timeout_us)
    {
        #ifdef ZRSOCKET_OS_WINDOWS
            Sleep(static_cast<DWORD>(timeout_us / 1000));
        #else
            //方法1
            //select函数

            //方法2
            //usleep(timeout_us);

            //方法3
            struct timespec ts;
            ts.tv_sec  = timeout_us / 1000000L;
            ts.tv_nsec = (timeout_us - ts.tv_sec * 1000000L) * 1000;
            nanosleep(&ts, nullptr);
        #endif
    }

    //sleep以纳秒为单位
    static inline void sleep_ns(uint64_t timeout_ns)
    {
        #ifdef ZRSOCKET_OS_WINDOWS
            Sleep(static_cast<DWORD>(timeout_ns / 1000000));
        #else
            //方法1
            //select函数

            //方法2
            //usleep(timeout_us);

            //方法3
            struct timespec ts;
            ts.tv_sec  = timeout_ns / 1000000000L;
            ts.tv_nsec = timeout_ns - ts.tv_sec * 1000000000L;
            nanosleep(&ts, nullptr);
        #endif
    }

    static inline int64_t atoll(const char *str)
    {
        //方法1
        return DataConvert::atoll(str);

        //方法2
        //_atoi64/atoll(

        //方法3
        //_strtoi64/strtoll

        //#ifdef ZRSOCKET_OS_WINDOWS
        //    return _atoi64(str);
        //#else
        //    return atoll(str);
        //#endif
    }

    static inline uint64_t atoull(const char *str)
    {
        //方法1
        return DataConvert::atoull(str);

        //方法2
        //#ifdef ZRSOCKET_OS_WINDOWS
        //    return _strtoui64(str, nullptr, 10);
        //#else
        //    return strtoull(str, nullptr, 10);
        //#endif
    }

    static inline void i64toa(int64_t i, char *str, int len)
    {
        //方法1
        DataConvert::lltoa(i, str);

        //方法2
        //#ifdef ZRSOCKET_OS_WINDOWS
        //    _i64toa(i, str, 10);
        //#else
        //    //lltoa(i, str, 10);
        //    snprintf(str, len, "%lld", i);
        //#endif
    }

    static inline void ui64toa(uint64_t i, char *str, int len)
    {
        //方法1
        DataConvert::ulltoa(i, str);

        //方法2
        //#ifdef ZRSOCKET_OS_WINDOWS
        //    _ui64toa(i, str, 10);
        //#else
        //    //ulltoa(i, str, 10);
        //    snprintf(str, len, "%llu", i);
        //#endif
    }

    static inline void lltoa(int64_t i, char *str, int len)
    {
        //方法1
        DataConvert::lltoa(i, str);

        //方法2
        //#ifdef ZRSOCKET_OS_WINDOWS
        //    _i64toa(i, str, 10);
        //#else
        //    //lltoa(i, str, 10);
        //    snprintf(str, len, "%lld", i);
        //#endif
    }

    static inline void ulltoa(uint64_t i, char *str, int len)
    {
        //方法1
        DataConvert::ulltoa(i, str);

        //方法2
        //#ifdef ZRSOCKET_OS_WINDOWS
        //    _ui64toa(i, str, 10);
        //#else
        //    //ulltoa(i, str, 10);
        //    snprintf(str, len, "%llu", i);
        //#endif
    }

    //取得系统时钟计数值(单位:ns)(一般是系统时钟的最高精度)
    static inline uint64_t system_clock_counter()
    {
#ifdef ZRSOCKET_OS_WINDOWS
        //方法1 100-nanosecond intervals
        //vc实现c++11中chrono库的内部函数_Xtime_get_ticks(100-nanosecond intervals)
        return _Xtime_get_ticks() * 100;

        //方法2
        //std::chrono::system_clock::now().time_since_epoch().count();

        ////方法3 100-nanosecond intervals
        //#define EPOCH 0x19DB1DED53E8000i64
        //FILETIME ft;
        ////GetSystemTimeAsFileTime(&ft);
        //GetSystemTimePreciseAsFileTime(&ft);
        //return (((static_cast<long long>(ft.dwHighDateTime)) << 32) + static_cast<long long>(ft.dwLowDateTime) - EPOCH) * 100;

        //方法4
        //struct _timeb tb;
        //_ftime_s(&tb);
        //return (tb.time * 1000000000LL + tb.millitm * 1000000LL);
#else
        //方法1
        struct timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);
        return (ts.tv_sec * 1000000000LL + ts.tv_nsec);

        ////方法2
        //struct timeval tv;
        //gettimeofday(&tv, nullptr);
        //return (tv.tv_sec * 1000000000LL + tv.tv_usec * 1000LL);
#endif
    }

    //取得当前系统时间(自公元1970/1/1 00:00:00以来经过纳秒)
    static inline uint64_t time_ns()
    {
        return system_clock_counter();
    }

    //取得当前系统时间(自公元1970/1/1 00:00:00以来经过微秒)
    static inline uint64_t time_us()
    {
#ifdef ZRSOCKET_OS_WINDOWS
        return system_clock_counter() / 1000LL;
#else
        struct timeval tv;
        gettimeofday(&tv, nullptr);
        return (tv.tv_sec * 1000000LL + tv.tv_usec);
#endif
    }

    //取得当前系统时间(自公元1970/1/1 00:00:00以来经过毫秒)
    static inline uint64_t time_ms()
    {
#ifdef ZRSOCKET_OS_WINDOWS
        return system_clock_counter() / 1000000LL;

        //struct _timeb tb;
        //_ftime_s(&tb);
        //return (tb.time * 1000LL + tb.millitm);
#else
        struct timeval tv;
        gettimeofday(&tv, nullptr);
        return (tv.tv_sec * 1000LL + tv.tv_usec / 1000LL);
#endif
    }

    //取得当前系统时间(自公元1970/1/1 00:00:00以来经过秒数)
    static inline uint64_t time_s()
    {
        return ::time(nullptr);
    }

    //取得当前系统时间(自公元1970/1/1 00:00:00以来经过秒数)
    static inline uint64_t time()
    {
        return ::time(nullptr);
    }

    //取得当前系统时间(自公元1970/1/1 00:00:00以来经过秒数,可以精确到微秒)
    static inline int gettimeofday(struct timeval *tv, struct timezone *tz)
    {
        #ifdef ZRSOCKET_OS_WINDOWS
            uint64_t now = system_clock_counter();
            tv->tv_sec   = static_cast<long>(now / 1000000000LL);
            tv->tv_usec  = static_cast<long>(now % 1000000000LL) / 1000LL;
            return 0;

            //struct _timeb tb;
            //_ftime_s(&tb);
            //tv->tv_sec  = tb.time;
            //tv->tv_usec = tb.millitm * 1000L;
        #else
            return gettimeofday(tv, tz);
        #endif
    }

    //取得当前系统时间(自公元1970/1/1 00:00:00以来经过秒数,可以精确到纳秒)
    static inline int gettimeofday(struct timespec *ts)
    {
    #ifdef ZRSOCKET_OS_WINDOWS
        uint64_t now = system_clock_counter();
        ts->tv_sec   = now / 1000000000LL;
        ts->tv_nsec  = now % 1000000000LL;
        return 0;
    #else 
        return clock_gettime(CLOCK_REALTIME, ts);
    #endif
    }

    //时间操作函数
    static inline void time_add(struct timeval *tvp, struct timeval *uvp, struct timeval *vvp)
    {
        vvp->tv_sec  = tvp->tv_sec + uvp->tv_sec;
        vvp->tv_usec = tvp->tv_usec + uvp->tv_usec;
        if (vvp->tv_usec >= 1000000) {
            ++vvp->tv_sec;
            vvp->tv_usec -= 1000000;
        }
    }

    static inline void time_sub(struct timeval *tvp, struct timeval *uvp, struct timeval *vvp)
    {
        vvp->tv_sec  = tvp->tv_sec - uvp->tv_sec;
        vvp->tv_usec = tvp->tv_usec - uvp->tv_usec;
        if (vvp->tv_usec < 0) {
            --vvp->tv_sec;
            vvp->tv_usec += 1000000;
        }
    }

    static inline bool time_isset(struct timeval *tvp)
    {
        return (tvp->tv_sec || tvp->tv_usec);
    }

    static inline void time_clear(struct timeval *tvp)
    {
        tvp->tv_sec = tvp->tv_usec = 0;
    }

    //时间比较函数
    //The return value for each of these functions indicates the lexicographic relation of tvp to uvp
    //<0 tvp less than uvp
    //=0 tvp identical to uvp
    //>0 tvp greater than uvp
    static inline int time_cmp(struct timeval *tvp, struct timeval *uvp)
    {
        if (tvp->tv_sec > uvp->tv_sec) {
            return 1;
        }
        else {
            if (tvp->tv_sec < uvp->tv_sec) {
                return -1;
            }
            if (tvp->tv_usec > uvp->tv_usec) {
                return 1;
            }
            else if (tvp->tv_usec < uvp->tv_usec) {
                return -1;
            }
        }
        return 0;
    }

    //取得系统高精度计数器频率(用于windows系统高精度计时)
    static inline uint64_t os_counter_frequency()
    {
#ifdef ZRSOCKET_OS_WINDOWS
        //方法1
        //LARGE_INTEGER li;
        //QueryPerformanceFrequency(&li);
        //return li.QuadPart;

        //方法2
        static uint64_t counter_frequency = 1;
        if (1 == counter_frequency) {
            LARGE_INTEGER li;
            QueryPerformanceFrequency(&li);
            counter_frequency = li.QuadPart;
        }
        return counter_frequency;
#else
        return 1;
#endif
    }

    //取得系统高精度计数器的计数值
    static inline uint64_t os_counter()
    {
#ifdef ZRSOCKET_OS_WINDOWS
        LARGE_INTEGER li;
        QueryPerformanceCounter(&li);
        return li.QuadPart;
#else
        struct timespec ts;
        clock_gettime(CLOCK_MONOTONIC, &ts);
        return (ts.tv_sec * 1000000000LL + ts.tv_nsec);
#endif
    }

    //计算系统高精度计数器的计时(两次计数值之差即时间差: 以毫秒为时间单位)
    static inline int64_t os_counter_time_ms(uint64_t counter_end, uint64_t counter_start)
    {
        return (counter_end - counter_start) * 1000LL / os_counter_frequency();
    }

    //计算系统高精度计数器的计时(两次计数值之差即时间差: 以微秒为时间单位)
    static inline int64_t os_counter_time_us(uint64_t counter_end, uint64_t counter_start)
    {
        return (counter_end - counter_start) * 1000000LL / os_counter_frequency();
    }

    //计算系统高精度计数器的计时(两次计数值之差即时间差: 以纳秒为时间单位)
    static inline int64_t os_counter_time_ns(uint64_t counter_end, uint64_t counter_start)
    {
        return (counter_end - counter_start) * 1000000000LL / os_counter_frequency();
    }

    //取得稳定时钟计数值(不受修改系统时钟影响/调整系统时间无关 纳秒:ns 只能用于计时)
    static inline uint64_t steady_clock_counter()
    {
#ifdef ZRSOCKET_OS_WINDOWS
        //方法1
        const long long freq  = os_counter_frequency();  // doesn't change after system boot
        const long long ctr   = os_counter();
        const long long whole = ctr / freq * 1000000000i64;
        const long long part  = (ctr % freq) * 1000000000i64 / freq;
        return whole + part;

        //方法2
        //std::chrono::system_clock::now().time_since_epoch().count();

        //const long long freq    = system_counter_frequency();  // doesn't change after system boot
        //const long long ctr     = system_counter();
        //const long long tmp     = ctr / freq;
        //const long long whole   = tmp * 1000000000i64;
        //const long long part    = (ctr - tmp * freq) * 1000000000i64 / freq;
        //return whole + part;
#else
        //方法1
        struct timespec ts;
        clock_gettime(CLOCK_MONOTONIC, &ts);
        return (ts.tv_sec * 1000000000LL + ts.tv_nsec);

        //方法2
        //struct timeval tv;
        //gettimeofday(&tv, nullptr);
        //return (tv.tv_sec * 1000000000LL + tv.tv_usec * 1000L);
#endif
    }

    //取得当前时间戳(不受修改系统时钟影响/调整系统时间无关 纳秒:ns 只能用于计时)
    static inline uint64_t timestamp_ns()
    {
        return steady_clock_counter();
    }

    //取得当前时间戳(不受修改系统时钟影响/调整系统时间无关 微秒:us 只能用于计时)
    static inline uint64_t timestamp_us()
    {
#ifdef ZRSOCKET_OS_WINDOWS
        return steady_clock_counter() / 1000LL;
#else
        //方法1
        struct timespec ts;
        clock_gettime(CLOCK_MONOTONIC, &ts);
        return (ts.tv_sec * 1000000LL + ts.tv_nsec / 1000LL);

        //方法2
        //struct timeval tv;
        //gettimeofday(&tv, nullptr);
        //return (tv.tv_sec * 1000000LL + tv.tv_usec);
#endif
    }

    //取得当前时间戳(不受修改系统时钟影响/调整系统时间无关 毫秒:ms 只能用于计时)
    static inline uint64_t timestamp_ms()
    {
#ifdef ZRSOCKET_OS_WINDOWS
        //方法1 精度低些, 精确到 10-16ms
        //return GetTickCount();

        //方法2 精度高些, 最高精确到 1ms
        //return timeGetTime();

        //方法3
        return steady_clock_counter() / 1000000LL;
#else
        //方法1
        struct timespec ts;
        clock_gettime(CLOCK_MONOTONIC, &ts);
        return (ts.tv_sec * 1000LL + ts.tv_nsec / 1000000LL);

        //方法2
        //struct timeval tv;
        //gettimeofday(&tv, nullptr);
        //return (tv.tv_sec * 1000L + tv.tv_usec / 1000);
#endif
    }

    //取得当前时间戳(不受修改系统时钟影响/调整系统时间无关 秒:s 只能用于计时)
    static inline uint64_t timestamp_s()
    {
#ifdef ZRSOCKET_OS_WINDOWS
        //方法1 精度低些, 精确到 10-16ms
        return GetTickCount() / 1000LL;

        //方法2 精度高些, 最高精确到 1ms
        //return (timeGetTime() / 1000LL);
#else
        
        //方法1
        struct timespec ts;
        clock_gettime(CLOCK_MONOTONIC, &ts);
        return ts.tv_sec;

        //方法2
        //struct timeval tv;
        //gettimeofday(&tv, nullptr);
        //return tv.tv_sec;
#endif
    }

    //取得当前时间戳(不受修改系统时钟影响/调整系统时间无关 微秒:us 只能用于计时)
    static inline uint64_t timestamp()
    {
        return timestamp_us();
    }

    //取得进程时钟滴答(不受修改系统时钟影响/调整系统时间无关 微秒:us 只能用于计时)
    static inline uint64_t gettickcount()
    {
        return timestamp_us();
    }

    //内存拷贝
    static inline void memcpy(void *dest, const void *src, size_t n)
    {
        zrsocket_memcpy(dest, src, n);
    }

    //取得错误号
    static inline int get_errno()
    {
        return errno;
    }

    //取得OS所采用的字节序
    static inline ByteOrder byte_order()
    {
        static ByteOrder byte_order = ByteOrder::BYTE_ORDER_UNKNOWN_ENDIAN;
        if (ByteOrder::BYTE_ORDER_UNKNOWN_ENDIAN == byte_order) {
            const int e = 1;
            if (*(char *)&e) {
                byte_order = ByteOrder::BYTE_ORDER_LITTLE_ENDIAN;
            }
            else {
                byte_order = ByteOrder::BYTE_ORDER_BIG_ENDIAN;
            }
        }
        return byte_order;
    }

    //64位整数的主机字节序转为网络字节序
    static inline int64_t htonll(int64_t host_int64)
    {
        #if ZRSOCKET_BYTE_ORDER == __ZRSOCKET_BIG_ENDIAN
            return host_int64;
        #else
            #ifdef ZRSOCKET_OS_WINDOWS
                return (static_cast<uint64_t>(htonl(static_cast<ulong_t>(host_int64))) << 32) + htonl(host_int64 >> 32);
            #else
                return bswap_64(host_int64);
            #endif
        #endif
    }

    //64位整数的网络字节序转为主机字节序
    static inline int64_t ntohll(int64_t net_int64)
    {
        #if ZRSOCKET_BYTE_ORDER == __ZRSOCKET_BIG_ENDIAN
            return net_int64;
        #else
            #ifdef ZRSOCKET_OS_WINDOWS
                return (static_cast<uint64_t>(ntohl(static_cast<ulong_t>(net_int64))) << 32) + ntohl(net_int64 >> 32);
            #else
                return bswap_64(net_int64);
            #endif
        #endif
    }

    // We can't just use _snprintf/snprintf as a drop-in replacement, because it
    // doesn't always NUL-terminate. :-(
    static inline int snprintf(char *str, size_t size, const char *format, ...)
    {
        if (0 == size) {    // not even room for a \0?
            return -1;      // not what C99 says to do, but what windows does
        }

        str[size-1] = '\0';
        va_list ap;
        va_start(ap, format);
        #ifdef ZRSOCKET_OS_WINDOWS
            const int rc = vsnprintf(str, size-1, format, ap);
        #else
            const int rc = vsnprintf(str, size-1, format, ap);
        #endif
        va_end(ap);
        return rc;
    }

    //取当前线程id
    /*说明:
    1.windows下直接用GetCurrentThreadId()
    2.linux当前线程id方法:
        1)用系统调用gettid(2)获取内核中的线程id;
        2)POSIX线程库提供的pthread_self(3)方法获取分配的线程id;
        3)C++11 std::thread的get_id()方法，封装的也是POSIX pthread线程库的线程id;
        在Linux中，线程本质上是一个进程（实现）,也就是说通过系统调用gettid获取的线程id跟进程id是一样的
        glibc的Pthreads实现，把pthread_self返回值类型pthread_t用作一个结构体指针（类型为unsigned long），
            指向一个动态分配的内存，而且该内存是反复使用的。
            这也就是说，pthread_t的值很容易重复。Pthreads只能保证同一个进程内，同一时刻的各个线程id不同；
            但不能保证同一进程先后多个线程具有不同id，不能保证一台机器上多个进程间的id不同。
        因此，pthread_t不适合作为程序中对线程的标识符id
        建议用gettid(2)系统调用返回值作为线程id，好处在于：
            类型是pid_t，其值是一个小整数（最大值 / proc / sys / kernel / pid_max，默认32768），便于log输出；
            表示内核的任务调度id，在 / proc文件系统中，可以找到对应项： / proc / tid，或 / prod / pid / task / tid；
            在其他系统工具中容易定位到具体某个线程，
                如top(1)命令中，可以按线程列出任务，然后找出CPU使用率最高的线程id，再根据log判断到底哪个线程在耗费CPU；
            任何时刻都是全局唯一的，且由于Linux分配新pid采用递增轮回办法，短时间内启动的多个线程也会具有不同的线程id；
            0是非法值，因为操作系统的第一个进程init的pid是1，而gettid采用的线程tid本质上是进程pid，因此不能再为1

       The gettid() system call first appeared on Linux in kernel 2.4.11;
       Library support was added in glibc 2.30;
       Earlier glibc versions did not provide a wrapper for this system call, necessitating the use of syscall(2);

    由于系统调用会陷入内核，频繁系统调用可能会影响系统性能，有没有办法可以避免这个问题？
    答案是有的.考虑到线程id在线程创建后,并不会随意改变,
    因此可以用每个线程自带的thread_local/__thread(gcc)变量来缓存其线程id值,初值设为0或负数即可
    */
    static inline uint64_t this_thread_id()
    {
        static zrsocket_fast_thread_local uint64_t id_ = 0;
        if (0 != id_) {
            return id_;
        }

    #if 1
        //方法1: 调用操作系统api
        #ifdef  ZRSOCKET_OS_WINDOWS
            id_ = ::GetCurrentThreadId();
        #else
            //因glibc2.30才有::gettid(), 所以用syscall间接实现
            id_ = static_cast<uint64_t>(::syscall(SYS_gettid));
        #endif
    #else
        //方法2: c++11的std::this_thread::get_id()
        std::stringstream ss;
        ss << std::this_thread::get_id();
        ss >> id_;
    #endif

        return id_;
    }

    static inline struct tm* gmtime_s(const time_t *time, struct tm *buf_tm)
    {
    #ifdef ZRSOCKET_OS_WINDOWS
        if (::gmtime_s(buf_tm, time) != 0) {
            return buf_tm;
        }
        return nullptr;
    #else
        return ::gmtime_r(time, buf_tm);
    #endif
    }

    static inline struct tm* localtime_s(const time_t *time, struct tm *buf_tm)
    {
    #ifdef ZRSOCKET_OS_WINDOWS
        //windows下::localtime_s这个函数, 特费时: 700-900us. 不知什么原因
        //比gmtime_s差很多
        if (::localtime_s(buf_tm, time) != 0) {
            return buf_tm;
        }
        return nullptr;
    #else
        return ::localtime_r(time, buf_tm);
    #endif
    }
    
public:
    OSApi() = delete;
    ~OSApi() = delete;

    OSApi(const OSApi&) = delete;
    OSApi(OSApi&&) = delete;
    OSApi& operator=(const OSApi&) = delete;
    OSApi& operator=(OSApi&&) = delete;
};

ZRSOCKET_NAMESPACE_END

#endif
