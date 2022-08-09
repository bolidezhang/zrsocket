// Some compilers (e.g. VC++) benefit significantly from using this. 
// We've measured 3-4% build speed improvements in apps as a result 
#pragma once

#ifndef ZRSOCKET_SYSTEM_CONSTANT_H
#define ZRSOCKET_SYSTEM_CONSTANT_H
#include "config.h"
#include "base_type.h"
#include "os_api.h"

ZRSOCKET_NAMESPACE_BEGIN

class OSConstant
{
public:
    static OSConstant & instance()
    {
        static OSConstant osc;
        return osc;
    }
      
    inline uint64_t os_counter_frequency()
    {
        return os_counter_frequency_;
    }

    inline ByteOrder byte_order()
    {
        return byte_order_;
    }

private:
    OSConstant()
    {
        init();
    }

    ~OSConstant() = default;
    OSConstant(const OSConstant &) = delete;
    OSConstant & operator=(const OSConstant &) = delete;

    inline void init()
    {
        os_counter_frequency_ = OSApi::os_counter_frequency();
        byte_order_ = OSApi::byte_order();
    }

private:
    uint64_t  os_counter_frequency_;
    ByteOrder byte_order_;
};

ZRSOCKET_NAMESPACE_END

#endif
