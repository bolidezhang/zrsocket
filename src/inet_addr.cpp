#include "zrsocket/inet_addr.h"
#include "zrsocket/memory.h"
#include <stdio.h>
#include <cstdio>

ZRSOCKET_BEGIN

InetAddr::InetAddr()
    : addr_type_(IPV4)
{
    reset();
}

InetAddr::InetAddr(bool ipv6)
{
    addr_type_ = ipv6 ? IPV6:IPV4;
    reset();
}

InetAddr::~InetAddr()
{
}

void InetAddr::reset()
{
    if (addr_type_ == IPV4) {
        inet_addr_.in4.sin_family = AF_INET;
    }
#ifdef ZRSOCKET_IPV6
    else if (addr_type_ == IPV6) {
        inet_addr_.in6.sin6_family = AF_INET6;
    }
#endif
}

InetAddr::operator sockaddr* ()
{
#ifdef ZRSOCKET_IPV6
    if (addr_type_ == IPV4) {
        return (sockaddr *)&inet_addr_.in4;
    }
    else {
        return (sockaddr *)&inet_addr_.in6;
    }
#else
    return (sockaddr *)&inet_addr_.in4;
#endif
}

sockaddr* InetAddr::get_addr() const
{
#ifdef ZRSOCKET_IPV6
    if (addr_type_ == IPV4) {
        return (sockaddr *)&inet_addr_.in4;
    }
    else {
        return (sockaddr *)&inet_addr_.in6;
    }
#else
    return (sockaddr *)&inet_addr_.in4;
#endif
}

int InetAddr::get_addr_size() const
{
#ifdef ZRSOCKET_IPV6
    if (addr_type_ == IPV4) {
        return sizeof(inet_addr_.in4);
    }
    else {
        return sizeof(inet_addr_.in6);
    }
#else
    return sizeof(inet_addr_.in4);
#endif
}

//ʵ�ַ�ʽ:
// 1)WSAAddressToString(windows)
// 2)inet_ntoa(ipv4) 
// 3)inet_ntop(linux)
// 4)getnameinfo
int InetAddr::get_ip_addr(char *ip_addr, int len) const
{
    int ret = 0;
#ifdef ZRSOCKET_IPV6
    if (addr_type_ == IPV4) {
        ret = getnameinfo((sockaddr *)&inet_addr_.in4, sizeof(inet_addr_.in4), ip_addr, len, 0, 0, NI_NUMERICHOST);
    }
    else {
        ret = getnameinfo((sockaddr *)&inet_addr_.in6, sizeof(inet_addr_.in6), ip_addr, len, 0, 0, NI_NUMERICHOST);
    }
#else
    ret = getnameinfo((sockaddr *)&inet_addr_.in4, sizeof(inet_addr_.in4), ip_addr, len, 0, 0, NI_NUMERICHOST);
#endif
    return ret;
}

ushort_t InetAddr::get_port_number() const
{
#ifdef ZRSOCKET_IPV6
    if (addr_type_ == IPV4) {
        return ntohs(inet_addr_.in4.sin_port);
    }
    else {
        return ntohs(inet_addr_.in6.sin6_port);
    }
#else
    return ntohs(inet_addr_.in4.sin_port);
#endif
}

bool InetAddr::is_ipv4() const
{
    return addr_type_ == IPV4;
}

bool InetAddr::is_ipv6() const
{
    return addr_type_ == IPV6;
}

