#pragma once
#include "zrsocket/zrsocket.h"

class BizStageHandler : public zrsocket::SedaStageHandler
{
public:
    BizStageHandler() = default;
    virtual ~BizStageHandler() = default;

    virtual int handle_event(const zrsocket::SedaEvent *event);

private:

};