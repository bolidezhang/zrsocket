#ifndef ZRSOCKET_TCPSERVER_H_
#define ZRSOCKET_TCPSERVER_H_
#include "config.h"
#include "object_pool.h"
#include "thread.h"
#include "system_api.h"
#include "inet_addr.h"
#include "event_handler.h"
#include "event_reactor.h"
#include "event_source.h"
#include "tcpserver_handler.h"

ZRSOCKET_BEGIN

template <typename TClientHandler, typename TObjectPool, typename TServerHandler = TcpServerHandler>
class TcpServer : public EventSource
{
public:
    TcpServer()
        : accept_reactor_(NULL)
        , object_pool_(NULL)
    {
        handler_ = &server_handler_;
        set_config();
    }

    virtual ~TcpServer()
    { 
        close();
    }

    int open(ushort_t local_port, int backlog = 1024, const char *local_name = NULL, bool is_ipv6 = false)
    {
        close();

        int ret = local_addr_.set(local_name, local_port, is_ipv6);
        if (ret < 0)
        {
            return -1;
        }
        int address_family = is_ipv6 ? AF_INET6:AF_INET;
        ZRSOCKET_SOCKET fd = SystemApi::socket_open(address_family, SOCK_STREAM, IPPROTO_TCP);
        if (ZRSOCKET_INVALID_SOCKET == fd)
        {
            return -2;
        }
        server_handler_.init(fd, this, accept_reactor_, EventHandler::STATE_CONNECTED);
        if (server_handler_.handle_open() < 0)
        {
            ret = -3;
            goto EXCEPTION_EXIT_PROC;
        }

        SystemApi::socket_set_reuseaddr(fd, 1);
        ret = SystemApi::socket_bind(fd, local_addr_.get_addr(), local_addr_.get_addr_size());
        if (ret < 0)
        {
            ret = -4;
            goto EXCEPTION_EXIT_PROC;
        }
        ret = SystemApi::socket_listen(fd, backlog);
        if (ret < 0)
        {
            ret = -5;
            goto EXCEPTION_EXIT_PROC;
        }
        
        if (NULL != accept_reactor_)
        {
            if (accept_reactor_->add_handler(&server_handler_, EventHandler::READ_EVENT_MASK) < 0)
            {
                ret = -6;
                goto EXCEPTION_EXIT_PROC;
            }
        }
        else
        {
            if (accept_thread_num_ > 0)
            {
                SystemApi::socket_set_block(fd, true);
                if (accept_thread_group_.start(accept_thread_num_, accept_thread_num_, accept_function, this) < 0)
                {
                    ret = -7;
                    goto EXCEPTION_EXIT_PROC;
                }
            }
            else
            {
                if (reactor_->add_handler(&server_handler_, EventHandler::READ_EVENT_MASK) < 0)
                {
                    ret = -8;
                    goto EXCEPTION_EXIT_PROC;
                }
            }
        }
        return 0;

 EXCEPTION_EXIT_PROC:
        SystemApi::socket_close(fd);
        return ret;
    }

    int close()
    {
        accept_thread_group_.clear();
        server_handler_.close();
        return 0;
    }

    int set_config(uint_t   accept_thread_num = 1,
                   uint_t   max_connect_num   = 100000,
                   uint_t   recvbuffer_size   = 4096,
                   uint_t   msgbuffer_size    = 4096,
                   uint8_t  msglen_bytes      = 2,
                   uint8_t  msg_byteorder     = 0)
    {
        accept_thread_num_  = accept_thread_num;
        max_connect_num_    = max_connect_num;
        recvbuffer_size_    = recvbuffer_size;
        msgbuffer_size_     = msgbuffer_size;
        msglen_bytes_       = msglen_bytes;
        msg_byteorder_      = msg_byteorder;

        switch (msglen_bytes)
        {
            case 1:
                {
                    get_msglen_proc_  = EventSource::get_msglen_int8;
                    set_msglen_proc_  = EventSource::set_msglen_int8;
                }
                break;
            case 2:
                if (msg_byteorder)
                {
                    get_msglen_proc_ = EventSource::get_msglen_int16_network;
                    set_msglen_proc_ = EventSource::set_msglen_int16_network;
                }
                else
                {
                    get_msglen_proc_ = EventSource::get_msglen_int16_host;
                    set_msglen_proc_ = EventSource::set_msglen_int16_host;
                }
                break;
            case 4:
                if (msg_byteorder)
                {
                    get_msglen_proc_ = EventSource::get_msglen_int32_network;
                    set_msglen_proc_ = EventSource::set_msglen_int32_network;
                }
                else
                {
                    get_msglen_proc_ = EventSource::get_msglen_int32_host;
                    set_msglen_proc_ = EventSource::set_msglen_int32_host;
                }
                break;
            default:
                break;
        }

        return 0;
    }
    
    inline int set_interface(TObjectPool *object_pool, EventReactor *reactor, EventReactor *accept_reactor = NULL)
    {
        object_pool_    = object_pool;
        reactor_        = reactor;
        accept_reactor_ = accept_reactor;
        return 0;
    }

    TServerHandler* get_handler()
    {
        return &server_handler_;
    }

    InetAddr* get_local_addr()
    {
        return &local_addr_;
    }

    EventSource::SOURCE_TYPE source_type()
    {
        return EventSource::TYPE_SERVER;
    }

private:
    EventHandler * alloc_handler()
    {
        EventHandler *handler = object_pool_->pop();
        if (NULL != handler) {
            handler->in_objectpool_ = false;
            handler->handler_id_ = EventSource::generate_id();
            return handler;
        }
        return NULL;
    }

    int free_handler(EventHandler *handler)
    {
        if (handler != NULL) {
            if ((handler != &server_handler_) && (!handler->in_objectpool_)) {
                handler->close();
                handler->in_objectpool_ = true;
                object_pool_->push((TClientHandler *)handler);
                return 1;
            }
            return 0;
        }
        return -1;
    }

    static int accept_function(uint_t thread_index, void *arg)
    {
        TcpServer *tcpserver = (TcpServer *)arg;
        InetAddr iaddr(tcpserver->get_local_addr()->is_ipv6());
        sockaddr *addr  = iaddr.get_addr();     
        int addrlen     = iaddr.get_addr_size();

        ZRSOCKET_SOCKET client_fd;
        TServerHandler &server_handler = tcpserver->server_handler_;
        ZRSOCKET_SOCKET listen_fd = server_handler.socket();
        Thread *thread = tcpserver->accept_thread_group_.get_thread(thread_index);

        while (thread->state() == Thread::RUNNING) {
            client_fd = SystemApi::socket_accept(listen_fd, addr, &addrlen, ZRSOCKET_SOCK_NONBLOCK);
            if (ZRSOCKET_INVALID_SOCKET != client_fd) {
                if (server_handler.handle_accept(client_fd, iaddr) < 0) {
                    SystemApi::socket_close(client_fd);
                }
            }
            else {
                //关闭TcpServer::socket_,进而促使accept线程退出
                break;
            }
        }
        return 0;
    }

protected:
    InetAddr        local_addr_;
    uint_t          max_connect_num_;
    uint_t          accept_thread_num_;
    ThreadGroup     accept_thread_group_;
    EventReactor   *accept_reactor_;
    TObjectPool    *object_pool_;
    TServerHandler  server_handler_;
};

ZRSOCKET_END

#endif
