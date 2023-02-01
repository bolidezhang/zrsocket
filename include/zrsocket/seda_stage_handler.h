#ifndef ZRSOCKET_SEDA_STAGE_HANDLER_H
#define ZRSOCKET_SEDA_STAGE_HANDLER_H
#include "seda_interface.h"

ZRSOCKET_NAMESPACE_BEGIN

class SedaStageHandler : public ISedaStageHandler
{
public:
    SedaStageHandler() = default;
    virtual ~SedaStageHandler() = default;

    virtual int handle_open()
    {
        return 0;
    }

    virtual int handle_close()
    {
        return 0;
    }

    virtual int handle_event(const SedaEvent *event)
    {
        return 0;
    }

    inline void set_stage_thread(ISedaStageThread *stage_thread)
    {
        stage_thread_ = stage_thread;
    }

    inline ISedaStageThread * get_stage_thread() const
    {
        return stage_thread_;
    }

public:
    ISedaStageThread *stage_thread_ = nullptr;
};

ZRSOCKET_NAMESPACE_END

#endif