//ʵ�ַ�ʽ��
// 1)getaddrinfo
// 2)gethostbyname(����ʹ��getaddrinfo, �˺����ѱ�����deprecated)
int InetAddr::set(const char *hostname, ushort_t port, bool is_ipv6)
{
    char str_port[7] = {0};
    snprintf(str_port, sizeof(str_port), "%d", port);
#ifdef ZRSOCKET_IPV6    
    struct addrinfo hints = {0};
    struct addrinfo *ret = NULL;

    if (!is_ipv6) {
        addr_type_ = IPV4;
        hints.ai_flags  = AI_PASSIVE;
        hints.ai_family = AF_INET;
        inet_addr_.in4.sin_family = AF_INET;
        inet_addr_.in4.sin_port = htons(port);
    }
    else {
        addr_type_ = IPV6;
        hints.ai_flags  = AI_PASSIVE;
        hints.ai_family = AF_INET6;
        inet_addr_.in6.sin6_family = AF_INET6;
        inet_addr_.in6.sin6_port = htons(port);
    }
    int error = getaddrinfo(hostname, str_port, &hints, &ret);
    if (error != 0) {
        if (ret != NULL) {
            freeaddrinfo(ret);
        }
        return -1;
    }    
    if (ret->ai_family == AF_INET) {
        zrsocket_memcpy(&inet_addr_.in4, ret->ai_addr, ret->ai_addrlen);
        inet_addr_.in4.sin_port = htons(port);
    }
    else {
        zrsocket_memcpy(&inet_addr_.in6, ret->ai_addr, ret->ai_addrlen);
        inet_addr_.in6.sin6_port = htons(port);
    }
    freeaddrinfo(ret);
#else
    struct addrinfo hints = {0};
    struct addrinfo *ret = NULL;
    addr_type_ = IPV4;
    hints.ai_flags  = AI_PASSIVE;
    hints.ai_family = AF_INET;
    inet_addr_.in4.sin_family = AF_INET;
    inet_addr_.in4.sin_port = htons(port);
    int error = getaddrinfo(hostname, str_port, &hints, &ret);
    if (error != 0) {
        if (ret != NULL) {
            freeaddrinfo(ret);
        }
        return -1;
    }    
    zrsocket_memcpy(&inet_addr_.in4, ret->ai_addr, ret->ai_addrlen);
    inet_addr_.in4.sin_port = htons(port);
#endif
    return 0;
}

int InetAddr::set(const struct sockaddr *addr, int addrlen)
{
#ifdef ZRSOCKET_IPV6
    if (addr->sa_family == AF_INET) {
        addr_type_ = IPV4;
        zrsocket_memcpy(&inet_addr_.in4, addr, addrlen);
    }
    else {
        addr_type_ = IPV6;
        zrsocket_memcpy(&inet_addr_.in6, addr, addrlen);
    }
#else
    addr_type_ = IPV4;
    zrsocket_memcpy(&inet_addr_.in4, addr, addrlen);
#endif
    return 0;
}

int InetAddr::set(ZRSOCKET_SOCKET fd, bool ipv6)
{
    int addrlen;
#ifdef ZRSOCKET_IPV6
    if (!ipv6) {
        addr_type_ = IPV4;
        addrlen = sizeof(inet_addr_.in4);
        return getpeername(fd, (sockaddr *)&inet_addr_.in4, (socklen_t *)&addrlen);
    }
    else {
        addr_type_= IPV6;
        addrlen = sizeof(inet_addr_.in6);
        return getpeername(fd, (sockaddr *)&inet_addr_.in6, (socklen_t *)&addrlen);
    }
#else
    addr_type_ = IPV4;
    addrlen = sizeof(inet_addr_.in4);
    return getpeername(fd, (sockaddr *)&inet_addr_.in4, (socklen_t *)&addrlen);
#endif
}

int InetAddr::set(ZRSOCKET_SOCKET fd)
{
    sockaddr_storage ss;
    int ss_len = sizeof(sockaddr_storage);
    int ret = getpeername(fd, (sockaddr *)&ss, (socklen_t *)&ss_len);
    if (ret != 0) {
        return ret;
    }

#ifdef ZRSOCKET_IPV6
    if (ss.ss_family == AF_INET) { //IPV4
        addr_type_ = IPV4;
        zrsocket_memcpy(&inet_addr_.in4, &ss, sizeof(inet_addr_.in4));
    }
    else { //IPV6
        addr_type_ = IPV6;
        zrsocket_memcpy(&inet_addr_.in6, &ss, sizeof(inet_addr_.in6));
    }            
#else
    addr_type_ = IPV4;
    zrsocket_memcpy(&inet_addr_.in4, &ss, sizeof(inet_addr_.in4));
#endif
    return 0;
}

