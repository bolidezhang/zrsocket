//**********************************************************************
//
// Copyright (C) 2005-2023 Bolide Zhang(bolidezhang@gmail.com)
// All rights reserved.
//
// This copy of zrsoket is licensed to you under the terms described 
// in the LICENSE.txt file included in this distribution.
//
//**********************************************************************

// Some compilers (e.g. VC++) benefit significantly from using this. 
// We've measured 3-4% build speed improvements in apps as a result 
#pragma once

#ifndef ZRSOCKET_GLOBAL_H
#define ZRSOCKET_GLOBAL_H
#include "event_loop.h"
#include "event_source.h"

ZRSOCKET_NAMESPACE_BEGIN

class Global
{
private:
    Global()  = default;
    ~Global() = default;
    Global(const Global &) = delete;
    Global & operator=(const Global &) = delete;

public:
    static Global & instance()
    {
        static Global global;
        return global;
    }
    
public:
    EventSource null_event_source_;
};

ZRSOCKET_NAMESPACE_END

#endif
