#include "zrsocket/inet_addr.h"
#include "zrsocket/memory.h"
#include <cstdio>

ZRSOCKET_NAMESPACE_BEGIN

InetAddr::InetAddr()
    : addr_type_(AddrType::IPV4)
{
    reset();
}

InetAddr::InetAddr(bool ipv6)
{
    addr_type_ = ipv6 ? IPV6:IPV4;
    reset();
}

InetAddr::InetAddr(const InetAddr &addr)
{
    addr_type_ = addr.addr_type_;
    switch (addr_type_) {
        case AddrType::IPV4:
            memcpy(&(addr_.ipv4), &(addr.addr_.ipv4), sizeof(addr.addr_.ipv4));
            break;
        case AddrType::IPV6:
            memcpy(&(addr_.ipv6), &(addr.addr_.ipv6), sizeof(addr.addr_.ipv6));
            break;
        default:
            memcpy(&(addr_.ipv4), &(addr.addr_.ipv4), sizeof(addr.addr_.ipv4));
            break;
    };
}

InetAddr::InetAddr(InetAddr &&addr) noexcept
{
    addr_type_ = addr.addr_type_;
    switch (addr_type_) {
        case AddrType::IPV4:
            memcpy(&(addr_.ipv4), &(addr.addr_.ipv4), sizeof(addr.addr_.ipv4));
            break;
        case AddrType::IPV6:
            memcpy(&(addr_.ipv6), &(addr.addr_.ipv6), sizeof(addr.addr_.ipv6));
            break;
        default:
            memcpy(&(addr_.ipv4), &(addr.addr_.ipv4), sizeof(addr.addr_.ipv4));
            break;
    };
}

InetAddr::~InetAddr()
{
}

InetAddr & InetAddr::operator= (const InetAddr &addr)
{
    addr_type_ = addr.addr_type_;
    switch (addr_type_) {
        case AddrType::IPV4:
            memcpy(&(addr_.ipv4), &(addr.addr_.ipv4), sizeof(addr.addr_.ipv4));
            break;
        case AddrType::IPV6:
            memcpy(&(addr_.ipv6), &(addr.addr_.ipv6), sizeof(addr.addr_.ipv6));
            break;
        default:
            memcpy(&(addr_.ipv4), &(addr.addr_.ipv4), sizeof(addr.addr_.ipv4));
            break;
    };

    return *this;
}

InetAddr & InetAddr::operator= (InetAddr &&addr) noexcept
{
    addr_type_ = addr.addr_type_;
    switch (addr_type_) {
        case AddrType::IPV4:
            memcpy(&(addr_.ipv4), &(addr.addr_.ipv4), sizeof(addr.addr_.ipv4));
            break;
        case AddrType::IPV6:
            memcpy(&(addr_.ipv6), &(addr.addr_.ipv6), sizeof(addr.addr_.ipv6));
            break;
        default:
            memcpy(&(addr_.ipv4), &(addr.addr_.ipv4), sizeof(addr.addr_.ipv4));
            break;
    };

    return *this;
}

void InetAddr::reset()
{
    switch (addr_type_) {
        case AddrType::IPV4:
            addr_.ipv4.sin_family = AF_INET;
            break;
        case AddrType::IPV6:
            addr_.ipv6.sin6_family = AF_INET6;
            break;
        default:
            addr_.ipv4.sin_family = AF_INET;
            break;
    }
}

InetAddr::operator sockaddr * ()
{
    switch (addr_type_) {
        case AddrType::IPV4:
            return (sockaddr *)&addr_.ipv4;
        case AddrType::IPV6:
            return (sockaddr *)&addr_.ipv6;
        default:
            return (sockaddr *)&addr_.ipv4;
    }
}

sockaddr * InetAddr::get_addr() const
{
    switch (addr_type_) {
        case AddrType::IPV4:
            return (sockaddr *)&addr_.ipv4;
        case AddrType::IPV6:
            return (sockaddr *)&addr_.ipv6;
        default:
            return (sockaddr *)&addr_.ipv4;
    }
}

int InetAddr::get_addr_size() const
{
    switch (addr_type_) {
        case AddrType::IPV4:
            return sizeof(addr_.ipv4);
        case AddrType::IPV6:
            return sizeof(addr_.ipv6);
        default:
            return sizeof(addr_.ipv4);
    }
}

//实现方式:
// 1)WSAAddressToString(windows)
// 2)inet_ntoa(ipv4) 
// 3)inet_ntop(linux)
// 4)getnameinfo
int InetAddr::get_ip_addr(char *ip_addr, int len) const
{
    switch (addr_type_) {
        case AddrType::IPV4:
            return getnameinfo((sockaddr*)&addr_.ipv4, sizeof(addr_.ipv4), ip_addr, len, 0, 0, NI_NUMERICHOST);
        case AddrType::IPV6:
            return getnameinfo((sockaddr*)&addr_.ipv6, sizeof(addr_.ipv6), ip_addr, len, 0, 0, NI_NUMERICHOST);
        default:
            return getnameinfo((sockaddr*)&addr_.ipv4, sizeof(addr_.ipv4), ip_addr, len, 0, 0, NI_NUMERICHOST);
    }
}

ushort_t InetAddr::get_port_number() const
{
    switch (addr_type_) {
        case AddrType::IPV4:
            return ntohs(addr_.ipv4.sin_port);
        case AddrType::IPV6:
            return ntohs(addr_.ipv6.sin6_port);
        default:
            return ntohs(addr_.ipv4.sin_port);
    }
}

bool InetAddr::is_ipv4() const
{
    return addr_type_ == IPV4;
}

bool InetAddr::is_ipv6() const
{
    return addr_type_ == IPV6;
}

