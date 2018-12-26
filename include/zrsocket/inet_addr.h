#ifndef SOCKETLITE_SOCKET_INET_ADDR_H
#define SOCKETLITE_SOCKET_INET_ADDR_H
#include "config.h"
#include "base_type.h"

ZRSOCKET_BEGIN

class ZRSOCKET_EXPORT InetAddr
{
public:
    InetAddr();
    InetAddr(bool ipv6);
    ~InetAddr();

    void            reset();
    int             set(const char *hostname, ushort_t port, bool is_ipv6 = false);
    int             set(const struct sockaddr *addr, int addrlen);
    int             set(ZRSOCKET_SOCKET fd, bool ipv6);
    int             set(ZRSOCKET_SOCKET fd);

    operator        sockaddr* ();
    sockaddr*       get_addr() const;
    int             get_addr_size() const;
    int             get_ip_addr(char *ip_addr, int len) const;
    ushort_t        get_port_number() const;

    bool            is_ipv4() const;
    bool            is_ipv6() const;

    static int      get_ip_remote_s(ZRSOCKET_SOCKET fd, char *ip_addr, int ip_len, ushort_t *port_number);
    static int      get_ip_local_s(ZRSOCKET_SOCKET fd, char *ip_addr, int ip_len, ushort_t *port_number);
    static int      get_ip_s(const struct sockaddr *addr, int addrlen, char *ip_addr, int ip_len, ushort_t *port_number);
    static int      get_addr_s(const char *hostname, ushort_t port, sockaddr *addr, int addrlen);

private:
    enum
    {
        IPV4,
        IPV6
    }addr_type_;

    union
    {
        sockaddr_in  in4;
#ifdef ZRSOCKET_IPV6
        sockaddr_in6 in6;
#endif
    }inet_addr_;

};

ZRSOCKET_END

#endif
