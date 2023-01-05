// Some compilers (e.g. VC++) benefit significantly from using this. 
// We've measured 3-4% build speed improvements in apps as a result 
#pragma once

#ifndef SOCKETLITE_SOCKET_INET_ADDR_H
#define SOCKETLITE_SOCKET_INET_ADDR_H
#include "config.h"
#include "base_type.h"

ZRSOCKET_NAMESPACE_BEGIN

class ZRSOCKET_EXPORT InetAddr
{
public:
    InetAddr();
    InetAddr(bool ipv6);
    InetAddr(const InetAddr &addr);
    InetAddr(InetAddr &&addr) noexcept;
    ~InetAddr();

    InetAddr & operator= (const InetAddr &addr);
    InetAddr & operator= (InetAddr && addr) noexcept;

    void        reset();
    int         set(const char *hostname, ushort_t port, bool is_ipv6 = false);
    int         set(const struct sockaddr *addr, int addrlen);
    int         set(ZRSOCKET_SOCKET fd, bool ipv6);
    int         set(ZRSOCKET_SOCKET fd);

    operator    sockaddr * ();
    sockaddr *  get_addr() const;
    int         get_addr_size() const;
    int         get_ip_addr(char *ip_addr, int len) const;
    ushort_t    get_port_number() const;

    bool        is_ipv4() const;
    bool        is_ipv6() const;

    static int  get_ip_remote_s(ZRSOCKET_SOCKET fd, char *ip_addr, int ip_len, ushort_t *port_number);
    static int  get_ip_local_s(ZRSOCKET_SOCKET fd, char *ip_addr, int ip_len, ushort_t *port_number);
    static int  get_ip_s(const struct sockaddr *addr, int addrlen, char *ip_addr, int ip_len, ushort_t *port_number);
    static int  get_addr_s(const char *hostname, ushort_t port, sockaddr *addr, int addrlen);

private:
    union
    {
        sockaddr_in6 ipv6;
        sockaddr_in  ipv4;
    } addr_;

    enum AddrType
    {
        IPV6,
        IPV4,
    } addr_type_;
};

ZRSOCKET_NAMESPACE_END

#endif