//实现方式：
// 1)getaddrinfo
// 2)gethostbyname(建议使用getaddrinfo, 此函数已被弃用deprecated)
int InetAddr::set(const char *hostname, ushort_t port, bool is_ipv6)
{
    char str_port[7] = {0};
    snprintf(str_port, sizeof(str_port), "%d", port);
    struct addrinfo hints = { 0 };
    struct addrinfo *res = nullptr;

    hints.ai_flags = AI_PASSIVE;
    if (!is_ipv6) {
        hints.ai_family = AF_INET;
    }
    else {
        hints.ai_family = AF_INET6;
    }

    if (!getaddrinfo(hostname, str_port, &hints, &res)) {
        if (res->ai_family == AF_INET) {
            addr_type_ = IPV4;
            zrsocket_memcpy(&addr_.ipv4, res->ai_addr, res->ai_addrlen);
        }
        else {
            addr_type_ = IPV6;
            zrsocket_memcpy(&addr_.ipv6, res->ai_addr, res->ai_addrlen);
        }
    }

    if (res != nullptr) {
        freeaddrinfo(res);
        return 0;
    }
    return -1;
}

int InetAddr::set(const struct sockaddr *addr, int addrlen)
{
    if (addr->sa_family == AF_INET) {
        addr_type_ = IPV4;
        zrsocket_memcpy(&addr_.ipv4, addr, addrlen);
    }
    else {
        addr_type_ = IPV6;
        zrsocket_memcpy(&addr_.ipv6, addr, addrlen);
    }
    return 0;
}

int InetAddr::set(ZRSOCKET_SOCKET fd, bool ipv6)
{
    int addrlen;
    if (!ipv6) {
        addr_type_ = IPV4;
        addrlen = sizeof(addr_.ipv4);
        return getpeername(fd, (sockaddr *)&addr_.ipv4, (socklen_t *)&addrlen);
    }
    else {
        addr_type_= IPV6;
        addrlen = sizeof(addr_.ipv6);
        return getpeername(fd, (sockaddr *)&addr_.ipv6, (socklen_t *)&addrlen);
    }
}

int InetAddr::set(ZRSOCKET_SOCKET fd)
{
    sockaddr_storage ss;
    int ss_len = sizeof(sockaddr_storage);
    int ret = getpeername(fd, (sockaddr *)&ss, (socklen_t *)&ss_len);
    if (ret != 0) {
        return ret;
    }

    if (ss.ss_family == AF_INET) { //IPV4
        addr_type_ = IPV4;
        zrsocket_memcpy(&addr_.ipv4, &ss, sizeof(addr_.ipv4));
    }
    else { //IPV6
        addr_type_ = IPV6;
        zrsocket_memcpy(&addr_.ipv6, &ss, sizeof(addr_.ipv6));
    }            
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
        if (ss.ss_family == AF_INET) { //IPV4
            sockaddr_in *ipv4 = (sockaddr_in *)&ss;
            *port_number = ntohs(ipv4->sin_port);
        }
        else { //IPV6
            sockaddr_in6 *ipv6 = (sockaddr_in6 *)&ss;
            *port_number = ntohs(ipv6->sin6_port);
        }            
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
        if (ss.ss_family == AF_INET) { //IPV4
            sockaddr_in *ipv4 = (sockaddr_in *)&ss;
            *port_number = ntohs(ipv4->sin_port);
        }
        else { //IPV6
            sockaddr_in6 *ipv6 = (sockaddr_in6 *)&ss;
            *port_number = ntohs(ipv6->sin6_port);
        }            
    }
    ret = getnameinfo((sockaddr *)&ss, ss_len, ip_addr, ip_len, 0, 0, NI_NUMERICHOST);
    return ret;
}

int InetAddr::get_ip_s(const struct sockaddr *addr, int addrlen, char *ip_addr, int ip_len, ushort_t *port_number)
{
    int ret = 0;
    if (port_number != 0) {
        if (addr->sa_family == AF_INET) { //IPV4
            sockaddr_in *ipv4 = (sockaddr_in *)addr;
            *port_number = ntohs(ipv4->sin_port);
        }
        else { //IPV6
            sockaddr_in6 *ipv6 = (sockaddr_in6 *)addr;
            *port_number = ntohs(ipv6->sin6_port);
        }            
    }
    ret = getnameinfo(addr, addrlen, ip_addr, ip_len, 0, 0, NI_NUMERICHOST);
    return ret;    
}

int InetAddr::get_addr_s(const char *hostname, ushort_t port, sockaddr *addr, int addrlen)
{
    struct addrinfo hints = {0};
    struct addrinfo *res = nullptr;
    if (addrlen == sizeof(sockaddr_in)) {
        hints.ai_family     = AF_INET;
        sockaddr_in *ipv4   = (sockaddr_in *)addr;
        ipv4->sin_port      = ntohs(port);
    }
    else {
        hints.ai_family     = AF_INET6;
        sockaddr_in6 *ipv6  = (sockaddr_in6 *)addr;
        ipv6->sin6_port     = ntohs(port);
    }
    if (nullptr == hostname) {
        return 0;
    }

    if (!getaddrinfo(hostname, nullptr, &hints, &res)) {
        zrsocket_memcpy(addr, res->ai_addr, res->ai_addrlen);
        freeaddrinfo(res);
        if (addr->sa_family == AF_INET) {
            sockaddr_in *ipv4 = (sockaddr_in *)addr;
            ipv4->sin_port = ntohs(port);
        }
        else {
            sockaddr_in6 *ipv6 = (sockaddr_in6 *)addr;
            ipv6->sin6_port = ntohs(port);
        }
    }

    if (res != nullptr) {
        freeaddrinfo(res);
        return 0;
    }
    return -1;
}

ZRSOCKET_NAMESPACE_END
