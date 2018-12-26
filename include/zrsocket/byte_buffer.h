#ifndef ZRSOCKET_BYTEBUFFER_H_
#define ZRSOCKET_BYTEBUFFER_H_
#include "config.h"
#include "base_type.h"
#include "malloc.h"
#include "memory.h"
#include "atomic.h"

ZRSOCKET_BEGIN

class ZRSOCKET_EXPORT ByteBuffer
{
public:
    inline ByteBuffer()
        : buffer_(NULL)
        , buffer_size_(0)
        , data_begin_index_(0)
        , data_end_index_(0)
        , owner_(true)
    {
    }

    inline ByteBuffer(uint_t buffer_size)
        : data_begin_index_(0)
        , data_end_index_(0)
        , owner_(true)
    {
        char *new_buffer = (char *)zrsocket_malloc(buffer_size);
        if (NULL != new_buffer) {
            buffer_      = new_buffer;
            buffer_size_ = buffer_size;
        }
        else {
            buffer_      = NULL;
            buffer_size_ = 0;
        }
    }

    inline ByteBuffer(const ByteBuffer &buf)
    {
        if (buf.owner_) {
            buf.owner_          = false;
            buffer_             = buf.buffer_;
            buffer_size_        = buf.buffer_size_;
            data_begin_index_   = buf.data_begin_index_;
            data_end_index_     = buf.data_end_index_;
            owner_              = true;
        }
        else {
            buffer_             = NULL;
            buffer_size_        = 0;
            data_begin_index_   = 0;
            data_end_index_     = 0;
            owner_              = false;
        }
    }

    inline ~ByteBuffer()
    {
        if (owner_ && (NULL != buffer_)) {
            zrsocket_free(buffer_);
        }
    }

    inline ByteBuffer& operator= (const ByteBuffer &buf)
    {
        if (this != &buf) {
            if (owner_ && (NULL != buffer_)) {
                zrsocket_free(buffer_);
            }

            if (buf.owner_) {
                buf.owner_          = false;
                buffer_             = buf.buffer_;
                buffer_size_        = buf.buffer_size_;
                data_begin_index_   = buf.data_begin_index_;
                data_end_index_     = buf.data_end_index_;
                owner_              = true;
            }
            else {
                buffer_             = NULL;
                buffer_size_        = 0;
                data_begin_index_   = 0;
                data_end_index_     = 0;
                owner_              = false;
            }
        }
        return *this;
    }

    inline void clear()
    {
        if (owner_ && (NULL != buffer_)) {
            zrsocket_free(buffer_);
        }
        buffer_             = NULL;
        buffer_size_        = 0;
        data_begin_index_   = 0;
        data_end_index_     = 0;
        owner_              = true;
    }

    inline void reset()
    {
        data_begin_index_   = 0;
        data_end_index_     = 0;
    }

    inline char* buffer() const
    {
        return buffer_;
    }

    inline uint_t buffer_size() const
    {
        return buffer_size_;
    }

    inline bool data(const char *data, uint_t size, bool owner = true)
    {
        if (size == 0) {
            return false;
        }
        if (NULL == data) {
            return false;
        }
        if ( (data >= buffer_) && (data < (buffer_ + buffer_size_)) ) {
            return false;
        }

        if (owner) {
            char *new_buffer;
            if (owner_) {
                new_buffer = (char *)zrsocket_realloc(buffer_, size);
            }
            else {   
                new_buffer = (char *)zrsocket_malloc(size);
            }
            if (NULL == new_buffer)
            {
                return false;
            }
            zrsocket_memcpy(new_buffer, data, size);
            buffer_ = new_buffer;
        }
        else
        {
            if (owner_ && (buffer_ != NULL))
            {
                zrsocket_free(buffer_);
            }
            buffer_ = (char *)data;
        }
        buffer_size_        = size;
        data_begin_index_   = 0;
        data_end_index_     = size;
        owner_              = owner;
        return true;
    }

    inline char* data() const
    {
        return buffer_ + data_begin_index_;
    }

    inline uint_t data_size() const
    {
        return data_end_index_ - data_begin_index_;
    }

    //����δʹ�õ�����
    inline uint_t free_size() const
    {
        return buffer_size_ - data_end_index_;
    }

    inline uint_t data_begin() const
    { 
        return data_begin_index_;
    }

