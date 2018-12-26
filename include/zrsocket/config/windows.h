#ifndef ZRSOCKET_CONFIG_WINDOWS_H_
#define ZRSOCKET_CONFIG_WINDOWS_H_

#define ZRSOCKET_OS_WINDOWS
#if defined(ZRSOCKET_STATIC)
    #define ZRSOCKET_EXPORT
#else
    #if defined(ZRSOCKET_COMPILE_LIBRARY)
        #define ZRSOCKET_EXPORT __declspec(dllexport)
    #else
        #define ZRSOCKET_EXPORT __declspec(dllimport)
    #endif
#endif

/* FD_SETSIZE */
#ifndef FD_SETSIZE
    #define FD_SETSIZE 1024
#else
    #undef FD_SETSIZE
    #define FD_SETSIZE 1024
#endif

// Winsock库版本(默认winsock2)
#if !defined(ZRSOCKET_USE_WINSOCK2)
    #define ZRSOCKET_USE_WINSOCK2 1
#endif

#if defined(ZRSOCKET_USE_WINSOCK2) && (ZRSOCKET_USE_WINSOCK2 != 0)
    #define ZRSOCKET_WINSOCK_VERSION 2, 2
    // will also include windows.h, if not present
    #include <winsock2.h>
    #if (_WIN32_WINNT >= 0x0500)  //window 2000 or later version
        //这个两个头文件先后顺序不能改变,否则在vc2008以上版本编译通不过
        #include <mstcpip.h>
        #include <ws2tcpip.h>
    #endif
#else
    #define ZRSOCKET_WINSOCK_VERSION 1, 0
    #include <winsock.h>
#endif
#include <windows.h>

// Microsoft-specific extensions to the Windows Sockets API.
#include <mswsock.h>
#include <nspapi.h>

// link os system library
#pragma comment(lib, "wsock32.lib")
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "mswsock.lib")

typedef SOCKET                  ZRSOCKET_HANDLE;
typedef SOCKET                  ZRSOCKET_SOCKET;
typedef OVERLAPPED              ZRSOCKET_OVERLAPPED;

struct ZRSOCKET_IOVEC
{
    u_long  iov_len;
    char   *iov_base;
};
#define ZRSOCKET_MAX_IOVCNT     65536
#define ZRSOCKET_IOVCNT         64

#define ZRSOCKET_SOCKET_ERROR   SOCKET_ERROR
#define ZRSOCKET_INVALID_SOCKET INVALID_SOCKET
#define ZRSOCKET_SHUT_RD        SD_RECEIVE
#define ZRSOCKET_SHUT_WR        SD_SEND
#define ZRSOCKET_SHUT_RDWR      SD_BOTH
#define ZRSOCKET_SOCK_NONBLOCK  0

//一些常见Socket错误
#define ZRSOCKET_EAGAIN         WSAEWOULDBLOCK
#define ZRSOCKET_ENOBUFS        WSAENOBUFS
#define ZRSOCKET_EINVAL         WSAEINVAL
#define ZRSOCKET_EINTR          WSAEINTR
#define ZRSOCKET_ENOTSOCK       WSAENOTSOCK
#define ZRSOCKET_EISCONN        WSAEISCONN
#define ZRSOCKET_ENOTCONN       WSAENOTCONN
#define ZRSOCKET_ESHUTDOWN      WSAESHUTDOWN
#define ZRSOCKET_EMSGSIZE       WSAEMSGSIZE
#define ZRSOCKET_EWOULDBLOCK    WSAEWOULDBLOCK
#define ZRSOCKET_EINPROGRESS    WSAEINPROGRESS
#define ZRSOCKET_ECONNECTING    WSAEWOULDBLOCK
#define ZRSOCKET_IO_PENDING     WSA_IO_PENDING

#endif
