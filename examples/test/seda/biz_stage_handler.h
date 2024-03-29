#pragma once
#include "zrsocket/zrsocket.h"

class BizStageHandler : public zrsocket::SedaStageHandler
{
public:
    BizStageHandler() = default;
    virtual ~BizStageHandler() = default;

    virtual int handle_open();
    virtual int handle_event(const zrsocket::SedaEvent *event);

private:
    zrsocket::SedaTimer *counter_timer_ = nullptr;
    zrsocket::SedaTimer *counter_timer_s_ = nullptr;

    zrsocket::uint_t handle_num_ = 0;
};
