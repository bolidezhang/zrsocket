#ifndef ZRSOCKET_EVENT_REACTOR_H_
#define ZRSOCKET_EVENT_REACTOR_H_
#include "config.h"
#include "event_handler.h"

ZRSOCKET_BEGIN

class ZRSOCKET_EXPORT EventReactor
{
public:
    EventReactor()
    {
    }

    virtual ~EventReactor()
    {
    }
    
    virtual int init(uint_t num, uint_t max_events, int event_mode)
    {
        return 0;
    }

    virtual int set_buffer_size(int recv_buffer_size, int send_buffer_size) 
    { 
        return 0;
    }

    virtual int get_recv_buffer_size() const
    { 
        return 0;
    }

    virtual char* get_recv_buffer() 
    { 
        return NULL;
    }

    virtual ZRSOCKET_IOVEC* get_iovec()
    {
        return NULL;
    }

    virtual int get_iovec_count()
    {
        return 0;
    }

    virtual int open(uint_t max_size, int iovec_count, int max_timeout_ms, int flags)
    { 
        return 0;
    }

    virtual int close()
    { 
        return 0;
    }

    virtual int add_handler(EventHandler *handler, int event_mask)
    { 
        return 0;
    }

    virtual int del_handler(EventHandler *handler, int event_mask)
    { 
        return 0;
    }

    virtual int add_event(EventHandler *handler, int event_mask)
    {
        return 0;
    }

	virtual int del_event(EventHandler *handler, int event_mask)
	{
		return 0;
	}

	virtual int set_event(EventHandler *handler, int event_mask)
	{
		return 0;
	}

    virtual size_t get_handler_size() 
    { 
        return 0;
    }

    virtual int event_loop(int timeout_ms)
    { 
        return 0;
    }

    virtual int thread_start(int timeout_ms)
    { 
        return 0;
    }

    virtual int thread_join() 
    { 
        return 0;
    }

    virtual int thread_stop() 
    { 
        return 0;
    }
};

ZRSOCKET_END

#endif
