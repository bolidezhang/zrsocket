// Some compilers (e.g. VC++) benefit significantly from using this. 
// We've measured 3-4% build speed improvements in apps as a result 
#pragma once

// low-level i/o 
//   os system file api

// file i/o api type:
//  0) system call:
//      STDIN_FILENO:  0  标准输入      keyboard(defaut)
//      STDOUT_FILENO: 1  标准输出      screen(defaut)
//      STDERR_FILENO: 2  标准错误输出  screen(defaut)
//      open / close / read / readv / write / writev / lseek / fsync /fdatasync ...
//  1）c-style cstdio
//      带缓冲区file io
//      stdin, stdout, stderr
//      std::FILE *
//      fopen / fclose / fflush / fread / fwrite / fprintf / fgets / fputs
//      fflush / setbuf / setvbuf
//      fprintf / printf / sprintf / snprintf
//      ftell / fseek / fseekpos ...
//  2) c++ iostream
//      https://en.cppreference.com/w/cpp/io
//      ios_base
//      std::cin / std::cout / std::cerr  三个全局变量

#ifndef ZRSOCKET_OS_API_FILE_H
#define ZRSOCKET_OS_API_FILE_H
#include <string>
#include <cassert>
#include "config.h"
#include "base_type.h"

#ifdef ZRSOCKET_OS_WINDOWS
#include <io.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "byte_buffer.h"

//标准输入 keyboard(defaut)
#define STDIN_FILENO  0

//标准输出 screen(defaut)
#define STDOUT_FILENO 1

//标准错误输出 screen(defaut)
#define STDERR_FILENO 2

#endif

ZRSOCKET_NAMESPACE_BEGIN

class OSApiFile
{
public:
    static int os_open(const char *file_name, int flags, int mode)
    {
#ifdef ZRSOCKET_OS_WINDOWS
        return ::_open(file_name, flags, mode);
#else
        return ::open(file_name, flags, mode);
#endif
    }

    static int os_creat(const char *file_name, int mode)
    {
#ifdef ZRSOCKET_OS_WINDOWS
        return ::_creat(file_name, mode);
#else
        return ::creat(file_name, mode);
#endif
    }

    static int os_close(int fd)
    {
#ifdef ZRSOCKET_OS_WINDOWS
        return ::_close(fd);
#else
        return ::close(fd);
#endif
    }

    static inline int os_read(int fd, char* buf, uint_t nbytes)
    {
#ifdef ZRSOCKET_OS_WINDOWS
        return _read(fd, buf, nbytes);
#else
        return ::read(fd, buf, nbytes);
#endif
    }

    static inline int os_write(int fd, const char *buf, uint_t nbytes)
    {
#ifdef ZRSOCKET_OS_WINDOWS
        return ::_write(fd, buf, nbytes);
#else
        return ::write(fd, buf, nbytes);
#endif
    }

    static int os_lseek(int fd, long offset, int whence)
    {
#ifdef ZRSOCKET_OS_WINDOWS
        return ::_lseek(fd, offset, whence);
#else
        return ::lseek(fd, offset, whence);
#endif
    }

    static int os_fsync(int fd)
    {
#ifdef ZRSOCKET_OS_WINDOWS
        return ::_commit(fd);
#else
        return ::fsync(fd);
#endif
    }

    static int os_fdatasync(int fd)
    {
#ifdef ZRSOCKET_OS_WINDOWS
        return ::_commit(fd);
#else
        return ::fdatasync(fd);
#endif
    }

    static void os_sync(int fd)
    {
#ifdef ZRSOCKET_OS_WINDOWS
        ::_commit(fd);
#else
        ::sync();
#endif
    }

    static int readn(int fd, char *buf, uint_t nbytes)
    {
        assert(buf != nullptr);
        assert(nbytes > 0);

        char *ptr = buf;
        uint_t nleft = nbytes;
        int nread = 0;

        do {
            nread = os_read(fd, ptr, nleft);
            if (nread < 0) {
                if (nleft == nbytes) {
                    return -1;
                }
                else {
                    break;
                }
            }
            else if (nread == 0) {
                break;
            }
            nleft -= nread;
            ptr += nread;
        } while (nleft > 0);

        return (nbytes - nleft);
    }