    inline void data_begin(uint_t new_begin_index)
    {
        if (new_begin_index > buffer_size_) {
            new_begin_index = buffer_size_;
        }
        data_begin_index_ = new_begin_index;
        if (data_begin_index_ > data_end_index_) {
            data_end_index_ = data_begin_index_;
        }
    }

    inline uint_t data_end() const
    {
        return data_end_index_;
    }

    inline void data_end(uint_t new_end_index)
    {
        if (new_end_index > buffer_size_) {
            new_end_index = buffer_size_;
        }
        data_end_index_ = new_end_index;
        if (data_begin_index_ > data_end_index_) {
            data_begin_index_ = data_end_index_;
        }
    }

    inline void advance(uint_t amount)
    {
        if (buffer_size_ >= (data_end_index_ + amount)) {
            data_end_index_ += amount;
        }
    }

    inline bool empty() const
    {
        return (data_end_index_ - data_begin_index_ == 0);
    }

    inline bool owner() const
    {
        return owner_;
    }

    inline void owner(bool owner)
    {
        owner_ = owner;
    }

    inline bool reserve(uint_t new_size)
    {
        if (owner_) {
            if (new_size > buffer_size_) {
                char *new_buffer = (char *)zrsocket_realloc(buffer_, new_size);
                if (NULL != new_buffer) {
                    buffer_             = new_buffer;
                    buffer_size_        = new_size;
                    data_begin_index_   = 0;
                    data_end_index_     = 0;
                    return true;
                }
            }
        }
        else {
            char *new_buffer = (char *)zrsocket_malloc(new_size);
            if (NULL != new_buffer) {
                buffer_             = new_buffer;
                buffer_size_        = new_size;
                data_begin_index_   = 0;
                data_end_index_     = 0;
                owner_              = true;
                return true;
            }
        }
        return false;
    }

    inline bool resize(uint_t new_size)
    {
        return reserve(new_size);
    }

    //multiple: ������������
    inline bool write(const char *data, uint_t size, uint_t multiple = 100)
    {
        if (!owner_) {
            return false;
        }

        if (free_size() >= size) {
            zrsocket_memcpy(buffer_ + data_end_index_, data, size);
            data_end_index_ += size;
        }
        else {   
            uint_t new_size  = buffer_size_ * multiple / 100 + buffer_size_ + size;
            char *new_buffer = (char *)zrsocket_realloc(buffer_, new_size);
            if (NULL != new_buffer) {
                zrsocket_memcpy(new_buffer + data_end_index_, data, size);
                buffer_ = new_buffer;
                buffer_size_ = new_size;
                data_end_index_ += size;
            }
            else {
                return false;
            }
        }
        return true;
    }

    inline bool write(const char *data)
    {
        return write(data, (strlen(data) + 1));
    }

    inline bool write(char c)
    {
        return write((const char *)&c, sizeof(char));
    }

    inline bool write(short n)
    {
        return write((const char *)&n, sizeof(short));
    }

    inline bool write(int n)
    {
        return write((const char *)&n, sizeof(uint_t));
    }

    inline bool write(int64_t n)
    {
        return write((const char *)&n, sizeof(int64_t));
    }

    inline bool write(real32_t r)
    {
        return write((const char *)&r, sizeof(real32_t));
    }

    inline bool write(real64_t r)
    {
        return write((const char *)&r, sizeof(real64_t));
    }

    inline uint_t read(char *data, uint_t size)
    {
        if (data_size() >= size) {
            zrsocket_memcpy(data, buffer_ + data_begin_index_, size);
            data_begin_index_ += size;
            return size;
        }
        return -1;
    }

    inline char* read(uint_t size)
    {
        if (data_size() >= size) {
            char *ret = (buffer_ + data_begin_index_);
            data_begin_index_ += size;
            return ret;
        }
        return NULL;
    }

private:
    char           *buffer_;            //������
    uint_t          buffer_size_;       //��������С
    uint_t          data_begin_index_;  //���ݿ�ʼλ
    uint_t          data_end_index_;    //���ݽ���λ
    mutable bool    owner_;             //��ʾ�Ƿ�ӵ������Ȩ
};

class ZRSOCKET_EXPORT SharedBuffer
{
public:
    inline SharedBuffer()
        : data_begin_index_(0)
        , data_end_index_(0)
    {
        buffer_ = new Buffer_();
    }

