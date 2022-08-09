// Some compilers (e.g. VC++) benefit significantly from using this. 
// We've measured 3-4% build speed improvements in apps as a result 
#pragma once

#ifndef ZRSOCKET_SINGLETON_H
#define ZRSOCKET_SINGLETON_H

//单例模式的线程安全的实现方法: 
//  1.局部静态成员(c++11以前不是线程安全的, c++11以后是线程安全的)
//  2.采用std::call_once/std::once_flag

template <class T>
class Singleton
{
public:
    static T & instance()
    {
        static T object_type;
        return object_type;
    }

private:
    Singleton() = default;
    ~Singleton() = default;

    Singleton(const Singleton &) = delete;
    Singleton & operator=(const Singleton &) = delete;
};

#endif
