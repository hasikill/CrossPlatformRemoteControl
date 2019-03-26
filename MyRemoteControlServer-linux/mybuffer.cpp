#include "mybuffer.h"
#include <QDebug>

MyBuffer::MyBuffer()
{
    m_Ptr = new char[BUF_SIZE];
    memset(m_Ptr, 0, BUF_SIZE);
    m_nCapacity = BUF_SIZE;
    m_nOffset = 0;
}

MyBuffer::~MyBuffer()
{
    if (m_Ptr != nullptr)
    {
        delete[] m_Ptr;
        m_Ptr = nullptr;
    }
}

void MyBuffer::cpData(void *data, size_t nLen)
{
    //异常判断
    if (data == nullptr || nLen == 0)
    {
        return;
    }
    //1.判断容量是否足够
    size_t nNewCapacity = m_nOffset + nLen;
    if (nNewCapacity >= m_nCapacity)
    {
        //判断差量
        size_t nResidu = nNewCapacity - m_nCapacity;
        if (nResidu < 1024 * 1024)
        {
            //小于1M统一申请1M
            m_nCapacity += (1024 * 1024);
        }
        else
        {
            //否则在新空间的基础上多给1M
            m_nCapacity = nNewCapacity + (1024 * 1024);
        }
        qDebug() << "容量扩充 : " << m_nCapacity;

        //申请
        char* tmp = new char[m_nCapacity];
        memset(tmp, 0, m_nCapacity);

        //拷贝原数据
        memcpy(tmp, m_Ptr, m_nOffset);

        //释放先前数据
        if (m_Ptr != nullptr)
        {
            delete [] m_Ptr;
            m_Ptr = nullptr;
        }
        //赋值新地址
        m_Ptr = tmp;
    }

    //足够容量,添加数据
    memcpy(m_Ptr + m_nOffset, data, nLen);
    m_nOffset += nLen;
    m_Ptr[m_nOffset] = 0;
}

void MyBuffer::moveLeft(size_t nOffset, size_t nLen)
{
    memcpy(m_Ptr, m_Ptr + nOffset, static_cast<size_t>(nLen));
    m_nOffset = nLen;
    m_Ptr[m_nOffset] = 0;
}

void MyBuffer::clear()
{
    m_nOffset = 0;
}

size_t MyBuffer::offset()
{
    return m_nOffset;
}

size_t MyBuffer::capacity()
{
    return m_nCapacity;
}

char *MyBuffer::data()
{
    return m_Ptr;
}