    inline SharedBuffer(uint_t buffer_size)
        : data_begin_index_(0)
        , data_end_index_(0)
    {
        buffer_ = new Buffer_(buffer_size);
    }

    inline SharedBuffer(const SharedBuffer &buf)
        : data_begin_index_(0)
        , data_end_index_(0)
    {
        buf.buffer_->increment_refcount();
        buffer_             = buf.buffer_;
        data_begin_index_   = buf.data_begin_index_;
        data_end_index_     = buf.data_end_index_;
    }

    inline ~SharedBuffer()
    {
        buffer_->release();
    }

    inline SharedBuffer& operator=(const SharedBuffer &buf)
    {
        buffer_->release();

        buf.buffer_->increment_refcount();
        buffer_             = buf.buffer_;
        data_begin_index_   = buf.data_begin_index_;
        data_end_index_     = buf.data_end_index_;
        return *this;
    }

    inline SharedBuffer& operator=(ByteBuffer &buf)
    {
        if (buf.owner()) {
            data(buf.buffer(), buf.buffer_size(), false);
            buffer_->owner_   = true;
            data_begin_index_ = buf.data_begin();
            data_end_index_   = buf.data_end();
            buf.owner(false);
        }
        return *this;
    }

    inline void clear()
    {
        reset();
        buffer_->clear();
    }

    inline bool reserve(uint_t new_size)
    {
        return buffer_->reserve(new_size);
    }

    inline bool resize(uint_t new_size)
    {
        return reserve(new_size);
    }

    inline void reset()
    {
        data_begin_index_ = 0;
        data_end_index_   = 0;
    }

    inline uint_t use_count()
    {
        return buffer_->refcount_.load();
    }

    inline bool empty() const
    {
        return data_end_index_ - data_begin_index_ == 0;
    }

    inline uint_t buffer_size() const
    {
        return buffer_->buffer_size();
    }

    inline char* buffer()
    {
        return buffer_->buffer();
    }

    //����δʹ�õ�����
    inline uint_t free_size() const
    {
        return buffer_->free_size();
    }

    inline bool data(const char *data, uint_t size, bool owner = true)
    {
        if (size == 0) {
            return false;
        }
        if (NULL == data) {
            return false;
        }
        buffer_->release();
        reset();

        if (owner) {
            buffer_ = new Buffer_(size);
            write(data, size);
        }
        else {
            buffer_ = new Buffer_();
            buffer_->buffer_ = (char *)data;
            buffer_->buffer_size_ = size;
        }
        buffer_->owner_ = owner;
        return true;
    }

    inline char* data() const
    {
        return buffer_->buffer_ + data_begin_index_;
    }

    inline uint_t data_size() const
    {
        return data_end_index_ - data_begin_index_;
    }

    inline uint_t data_begin() const
    { 
        return data_begin_index_;
    }

    inline void data_begin(uint_t new_begin_index)
    {
        if (new_begin_index > buffer_->buffer_size_)
        {
            new_begin_index = buffer_->buffer_size_;
        }
        data_begin_index_ = new_begin_index;
        if (data_begin_index_ > data_end_index_)
        {
            data_end_index_ = data_begin_index_;
        }
    }

    inline uint_t data_end() const
    {
        return data_end_index_;
    }

    inline void data_end(uint_t new_end_index)
    {
        if (new_end_index > buffer_->buffer_size_)
        {
            new_end_index = buffer_->buffer_size_;
        }
        data_end_index_ = new_end_index;
        if (data_begin_index_ > data_end_index_)
        {
            data_begin_index_ = data_end_index_;
        }
    }

    inline void advance(uint_t amount)
    {
        if (buffer_->buffer_size() >= (data_end_index_ + amount))
        {
            data_end_index_ += amount;
        }
    }

    inline bool owner() const
    {
        return buffer_->owner_;
    }

    inline void owner(bool owner)
    {
        buffer_->owner_ = owner;
    }

    //multiple: ������������
    inline bool write(const char *data, uint_t size, uint_t multiple = 100)
    {
        bool ret = buffer_->write(data, size, multiple);
        if (ret)
        {
            data_end_index_ += size;
        }
        return ret;
    }

    inline bool write(const char *data)
    {
        return write(data, (uint_t)(strlen(data) + 1));
    }

    inline bool write(char c)
    {
        return write((const char *)&c, sizeof(char));
    }

    inline bool write(short n)
    {
        return write((const char *)&n, sizeof(short));
    }

