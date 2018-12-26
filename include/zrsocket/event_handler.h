#ifndef ZRSOCKET_EVENT_HANDLER_H_
#define ZRSOCKET_EVENT_HANDLER_H_
#include "config.h"
#include "base_type.h"
#include "byte_buffer.h"
#include "system_api.h"

ZRSOCKET_BEGIN

class EventSource;
class EventReactor;
template <typename TMutex> class SelectReactor;
template <typename TMutex> class EpollReactor;
template <typename TEventReactor> class EventReactorGroup;
template <typename TClientHandler, typename TObjectPool, typename TServerHandler> class TcpServer;

class ZRSOCKET_EXPORT EventHandler
{
public:
    enum EVENT_MASK
    {
        NULL_EVENT_MASK         = 0x00000000,
        READ_EVENT_MASK         = 0x00000001,
        WRITE_EVENT_MASK        = 0x00000002,
        EXCEPT_EVENT_MASK       = 0x00000004,
        TIMEOUT_EVENT_MASK      = 0x00000008,
        ALL_EVENT_MASK          = READ_EVENT_MASK | WRITE_EVENT_MASK | EXCEPT_EVENT_MASK  | TIMEOUT_EVENT_MASK,
    };

    enum HANDLER_STATE
    {
        STATE_CLOSED            = 0x00,             //�ر�
        STATE_OPENED            = 0x01,             //��
        STATE_CONNECTED         = 0x02,             //���ӳɹ�,���շ�����
        STATE_CLOSE_WAIT        = 0x04,             //�ȴ��ر�(Ӧ�ò�����Ҫ�ر�socket,socket��δ�ر�)
        STATE_CLOSE_SEND        = 0x08,             //�رշ���ͨ��(socket�ѹرշ���ͨ��,���ɽ�������)
        STATE_CLOSE_RECV        = 0x10,             //�رս���ͨ��(socket�ѹرս���ͨ��,���ɷ�������)
    };

    enum HANDLE_ERROR
    {
        ERROR_START_NUMBER      = 1000000,

        ERROR_CLOSE_PASSIVE     = 1000001,          //�����ر�(Զ�˹ر�remote)
        ERROR_CLOSE_ACTIVE      = 1000002,          //�����ر�(���عر�local)
        ERROR_CLOSE_RECV        = 1000003,          //����ʱ�������ر�
        ERROR_CLOSE_SEND        = 1000004,          //�������ݳ���ر�
        ERROR_KEEPALIVE_TIMEOUT = 1000005,          //keepalive��ʱ�ر�

        ERROR_END_NUMBER        = 1009999,
    };

    inline EventHandler()
        : source_(NULL)
        , reactor_(NULL)
        , socket_(ZRSOCKET_INVALID_SOCKET)
        , handler_id_(0)
        , last_update_time_(0)
        , last_errno_(0)
        , event_mask_(READ_EVENT_MASK)
        , state_(STATE_CLOSED)
        , in_reactor_(false)
        , in_objectpool_(true)
    {
    }

    virtual ~EventHandler()
    {
    }

    inline ZRSOCKET_SOCKET socket() const
    {
        return socket_;
    }

    inline EventSource* get_source() const
    {
        return source_;
    }

    inline EventReactor* get_reactor() const
    {
        return reactor_;
    }
    
    inline uint64_t handler_id() const
    {
        return handler_id_;
    }

    void init(ZRSOCKET_SOCKET fd, EventSource *source, EventReactor *reactor, HANDLER_STATE state)
    {
        socket_ = fd;
        source_ = source;
        reactor_ = reactor;
        last_errno_ = 0;
        state_ = state;
#ifndef ZRSOCKET_USE_ACCEPT4
        SystemApi::socket_set_block(fd, false);
#endif
    }

    virtual int handle_open()
    {
        return 0;
    }

    virtual int handle_close()
    {
        return 0;
    }

    virtual int handle_read()
    { 
        return 0;
    }

    virtual int handle_write()
    {
        return 0;
    }

    virtual int handle_connect()
    {
        return 0;
    }

    virtual int write_data()
    {
        return 0;
    }

    inline int8_t status()
    {
        return state_;
    }

    int event_mask()
    {
        return event_mask_;
    }

    inline void close()
    {
        SystemApi::socket_close(socket_);
        socket_ = ZRSOCKET_INVALID_SOCKET;
        state_  = STATE_CLOSED;
    }

 protected:
    EventSource    *source_;
    EventReactor   *reactor_;
    ZRSOCKET_SOCKET socket_;

    uint64_t        handler_id_;
    uint64_t        last_update_time_;
    int             last_errno_;        //�������

private:
    int             event_mask_;        //�¼���(�ϲ㲻���޸�)
    int8_t          state_;             //��ǰ״̬
    bool            in_reactor_;        //�Ƿ����reactor_��(�ϲ㲻���޸�)
    bool            in_objectpool_;     //�Ƿ���objectpool��(�ϲ㲻���޸�)

    template <typename TMutex> friend class SelectReactor;
    template <typename TMutex> friend class EpollReactor;
    template <typename TEventReactor> friend class EventReactorGroup;
    template <typename TClientHandler, typename TObjectPool, typename TServerHandler> friend class TcpServer;
};

ZRSOCKET_END

#endif
