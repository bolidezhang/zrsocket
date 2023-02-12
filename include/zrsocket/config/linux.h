// Some compilers (e.g. VC++) benefit significantly from using this. 
// We've measured 3-4% build speed improvements in apps as a result 
#pragma once

#ifndef ZRSOCKET_CONFIG_LINUX_H
#define ZRSOCKET_CONFIG_LINUX_H

#define ZRSOCKET_OS_LINUX
#ifdef ZRSOCKET_STATIC
    #define ZRSOCKET_EXPORT
#else
    #define ZRSOCKET_EXPORT __attribute__((visibility ("default")))
#endif

#include <cstring>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/uio.h>
#include <sys/socket.h>
#include <sys/times.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <signal.h>

typedef int   ZRSOCKET_HANDLE;
typedef int   ZRSOCKET_SOCKET;
typedef void  ZRSOCKET_OVERLAPPED;
typedef int   ZRSOCKET_FD;

typedef iovec ZRSOCKET_IOVEC;
#define ZRSOCKET_MAX_IOVCNT         1024
#define ZRSOCKET_IOVCNT             64
#define ZRSOCKET_TIMER_MIN_INTERVAL 5000    //5ms

#define ZRSOCKET_HAVE_PTHREADS
#define ZRSOCKET_SOCKET_ERROR       (-1)
#define ZRSOCKET_INVALID_SOCKET     (-1)
#define ZRSOCKET_SHUT_RD            SHUT_RD
#define ZRSOCKET_SHUT_WR            SHUT_WR
#define ZRSOCKET_SHUT_RDWR          SHUT_RDWR
#define ZRSOCKET_SOCK_NONBLOCK      SOCK_NONBLOCK

//一些常见Socket错误
#define ZRSOCKET_EAGAIN             EAGAIN
#define ZRSOCKET_ENOBUFS            ENOBUFS
#define ZRSOCKET_EINVAL             EINVAL
#define ZRSOCKET_EINTR              EINTR
#define ZRSOCKET_ENOTSOCK           ENOTSOCK
#define ZRSOCKET_EISCONN            EISCONN
#define ZRSOCKET_ENOTCONN           ENOTCONN
#define ZRSOCKET_ESHUTDOWN          ESHUTDOWN
#define ZRSOCKET_EMSGSIZE           EMSGSIZE
#define ZRSOCKET_EWOULDBLOCK        EWOULDBLOCK
#define ZRSOCKET_EINPROGRESS        EINPROGRESS
#define ZRSOCKET_ECONNECTING        EINPROGRESS
#define ZRSOCKET_IO_PENDING         EAGAIN

//accept4 os api 
#ifndef ZRSOCKET_NOT_HAVE_ACCEPT4
    #define ZRSOCKET_HAVE_ACCEPT4
#endif

//SO_REUSEPORT socket option 
#ifndef ZRSOCKET_NOT_HAVE_REUSREPORT
    #define ZRSOCKET_HAVE_REUSREPORT
#endif

//recvmmsg/sendmmsg os api 
#ifndef ZRSOCKET_NOT_HAVE_RECVSENDMMSG
    #define ZRSOCKET_HAVE_RECVSENDMMSG
#endif

#define zrsocket_fast_thread_local  __thread
#define ZRSOCKET_FAST_THREAD_LOCAL  zrsocket_fast_thread_local

#endif