    inline bool write(int n)
    {
        return write((const char *)&n, sizeof(uint_t));
    }

    inline bool write(int64_t n)
    {
        return write((const char *)&n, sizeof(int64_t));
    }

    inline bool write(real32_t r)
    {
        return write((const char *)&r, sizeof(real32_t));
    }

    inline bool write(real64_t r)
    {
        return write((const char *)&r, sizeof(real64_t));
    }

    inline uint_t read(char *data, uint_t size)
    {
        uint_t data_size = data_end_index_ - data_begin_index_;
        if (data_size >= size) {
            zrsocket_memcpy(data, buffer_->buffer_ + data_begin_index_, size);
            data_begin_index_ += size;
            return size;
        }
        return -1;
    }

    inline char* read(uint_t size)
    {
        uint_t data_size = data_end_index_ - data_begin_index_;
        if (data_size >= size) {
            char *ret = buffer_->buffer_ + data_begin_index_;
            data_begin_index_ += size;
            return ret;
        }
        return NULL;
    }

private:
    class Buffer_
    {
    public:
        inline Buffer_()
            : buffer_(NULL)
            , buffer_size_(0)
            , use_size_(0)
            , refcount_(1)
            , owner_(true)
        {
        }

        inline Buffer_(uint_t buffer_size)
            : use_size_(0)
            , refcount_(1)
            , owner_(true)
        {
            char *new_buffer = (char *)zrsocket_malloc(buffer_size);
            if (NULL != new_buffer) {
                buffer_      = new_buffer;
                buffer_size_ = buffer_size;
            }
            else {
                buffer_      = NULL;
                buffer_size_ = 0;
            }
        }

        inline ~Buffer_()
        {
        }

        inline bool reserve(uint_t new_size)
        {   
            if (owner_ && (new_size > buffer_size_)) {
                char *new_buffer = (char *)zrsocket_realloc(buffer_, new_size);
                if (NULL != new_buffer) {
                    buffer_      = new_buffer;
                    buffer_size_ = new_size;
                    return true;
                }
            }
            return false;
        }

        //��1, ���ر仯��ֵ
        inline int increment_refcount()
        {
            return ++refcount_;
        }

        //��1, ���ر仯��ֵ
        inline int decrement_refcount()
        {
            return --refcount_;
        }

        inline void clear()
        {
            if (NULL != buffer_) {
                if (owner_) {
                    zrsocket_free(buffer_);
                }
                buffer_ = NULL;
                buffer_size_ = 0;
                use_size_ = 0;
            }
        }

        inline void release()
        {
            if (--refcount_ == 0) {
                if (NULL != buffer_) {
                    if (owner_) {
                        zrsocket_free(buffer_);
                    }
                    buffer_ = NULL;
                    buffer_size_ = 0;
                    use_size_ = 0;
                }

                delete this;
            }
        }

        inline uint_t buffer_size() const
        {
            return buffer_size_;
        }

        inline char* buffer()
        {
            return buffer_;
        }

        inline uint_t free_size() const
        {
            return buffer_size_ - use_size_;
        }

        //multiple: ������������
        inline bool write(const char *data, uint_t size, uint_t multiple = 100)
        {
            if (free_size() >= size) {
                zrsocket_memcpy(buffer_ + use_size_, data, size);
                use_size_ += size;
                return true;
            }
            else {
                if (owner_) {
                    uint_t new_size  = buffer_size_ * multiple / 100 + buffer_size_ + size;
                    char *new_buffer = (char *)zrsocket_realloc(buffer_, new_size);
                    if (NULL != new_buffer) {
                        zrsocket_memcpy(new_buffer + use_size_, data, size);
                        use_size_      += size;
                        buffer_size_    = new_size;
                        buffer_         = new_buffer;
                        return true;
                    }
                }
            }
            return false;
        }

    public:
        char           *buffer_;        //������
        uint_t          buffer_size_;   //��������С
        uint_t          use_size_;      //��ʹ�ô�С
        AtomicUInt      refcount_;      //���ü���
        bool            owner_;         //��ʾ�Ƿ�ӵ������Ȩ
    };

private:
    Buffer_ *buffer_;       
    uint_t   data_begin_index_; //���ݿ�ʼλ
    uint_t   data_end_index_;   //���ݽ���λ
};

ZRSOCKET_END

#endif
