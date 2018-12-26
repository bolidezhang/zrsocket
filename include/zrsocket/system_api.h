#ifndef ZRSOCKET_SYSTEM_API_H_
#define ZRSOCKET_SYSTEM_API_H_
#include <time.h>
#include <stdio.h>
#include <stdarg.h>         // for va_list, va_start, va_end
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

    //windows��ý���(��Ҫʹ������timeGetTime())
    #include <MMSystem.h>
    #pragma comment(lib, "Winmm.lib")
#else
    #include <sys/time.h>
    #include <sys/ioctl.h>
    #include <byteswap.h>
#endif

ZRSOCKET_BEGIN

class ZRSOCKET_EXPORT SystemApi
{  
public:
    //socket_init ����timer_resolution���壺 1) =0 �Զ�����OS����ʱ���ȵ���Сֵ 2) >0�ֶ����ö�ʱ����(���ȷ�Χ: 1-5 ms)
    static int socket_init(int version_high, int version_low, int timer_resolution_ms = 0)
    {
        #ifdef ZRSOCKET_OS_WINDOWS
            //initialize winsock
            WSADATA wsaData;
            WORD version_requested = MAKEWORD(version_high, version_low);
            return WSAStartup(version_requested, &wsaData);

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
        #ifdef ZRSOCKET_OS_WINDOWS
            return WSAGetLastError();
        #else
            return errno;
        #endif

    }