    static int writen(int fd, const char *buf, uint_t nbytes)
    {
        assert(buf != nullptr);
        assert(nbytes > 0);

        char *ptr = const_cast<char *>(buf);
        uint_t nleft = nbytes;
        int nwrite = 0;

        do {
            nwrite = os_write(fd, ptr, nleft);
            if (nwrite < 0) {
                if (nleft == nbytes) {
                    return -1;
                }
                else {
                    break;
                }
            }
            else if (nwrite == 0) {
                break;
            }
            nleft -= nwrite;
            ptr += nwrite;
        } while (nleft > 0);

        return (nbytes - nleft);
    }

    //因windows下没有writev, 
    //  当mode==0时,将iov指向内容拷贝到一个buffer中
    //    mode!=0时,循环调write
    static int writev(int fd, const ZRSOCKET_IOVEC *iov, int iovcnt, int mode = 0)
    {
#ifdef ZRSOCKET_OS_WINDOWS
        if (0 == mode) {
            thread_local ByteBuffer buf(1024);
            buf.reset();
            for (int i = 0; i < iovcnt; ++i) {
                buf.write(iov[i].iov_base, iov[i].iov_len);
            }
            return os_write(fd, buf.data(), buf.data_size());
        }
        else {
            int ret;
            int write_nbytes = 0;
            for (int i = 0; i < iovcnt; ++i) {
                ret = os_write(fd, iov[i].iov_base, iov[i].iov_len);
                if (ret < static_cast<int>(iov[i].iov_len)) {
                    return write_nbytes;
                }
                else {
                    write_nbytes += ret;
                }
            }
            return write_nbytes;
        }
#else
        return ::writev(fd, iov, iovcnt);
#endif
    }

    OSApiFile() = default;
    virtual ~OSApiFile()
    {
        close();
    }

    inline int open(const char *file_name, int flags, int mode)
    {
        close();

        fd_ = os_open(file_name, flags, mode);
        if (fd_ < 0) {
            return -1;
        }
        file_name_ = file_name;
        return 0;
    }

    inline int creat(const char *file_name, int mode)
    {
        close();

        fd_ = os_creat(file_name, mode);
        if (fd_ < 0) {
            return -1;
        }
        file_name_ = file_name;
        return 0;
    }

    inline int close()
    {
        if (fd_ < 0) {
            return -1;
        }

        if (os_close(fd_) < 0) {
            return -1;
        }

        fd_ = -1;
        file_name_.clear();
        return 0;
    }

    inline int read(char *buf, uint_t nbytes)
    {
        return os_read(fd_, buf, nbytes);
    }

    inline int write(const char *buf, uint_t nbytes)
    {
        return os_write(fd_, buf, nbytes);
    }

    inline int lseek(long offset, int whence)
    {
        return os_lseek(fd_, offset, whence);
    }

    inline int fsync()
    {
        return os_fsync(fd_);
    }

    inline int fdatasync()
    {
        return os_fdatasync(fd_);
    }

    inline void sync()
    {
        os_sync(fd_);
    }

    inline int readn(char *buf, uint_t nbytes)
    {
        return readn(fd_, buf, nbytes);
    }

    inline int writen(const char *buf, uint_t nbytes)
    {
        return writen(fd_, buf, nbytes);
    }

    inline int writev(const ZRSOCKET_IOVEC *iov, int iovcnt)
    {
        return writev(fd_, iov, iovcnt);
    }

    inline int fd() const
    {
        return fd_;
    }

    inline const char * file_name() const
    {
        return file_name_.c_str();
    }
protected:
    int fd_ = -1;
    std::string file_name_;
};

ZRSOCKET_NAMESPACE_END

#endif