int InetAddr::get_ip_remote_s(ZRSOCKET_SOCKET fd, char *ip_addr, int ip_len, ushort_t *port_number)
{
    sockaddr_storage ss;
    int ss_len = sizeof(sockaddr_storage);
    int ret = getpeername(fd, (sockaddr *)&ss,(socklen_t *)&ss_len);
    if (ret != 0) {
        return ret;
    }
    if (port_number != 0) {
#ifdef ZRSOCKET_IPV6
        if (ss.ss_family == AF_INET) { //IPV4
            sockaddr_in *in4 = (sockaddr_in *)&ss;
            *port_number = ntohs(in4->sin_port);
        }
        else { //IPV6
            sockaddr_in6 *in6 = (sockaddr_in6 *)&ss;
            *port_number = ntohs(in6->sin6_port);
        }            
#else
        sockaddr_in *in4 = (sockaddr_in *)&ss;
        *port_number = ntohs(in4->sin_port);
#endif
    }
    ret = getnameinfo((sockaddr *)&ss, ss_len, ip_addr, ip_len, 0, 0, NI_NUMERICHOST);
    return ret;
}

int InetAddr::get_ip_local_s(ZRSOCKET_SOCKET fd, char *ip_addr, int ip_len, ushort_t *port_number)
{
    sockaddr_storage ss;
    int ss_len = sizeof(sockaddr_storage);
    int ret  = getsockname(fd, (sockaddr *)&ss, (socklen_t *)&ss_len);
    if (ret != 0) {
        return ret;
    }
    if (port_number != 0) {
#ifdef ZRSOCKET_IPV6
        if (ss.ss_family == AF_INET) { //IPV4
            sockaddr_in *in4 = (sockaddr_in *)&ss;
            *port_number = ntohs(in4->sin_port);
        }
        else { //IPV6
            sockaddr_in6 *in6 = (sockaddr_in6 *)&ss;
            *port_number = ntohs(in6->sin6_port);
        }            
#else
        sockaddr_in *in4 = (sockaddr_in *)&ss;
        *port_number = ntohs(in4->sin_port);
#endif
    }
    ret = getnameinfo((sockaddr *)&ss, ss_len, ip_addr, ip_len, 0, 0, NI_NUMERICHOST);
    return ret;
}

int InetAddr::get_ip_s(const struct sockaddr *addr, int addrlen, char *ip_addr, int ip_len, ushort_t *port_number)
{
    int ret = 0;
    if (port_number != 0) {
#ifdef ZRSOCKET_IPV6
        if (addr->sa_family == AF_INET) { //IPV4
            sockaddr_in *in4 = (sockaddr_in *)addr;
            *port_number = ntohs(in4->sin_port);
        }
        else { //IPV6
            sockaddr_in6 *in6 = (sockaddr_in6 *)addr;
            *port_number = ntohs(in6->sin6_port);
        }            
#else
        sockaddr_in *in4 = (sockaddr_in *)addr;
        *port_number = ntohs(in4->sin_port);
#endif
    }
    ret = getnameinfo(addr, addrlen, ip_addr, ip_len, 0, 0, NI_NUMERICHOST);
    return ret;    
}

int InetAddr::get_addr_s(const char *hostname, ushort_t port, sockaddr *addr, int addrlen)
{
    struct addrinfo hints = {0};
    struct addrinfo *ret = NULL;
    int error = 0;
    if (addrlen == sizeof(sockaddr_in)) {
        hints.ai_family     = AF_INET;
        sockaddr_in *in4    = (sockaddr_in *)addr;
        in4->sin_port       = ntohs(port);
    }
    else {
        hints.ai_family     = AF_INET6;
        sockaddr_in6 *in6   = (sockaddr_in6 *)addr;
        in6->sin6_port      = ntohs(port);
    }
    if (hostname == NULL) {
        return 0;
    }

    error = getaddrinfo(hostname, 0, &hints, &ret);
    if (error != 0) {
        if (ret != NULL) {
            freeaddrinfo(ret);
        }
        return -1;
    }    
    zrsocket_memcpy(addr, ret->ai_addr, ret->ai_addrlen);
    freeaddrinfo(ret);
    if (addr->sa_family == AF_INET) {
        sockaddr_in *in4    = (sockaddr_in *)addr;
        in4->sin_port       = ntohs(port);
    }
    else {
        sockaddr_in6 *in6   = (sockaddr_in6 *)addr;
        in6->sin6_port      = ntohs(port);
    }
    return 0;
}

ZRSOCKET_END
