#ifndef MYBUFFER_H
#define MYBUFFER_H

#include <memory.h>
#define BUF_SIZE 1024 * 1024 * 10

class MyBuffer
{
public:
    MyBuffer();
    ~MyBuffer();

    //删除拷贝构造 防止浅拷贝问题
    MyBuffer(MyBuffer&) = delete;
    MyBuffer(MyBuffer&&) = delete;

    void cpData(void *ptr, size_t nLen);
    void moveLeft(size_t nOffset, size_t nLen);
    void clear();
    size_t offset();
    size_t capacity();
    char* data();

private:
    char* m_Ptr;
    size_t m_nCapacity;
    size_t m_nOffset;
};

#endif // MYBUFFER_H