    static inline int socket_get_error(ZRSOCKET_SOCKET fd)
    {
        int socket_error = 0;
        int socket_error_len = sizeof(int);
        if (getsockopt(fd, SOL_SOCKET, SO_ERROR, (char *)&socket_error, (socklen_t *)&socket_error_len) < 0)
        {
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
        if ( (fd < 0) || (ZRSOCKET_INVALID_SOCKET == fd) )
        {
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
        if ( (fd < 0) || (ZRSOCKET_INVALID_SOCKET == fd) )
        {
            return -1;
        }
        return shutdown(fd, how);
    }

    static inline ZRSOCKET_SOCKET socket_accept(ZRSOCKET_SOCKET fd, struct sockaddr *addr, int *addrlen, int flags = 0)
    {
        #ifdef ZRSOCKET_OS_WINDOWS
            return WSAAccept(fd, addr, addrlen, NULL, 0);
        #else
            #ifdef ZRSOCKET_USE_ACCEPT4
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
            {
                unsigned long arg = (block ? 0:1);
                return ioctlsocket(fd, FIONBIO, &arg);
            }
        #else
            //����1
            int flags = fcntl(fd, F_GETFL);
            if (block)
            {
                flags &= ~O_NONBLOCK;
            }
            else
            {
                flags |= O_NONBLOCK;
            }
            if (fcntl(fd, F_SETFL, flags) == -1) 
            {
                return -1;
            }
            return 0;

            //����2
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

                    //����socket��keepalivetime,keepaliveinterval
                    in_keepalive.onoff = 1;
                    in_keepalive.keepalivetime = keepalivetime;
                    in_keepalive.keepaliveinterval = keepaliveinterval;

                    //Ϊѡ����SOCKET����Keep Alive���ɹ���SOCKET��ͨ��KeepAlive�Զ���������Ƿ�Ͽ� 
                    if ( SOCKET_ERROR == WSAIoctl(fd, SIO_KEEPALIVE_VALS,(LPVOID)&in_keepalive, inlen,(LPVOID)&out_keepalive, outlen, &bytesreturn, NULL, NULL) )
                    {
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

    static inline int socket_set_broadcast(ZRSOCKET_SOCKET fd, int flag)
    {
        return setsockopt(fd, SOL_SOCKET, SO_BROADCAST, (char *)&flag, sizeof(int));
    }

    static inline int socket_set_send_buffersize(ZRSOCKET_SOCKET fd, int sz)
    {
        return setsockopt(fd, SOL_SOCKET, SO_SNDBUF, (char *)&sz, sizeof(int));
    }

    static inline int socket_get_send_buffersize(ZRSOCKET_SOCKET fd)
    {
        int sz  = 0;
        int len = sizeof(int);
        if (getsockopt(fd, SOL_SOCKET, SO_SNDBUF, (char *)&sz, (socklen_t *)&len) == ZRSOCKET_SOCKET_ERROR)
        {
            return -1;
        }
        return sz;
    }

    static inline int socket_get_recv_buffersize(ZRSOCKET_SOCKET fd)
    {
        int sz  = 0;
        int len = sizeof(int);
        if (getsockopt(fd, SOL_SOCKET, SO_RCVBUF, (char *)&sz, (socklen_t *)&len) == ZRSOCKET_SOCKET_ERROR)
        {
            return -1;
        }
        return sz;
    }

    static inline int socket_set_recv_buffersize(ZRSOCKET_SOCKET fd, int sz)
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
            long   bytes_recv = 0;
            WSABUF wsabuf;
            wsabuf.len = len;
            wsabuf.buf = (char *)buf;
            int ret = WSARecv(fd, &wsabuf, 1, (ulong_t *)&bytes_recv, (LPDWORD)&flags, overlapped, 0);
            if (ret < 0) {
                error = socket_get_lasterror_wsa();
                return ret;
            }
            return bytes_recv;
        #else 
            int ret = recv(fd, buf, len, flags);
            if (ret < 0) {
                error = socket_get_lasterror_wsa();
            }
            return ret;
        #endif
    }

    static inline int socket_recvn(ZRSOCKET_SOCKET fd, char *buf, uint_t len, int flags, ZRSOCKET_OVERLAPPED *overlapped, int &error)
    {
        uint_t bytes_recv = 0;
        int  ret;
        while (bytes_recv < len) {
            ret = recv(fd, buf+bytes_recv, len-bytes_recv, flags);
            if (ret < 0) {
                error = socket_get_lasterror_wsa();
                return bytes_recv;
            }
            else
            {
                bytes_recv += ret;
            }
        }
        return bytes_recv;
    }

    static inline int socket_recvv(ZRSOCKET_SOCKET fd, ZRSOCKET_IOVEC *iov, uint_t iovcnt, int flags, ZRSOCKET_OVERLAPPED *overlapped, int &error)
    {
        #ifdef ZRSOCKET_OS_WINDOWS
            long bytes_recv = 0;
            int ret = WSARecv(fd, (WSABUF *)iov, iovcnt, (ulong_t *)&bytes_recv, (LPDWORD)&flags, overlapped, 0);
            if (ret < 0)
            {
                error = socket_get_lasterror_wsa();
                return ret;
            }
            return bytes_recv;
        #else
            int ret = readv(fd, iov, iovcnt);
            if (ret < 0)
            {
                error = socket_get_lasterror_wsa();
            }
            return ret;
        #endif
    }

    static inline int socket_send(ZRSOCKET_SOCKET fd, const char *buf, uint_t len, int flags = 0, ZRSOCKET_OVERLAPPED *overlapped = NULL, int *error = NULL)
    {
        #ifdef ZRSOCKET_OS_WINDOWS
            long   bytes_send = 0;
            WSABUF wsabuf;
            wsabuf.len = len;
            wsabuf.buf = (char *)buf;
            int ret = WSASend(fd, &wsabuf, 1, (ulong_t *)&bytes_send, flags, overlapped, 0);
            if (ret < 0) {
                if (NULL != error) {
                    *error = socket_get_lasterror_wsa();
                }
                return ret;
            }
            return bytes_send;
        #else 
            int ret = send(fd, buf, len, flags);
            if (ret < 0) {
                if (NULL != error) {
                    *error = socket_get_lasterror_wsa();
                }
            }
            return ret;
        #endif
    }

    static inline int socket_sendn(ZRSOCKET_SOCKET fd, const char *buf, uint_t len, int flags = 0, ZRSOCKET_OVERLAPPED *overlapped = NULL, int *error = NULL)
    {
        uint_t bytes_send = 0;
        int  ret;
        while (bytes_send < len ) {
            ret = send(fd, buf+bytes_send, len-bytes_send, flags);
            if (ret < 0) {
                if (NULL != error) {
                    *error = socket_get_lasterror_wsa();
                }
                return bytes_send;
            }
            else {
                bytes_send += ret;
            }
        }
        return bytes_send;
    }

    static inline int socket_sendv(ZRSOCKET_SOCKET fd, const ZRSOCKET_IOVEC *iov, int iovcnt, int flags, ZRSOCKET_OVERLAPPED *overlapped, int &error)
    {
        #ifdef ZRSOCKET_OS_WINDOWS
            long  bytes_send = 0;
            int ret = WSASend(fd, (WSABUF *)iov, iovcnt, (ulong_t *)&bytes_send, flags, overlapped, 0);
            if (ret < 0) {
                error = socket_get_lasterror_wsa();
                return ret;
            }
            return bytes_send;
        #else
            int ret = writev(fd, iov, iovcnt);
            if (ret < 0) {
                error = socket_get_lasterror_wsa();
            }
            return ret;
        #endif
    }

    static inline int socket_recvfrom(ZRSOCKET_SOCKET fd, char *buf, uint_t len, int flags, struct sockaddr *src_addr, int *addrlen, ZRSOCKET_OVERLAPPED *overlapped, int &error)
    {
        #ifdef ZRSOCKET_OS_WINDOWS
            long   bytes_recv = 0;
            WSABUF wsabuf;
            wsabuf.len = len;
            wsabuf.buf = (char *)buf;
            int ret = WSARecvFrom(fd, &wsabuf, 1, (ulong_t *)&bytes_recv, (LPDWORD)&flags, src_addr, addrlen, overlapped, 0);
            if (ret < 0) {
                error = socket_get_lasterror_wsa();
                return ret;
            }
            return bytes_recv;
        #else 
            int ret = recvfrom(fd, buf, len, flags, src_addr, (socklen_t *)addrlen);
            if (ret < 0) {
                error = socket_get_lasterror_wsa();
            }
            return ret;
        #endif
    }

    static inline int socket_sendto(ZRSOCKET_SOCKET  fd, const char *buf, uint_t len, int flags, const struct sockaddr *dest_addr, int addrlen, ZRSOCKET_OVERLAPPED *overlapped, int *error)
    {
        #ifdef ZRSOCKET_OS_WINDOWS
            long   bytes_send = 0;
            WSABUF wsabuf;
            wsabuf.len = len;
            wsabuf.buf = (char *)buf;
            int ret = WSASendTo(fd, &wsabuf, 1, (ulong_t *)&bytes_send, flags, dest_addr, addrlen, overlapped, 0);
            if (ret < 0) {
                if (NULL != error) {
                    *error = socket_get_lasterror_wsa();
                }
                return ret;
            }
            return bytes_send;
        #else 
            int ret = sendto(fd, buf, len, flags, dest_addr, addrlen);
            if (ret < 0) {
                if (NULL != error) {
                    *error = socket_get_lasterror_wsa();
                }
            }
            return ret;
        #endif
    }

    //sleep�Ժ���Ϊ��λ
    static inline void util_sleep(uint_t timeout_ms)
    {
        #ifdef ZRSOCKET_OS_WINDOWS
            Sleep(timeout_ms);
        #else
            //����1
            //select();

            //����2
            //usleep(timeout_ms * 1000);

            //����3
            struct timespec ts;
            ts.tv_sec  = timeout_ms / 1000;
            ts.tv_nsec = (timeout_ms - ts.tv_sec * 1000) * 1000000L;
            nanosleep(&ts, 0);
        #endif
    }

    //sleep����Ϊ��λ
    static inline void util_sleep_s(uint_t timeout_s)
    {
        #ifdef ZRSOCKET_OS_WINDOWS
            Sleep(timeout_s * 1000);
        #else
            //����1
            //select����

            //����2
            //sleep(timeout_s);

            //����3
            //usleep(timeout_s * 10000000L);

            //����4
            struct timespec ts;
            ts.tv_sec  = timeout_s;
            ts.tv_nsec = 0;
            nanosleep(&ts, NULL);
        #endif
    }

    //sleep�Ժ���Ϊ��λ
    static inline void util_sleep_ms(uint_t timeout_ms)
    {
        #ifdef ZRSOCKET_OS_WINDOWS
            Sleep(timeout_ms);
        #else
            //����1
            //select����

            //����2
            //usleep(timeout_ms * 1000);

            //����3
            struct timespec ts;
            ts.tv_sec  = timeout_ms / 1000;
            ts.tv_nsec = (timeout_ms - ts.tv_sec * 1000) * 1000000L;
            nanosleep(&ts, NULL);
        #endif
    }

    //sleep��΢��Ϊ��λ
    static inline void util_sleep_us(uint_t timeout_us)
    {
        #ifdef ZRSOCKET_OS_WINDOWS
            Sleep(timeout_us / 1000);
        #else
            //����1
            //select����

            //����2
            //usleep(timeout_us);

            //����3
            struct timespec ts;
            ts.tv_sec  = timeout_us / 1000000L;
            ts.tv_nsec = (timeout_us - ts.tv_sec * 1000000L) * 1000;
            nanosleep(&ts, NULL);
        #endif
    }

    //sleep������Ϊ��λ
    static inline void util_sleep_ns(uint64_t timeout_ns)
    {
        #ifdef ZRSOCKET_OS_WINDOWS
            Sleep(timeout_ns / 1000000);
        #else
            //����1
            //select����

            //����2
            //usleep(timeout_us);

            //����3
            struct timespec ts;
            ts.tv_sec  = timeout_ns / 1000000000L;
            ts.tv_nsec = timeout_ns - ts.tv_sec * 1000000000L;
            nanosleep(&ts, NULL);
        #endif
    }

    static inline int64_t util_atoll(const char *str)
    {
        //����1
        return DataConvert::atoll(str);

        //����2
        //_atoi64/atoll(

        //����3
        //_strtoi64/strtoll

        //#ifdef ZRSOCKET_OS_WINDOWS
        //    return _atoi64(str);
        //#else
        //    return atoll(str);
        //#endif
    }

    static inline uint64_t util_atoull(const char *str)
    {
        //����1
        return DataConvert::atoull(str);

        //����2
        //#ifdef ZRSOCKET_OS_WINDOWS
        //    return _strtoui64(str, NULL, 10);
        //#else
        //    return strtoull(str, NULL, 10);
        //#endif
    }

    static inline void util_i64toa(int64_t i, char *str, int len)
    {
        //����1
        DataConvert::lltoa(i, str);

        //����2
        //#ifdef ZRSOCKET_OS_WINDOWS
        //    _i64toa(i, str, 10);
        //#else
        //    //lltoa(i, str, 10);
        //    snprintf(str, len, "%lld", i);
        //#endif
    }

    static inline void util_ui64toa(uint64_t i, char *str, int len)
    {
        //����1
        DataConvert::ulltoa(i, str);

        //����2
        //#ifdef ZRSOCKET_OS_WINDOWS
        //    _ui64toa(i, str, 10);
        //#else
        //    //ulltoa(i, str, 10);
        //    snprintf(str, len, "%llu", i);
        //#endif
    }

    static inline void util_lltoa(int64_t i, char *str, int len)
    {
        //����1
        DataConvert::lltoa(i, str);

        //����2
        //#ifdef ZRSOCKET_OS_WINDOWS
        //    _i64toa(i, str, 10);
        //#else
        //    //lltoa(i, str, 10);
        //    snprintf(str, len, "%lld", i);
        //#endif
    }

    static inline void util_ulltoa(uint64_t i, char *str, int len)
    {
        //����1
        DataConvert::ulltoa(i, str);

        //����2
        //#ifdef ZRSOCKET_OS_WINDOWS
        //    _ui64toa(i, str, 10);
        //#else
        //    //ulltoa(i, str, 10);
        //    snprintf(str, len, "%llu", i);
        //#endif
    }

    //ȡ�õ�ǰϵͳʱ��(�Թ�Ԫ1970/1/1 00:00:00������������)
    static inline uint64_t util_time()
    {
        return time(NULL);
    }

    //ȡ�õ�ǰϵͳʱ��(�Թ�Ԫ1970/1/1 00:00:00������������)
    static inline uint64_t util_time_s()
    {
        return time(NULL);
    }

    //ȡ�õ�ǰϵͳʱ��(�Թ�Ԫ1970/1/1 00:00:00������������)
    static inline uint64_t util_time_ms()
    {
        #ifdef ZRSOCKET_OS_WINDOWS
            struct _timeb tb;
            _ftime_s(&tb);
            return (tb.time * 1000LL + tb.millitm);
        #else
            struct timeval tv;
            gettimeofday(&tv, NULL);
            return (tv.tv_sec * 1000LL + tv.tv_usec / 1000LL);
        #endif
    }

    //ȡ�õ�ǰϵͳʱ��(�Թ�Ԫ1970/1/1 00:00:00��������΢��)
    static inline uint64_t util_time_us()
    {
        #ifdef ZRSOCKET_OS_WINDOWS
            struct _timeb tb;
            _ftime_s(&tb);
            return (tb.time * 1000000LL + tb.millitm * 1000LL);
        #else
            struct timeval tv;
            gettimeofday(&tv, NULL);
            return (tv.tv_sec * 1000000LL + tv.tv_usec);
        #endif
    }

    //ȡ�õ�ǰϵͳʱ��(�Թ�Ԫ1970/1/1 00:00:00������������)
    static inline uint64_t util_time_ns()
    {
        #ifdef ZRSOCKET_OS_WINDOWS
            struct _timeb tb;
            _ftime_s(&tb);
            return (tb.time * 1000000000LL + tb.millitm * 1000000LL);
        #else
            //����1
            struct timespec tp;
            clock_gettime(CLOCK_REALTIME, &tp);
            return (tp.tv_sec * 1000000000LL + tp.tv_nsec);

            ////����2
            //struct timeval tv;
            //gettimeofday(&tv, NULL);
            //return (tv.tv_sec * 1000000000LL + tv.tv_usec * 1000LL);
        #endif
    }

    //ȡ�õ�ǰϵͳʱ��(�Թ�Ԫ1970/1/1 00:00:00������������,���Ծ�ȷ��΢��)
    static inline int util_gettimeofday(struct timeval *tv, struct timezone *tz)
    {
        #ifdef ZRSOCKET_OS_WINDOWS
            struct _timeb tb;
            _ftime_s(&tb);

            tv->tv_sec  = tb.time;
            tv->tv_usec = tb.millitm * 1000L;
        #else
            gettimeofday(tv, tz);
        #endif
        return 0;
    }

    //ʱ���������
    static inline void util_timeradd(struct timeval *tvp, struct timeval *uvp, struct timeval *vvp)
    {
        vvp->tv_sec  = tvp->tv_sec + uvp->tv_sec;
        vvp->tv_usec = tvp->tv_usec + uvp->tv_usec;
        if (vvp->tv_usec >= 1000000)
        {
            ++vvp->tv_sec;
            vvp->tv_usec -= 1000000;
        }
    }

    static inline void util_timersub(struct timeval *tvp, struct timeval *uvp, struct timeval *vvp)
    {
        vvp->tv_sec  = tvp->tv_sec - uvp->tv_sec;
        vvp->tv_usec = tvp->tv_usec - uvp->tv_usec;
        if (vvp->tv_usec < 0)
        {
            --vvp->tv_sec;
            vvp->tv_usec += 1000000;
        }
    }

    static inline bool util_timerisset(struct timeval *tvp)
    {
        return (tvp->tv_sec || tvp->tv_usec);
    }

    static inline void util_timerclear(struct timeval *tvp)
    {
        tvp->tv_sec = tvp->tv_usec = 0;
    }

    //ʱ��ȽϺ���
    //The return value for each of these functions indicates the lexicographic relation of tvp to uvp
    //<0 tvp less than uvp
    //=0 tvp identical to uvp
    //>0 tvp greater than uvp
    static inline int util_timercmp(struct timeval *tvp, struct timeval *uvp)
    {
        if (tvp->tv_sec > uvp->tv_sec)
        {
            return 1;
        }
        else
        {
            if (tvp->tv_sec < uvp->tv_sec)
            {
                return -1;
            }
            if (tvp->tv_usec > uvp->tv_usec)
            {
                return 1;
            }
            else if (tvp->tv_usec < uvp->tv_usec)
            {
                return -1;
            }
        }
        return 0;
    }

    //ȡ�ý���ʱ�ӵδ�(�����޸�ϵͳʱ��Ӱ��/����ϵͳʱ���޹� ����:ms ֻ�����ڼ�ʱ)
    static inline uint64_t util_gettickcount()
    {
        #ifdef ZRSOCKET_OS_WINDOWS
            //����1 ���ȵ�Щ, ��ȷ�� 10-35ms
            //return GetTickCount();

            //����2 ���ȸ�Щ, ��ȷ�� 1ms
            return timeGetTime();
        #else  
            //����1
            struct timespec tp;
            clock_gettime(CLOCK_MONOTONIC, &tp);
            return (tp.tv_sec * 1000LL + tp.tv_nsec / 1000000LL);

            //����2
            //struct timeval tv;
            //gettimeofday(&tv, NULL);
            //return (tv.tv_sec*1000L + tv.tv_usec/1000);
        #endif
    }

    //ȡ�ý���ʱ�ӵδ�(�����޸�ϵͳʱ��Ӱ��/����ϵͳʱ���޹� ��:s ֻ�����ڼ�ʱ)
    static inline uint64_t util_process_clock_s()
    {
        #ifdef ZRSOCKET_OS_WINDOWS
            //����1 ���ȵ�Щ, ��ȷ�� 10-35ms
            //return GetTickCount();

            //����2 ���ȸ�Щ, ��ȷ�� 1ms
            return (timeGetTime() / 1000LL);
        #else
            //����1
            struct timespec tp;
            clock_gettime(CLOCK_MONOTONIC, &tp);
            return tp.tv_sec;

            //����2
            //struct timeval tv;
            //gettimeofday(&tv, NULL);
            //return tv.tv_sec;
        #endif
    }

    //ȡ�ý���ʱ�ӵδ�(�����޸�ϵͳʱ��Ӱ��/����ϵͳʱ���޹� ����:ms ֻ�����ڼ�ʱ)
    static inline uint64_t util_process_clock_ms()
    {
        #ifdef ZRSOCKET_OS_WINDOWS
            //����1 ���ȵ�Щ, ��ȷ�� 10-35ms
            //return GetTickCount();

            //����2 ���ȸ�Щ, ��ȷ�� 1ms
            return timeGetTime();
        #else
            //����1
            struct timespec tp;
            clock_gettime(CLOCK_MONOTONIC, &tp);
            return (tp.tv_sec * 1000LL + tp.tv_nsec / 1000000LL);

            //����2
            //struct timeval tv;
            //gettimeofday(&tv, NULL);
            //return (tv.tv_sec * 1000L + tv.tv_usec / 1000);
        #endif
    }

    //ȡ�ý���ʱ�ӵδ�(�����޸�ϵͳʱ��Ӱ��/����ϵͳʱ���޹� ΢��:us ֻ�����ڼ�ʱ)
    static inline uint64_t util_process_clock_us()
    {
        #ifdef ZRSOCKET_OS_WINDOWS
            //����1 ���ȵ�Щ, ��ȷ�� 10-35ms
            //return (GetTickCount() * 1000);

            //����2 ���ȸ�Щ, ��ȷ�� 1ms
            return (timeGetTime() * 1000LL);
        #else
            //����1
            struct timespec tp;
            clock_gettime(CLOCK_MONOTONIC, &tp);
            return (tp.tv_sec * 1000000LL + tp.tv_nsec / 1000LL);

            //����2
            //struct timeval tv;
            //gettimeofday(&tv, NULL);
            //return (tv.tv_sec * 1000000LL + tv.tv_usec);
        #endif
    }

    //ȡ�ý���ʱ�ӵδ�(�����޸�ϵͳʱ��Ӱ��/����ϵͳʱ���޹� ����:ns ֻ�����ڼ�ʱ)
    static inline uint64_t util_process_clock_ns()
    {
        #ifdef ZRSOCKET_OS_WINDOWS
            //����1 ���ȵ�Щ, ��ȷ�� 10-35ms
            //return (GetTickCount() * 1000);

            //����2 ���ȸ�Щ, ��ȷ�� 1ms
            return (timeGetTime() * 1000000LL);
        #else
            //����1
            struct timespec tp;
            clock_gettime(CLOCK_MONOTONIC, &tp);
            return (tp.tv_sec * 1000000000LL + tp.tv_nsec);

            //����2
            //struct timeval tv;
            //gettimeofday(&tv, NULL);
            //return (tv.tv_sec * 1000000000LL + tv.tv_usec * 1000L);
        #endif
    }

    //��������ʱ�Ӳ�
    static inline int util_process_clock_difference(uint_t end_clock, uint_t start_clock)
    {
        return (end_clock - start_clock);
    }

    //ȡ�õ�ǰʱ���(�����޸�ϵͳʱ��Ӱ��/����ϵͳʱ���޹� ����:ms ֻ�����ڼ�ʱ)
    static inline uint64_t util_timestamp()
    {
        #ifdef ZRSOCKET_OS_WINDOWS
            //����1 ���ȵ�Щ, ��ȷ�� 10-35ms
            //return GetTickCount();

            //����2 ���ȸ�Щ, ��ȷ�� 1ms
            return timeGetTime();
        #else
            //����1
            struct timespec tp;
            clock_gettime(CLOCK_MONOTONIC, &tp);
            return (tp.tv_sec * 1000LL + tp.tv_nsec / 1000000LL);

            //����2
            //struct timeval tv;
            //gettimeofday(&tv, NULL);
            //return (tv.tv_sec * 1000L + tv.tv_usec / 1000);
        #endif
    }

    //ȡ�õ�ǰʱ���(�����޸�ϵͳʱ��Ӱ��/����ϵͳʱ���޹� ����:s ֻ�����ڼ�ʱ)
    static inline uint64_t util_timestamp_s()
    {
        #ifdef ZRSOCKET_OS_WINDOWS
            //����1 ���ȵ�Щ, ��ȷ�� 10-35ms
            //return GetTickCount();

            //����2 ���ȸ�Щ, ��ȷ�� 1ms
            return (timeGetTime() / 1000LL);
        #else
            //����1
            struct timespec tp;
            clock_gettime(CLOCK_MONOTONIC, &tp);
            return tp.tv_sec;

            //����2
            //struct timeval tv;
            //gettimeofday(&tv, NULL);
            //return tv.tv_sec;
        #endif
    }

    //ȡ�õ�ǰʱ���(�����޸�ϵͳʱ��Ӱ��/����ϵͳʱ���޹� ����:ms ֻ�����ڼ�ʱ)
    static inline uint64_t util_timestamp_ms()
    {
        #ifdef ZRSOCKET_OS_WINDOWS
            //����1 ���ȵ�Щ, ��ȷ�� 10-35ms
            //return GetTickCount();

            //����2 ���ȸ�Щ, ��ȷ�� 1ms
            return timeGetTime();
        #else
            //����1
            struct timespec tp;
            clock_gettime(CLOCK_MONOTONIC, &tp);
            return (tp.tv_sec * 1000LL + tp.tv_nsec / 1000000LL);

            //����2
            //struct timeval tv;
            //gettimeofday(&tv, NULL);
            //return (tv.tv_sec * 1000L + tv.tv_usec / 1000);
        #endif
    }

    //ȡ�õ�ǰʱ���(�����޸�ϵͳʱ��Ӱ��/����ϵͳʱ���޹� ΢��:us ֻ�����ڼ�ʱ)
    static inline uint64_t util_timestamp_us()
    {
        #ifdef ZRSOCKET_OS_WINDOWS
            //����1 ���ȵ�Щ, ��ȷ�� 10-35ms
            //return (GetTickCount() * 1000);

            //����2 ���ȸ�Щ, ��ȷ�� 1ms
            return (timeGetTime() * 1000LL);
        #else
            //����1
            struct timespec tp;
            clock_gettime(CLOCK_MONOTONIC, &tp);
            return (tp.tv_sec * 1000000LL + tp.tv_nsec / 1000LL);

            //����2
            //struct timeval tv;
            //gettimeofday(&tv, NULL);
            //return (tv.tv_sec * 1000000LL + tv.tv_usec);
        #endif
    }

    //ȡ�õ�ǰʱ���(�����޸�ϵͳʱ��Ӱ��/����ϵͳʱ���޹� ����:ns ֻ�����ڼ�ʱ)
    static inline uint64_t util_timestamp_ns()
    {
        #ifdef ZRSOCKET_OS_WINDOWS
            //����1 ���ȵ�Щ, ��ȷ�� 10-35ms
            //return (GetTickCount() * 1000);

            //����2 ���ȸ�Щ, ��ȷ�� 1ms
            return (timeGetTime() * 1000000LL);
        #else
            //����1
            struct timespec tp;
            clock_gettime(CLOCK_MONOTONIC, &tp);
            return (tp.tv_sec * 1000000000LL + tp.tv_nsec);

            //����2
            //struct timeval tv;
            //gettimeofday(&tv, NULL);
            //return (tv.tv_sec * 1000000000LL + tv.tv_usec * 1000L);
        #endif
    }

    //ȡ��ϵͳ�߾��ȼ�����Ƶ��(����windowsϵͳ�߾��ȼ�ʱ)
    static inline uint64_t util_system_counter_frequency()
    {
        #ifdef ZRSOCKET_OS_WINDOWS
            static uint64_t counter_frequency = 1;
            if (1 == counter_frequency)
            {
                LARGE_INTEGER i;
                if (QueryPerformanceFrequency(&i))
                {
                    counter_frequency = i.QuadPart;
                }
            }
            return counter_frequency;
        #else
            return 1;
        #endif
    }

    //ȡ��ϵͳ�߾��ȼ������ļ���ֵ
    static inline uint64_t util_system_counter()
    {
        #ifdef ZRSOCKET_OS_WINDOWS
            LARGE_INTEGER i;
            QueryPerformanceCounter(&i);
            return i.QuadPart;
        #else
            struct timespec tp;
            clock_gettime(CLOCK_MONOTONIC, &tp);
            return (tp.tv_sec * 1000000000LL + tp.tv_nsec);
        #endif
    }

    //����ϵͳ�߾��ȼ������ļ�ʱ(���μ���ֵ֮�ʱ���: ��΢��Ϊʱ�䵥λ)
    static inline uint64_t util_system_counter_time_us(uint64_t counter_end, uint64_t counter_start)
    {
        return (counter_end - counter_start) / 1000;
    }

    //����ϵͳ�߾��ȼ������ļ�ʱ(���μ���ֵ֮�ʱ���: ������Ϊʱ�䵥λ)
    static inline uint64_t util_system_counter_time_ns(uint64_t counter_end, uint64_t counter_start)
    {
        return counter_end - counter_start;
    }

    //�ڴ濽��
    static inline void util_memcpy(void *dest, const void *src, size_t n)
    {
        zrsocket_memcpy(dest, src, n);
    }

    //�ڴ濽��
    static inline void util_memcpy_char(void *dest, const void *src, size_t n)
    {
        //����1
        zrsocket_memcpy(dest, src, n);

        //����2: ���ܱȽϲ�
        //char *dest_char = (char *)dest;
        //char *src_char  = (char *)src;
        //while (n--) 
        //{
        //    *dest_char++ = *src_char++;
        //}
    }

    //ȡ�ô����
    static inline int util_get_errno()
    {
        return errno;
    }

    //ȡ�ñ���ϵͳ�����õ��ֽ���
    static inline int8_t util_get_byteorder()
    {
        static int8_t byte_order = 0;
        if (byte_order < 1)
        {
            const int e = 1;
            if (*(char *)&e)
            {  
                byte_order = __ZRSOCKET_LITTLE_ENDIAN;
            }
            else
            {
                byte_order = __ZRSOCKET_BIG_ENDIAN;
            }
        }
        return byte_order;
    }

    //64λ�����������ֽ���תΪ�����ֽ���
    static inline int64_t util_htonll(int64_t host_int64)
    {
        #if ZRSOCKET_BYTE_ORDER == __ZRSOCKET_BIG_ENDIAN
            return host_int64;
        #else
            #ifdef ZRSOCKET_OS_WINDOWS
                return ( (((unsigned long long)htonl(host_int64)) << 32) + htonl(host_int64 >> 32) );
            #else
                return bswap_64(host_int64);
            #endif
        #endif
    }

    //64λ�����������ֽ���תΪ�����ֽ���
    static inline int64_t util_ntohll(int64_t net_int64)
    {
        #if ZRSOCKET_BYTE_ORDER == __ZRSOCKET_BIG_ENDIAN
            return net_int64;
        #else
            #ifdef ZRSOCKET_OS_WINDOWS
                return ( (((unsigned long long)ntohl(net_int64)) << 32) + ntohl(net_int64 >> 32) );
            #else
                return bswap_64(net_int64);
            #endif
        #endif
    }

    // We can't just use _snprintf/snprintf as a drop-in replacement, because it
    // doesn't always NUL-terminate. :-(
    static inline int util_snprintf(char *str, size_t size, const char *format, ...)
    {
        if (0 == size)      // not even room for a \0?
        {
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

private:
    SystemApi()
    {
    }

    ~SystemApi()
    {
    }
};

ZRSOCKET_END

#endif
