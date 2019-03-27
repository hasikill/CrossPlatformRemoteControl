#include "myfilesender.h"
#include "common.h"
#include <QFile>
#include <QDir>
#include <thread>
#include <QQueue>
#include <QDebug>
#include "myclient.h"

MyFileSender::MyFileSender(MyClient *client, ushort port, QString dlPath, int nMode)
{
    m_pClient = client;
    m_dlPath = dlPath;
    m_nMode = nMode;
    m_uPort = port;
}

bool MyFileSender::sendFileData(QString fileName, int nOffset)
{
    //数据包
    char packBuf[128] = { 0 };
    PACKET *pkt = reinterpret_cast<PACKET*>(packBuf);

    //临时拷贝缓冲区
    QFile file;
    char *buf = new char[1024 * 1024 * 10];
    if (buf == nullptr)
    {
        goto  FILE_EXIT;
    }

    //文件发送开始
    {
        file.setFileName(fileName);
        if (!file.open(QIODevice::ReadOnly))
        {
            goto FILE_EXIT;
        }

        //发送路径
        QString rPath = fileName.right(fileName.size() - nOffset);
        QByteArray ary = rPath.toLocal8Bit();
        //清零缓冲区
        memset(packBuf, 0, 128);
        //开始组包
        pkt->byCmdMain = DOWNLOAD_FILE;
        pkt->byCmdSub = DOWNLOAD_READY;
        pkt->uLength = sizeof(qint64) + static_cast<size_t>(ary.size());

        //文件大小
        qint64 fileSize = file.size();
        reinterpret_cast<qint64 *>(pkt->Data)[0] = fileSize;
        memcpy(&reinterpret_cast<qint64 *>(pkt->Data)[1], ary, static_cast<size_t>(ary.size()));
        int nRet = send(m_Socket, packBuf, static_cast<int>(sizeof(PACKET) + pkt->uLength), 0);
        if (nRet == SOCKET_ERROR)
        {
            qDebug("send失败！错误码 : %d", WSAGetLastError());
            goto FILE_EXIT;
        }

        //开始发送数据
        pkt->byCmdMain = DOWNLOAD_FILE;
        pkt->byCmdSub = DOWNLOAD_DATA;
        pkt->uLength = 0;
        nRet = send(m_Socket, packBuf, static_cast<int>(sizeof(PACKET) + pkt->uLength), 0);
        if (nRet == SOCKET_ERROR)
        {
            qDebug("send失败！错误码 : %d", WSAGetLastError());
            goto FILE_EXIT;
        }

        //读文件发送
        qint64 nTotolSize = 0;
        while(nTotolSize < fileSize)
        {
            qint64 nReadSize = file.read(buf, 1024 * 1024 * 10);
            if (nReadSize <= 0)
            {
                qDebug("nReadSize失败！");
                goto FILE_EXIT;
            }
            int nSendSize = 0;
            while (nSendSize < nReadSize)
            {
                nRet = send(m_Socket, buf + nSendSize, static_cast<int>(nReadSize - nSendSize), 0);
                if (nRet == SOCKET_ERROR)
                {
                    qDebug("send失败！错误码 : %d", WSAGetLastError());
                    goto FILE_EXIT;
                }
                nSendSize += nRet;
            }
            nTotolSize += nReadSize;
        }

        //发送End
        pkt->byCmdMain = DOWNLOAD_FILE;
        pkt->byCmdSub = DOWNLOAD_ENDFILE;
        pkt->uLength = 0;

        nRet = send(m_Socket, packBuf, static_cast<int>(sizeof(PACKET) + pkt->uLength), 0);
        if (nRet == SOCKET_ERROR)
        {
            qDebug("send失败！错误码 : %d", WSAGetLastError());
            goto FILE_EXIT;
        }
    }

FILE_EXIT:
    if (buf != nullptr)
    {
        delete[] buf;
        buf = nullptr;
    }
    if (file.isOpen())
    {
        file.close();
    }
}

void MyFileSender::run()
{
    //数据包
    char packBuf[1024] = { 0 };
    PACKET *pkt = reinterpret_cast<PACKET*>(packBuf);
    int nOffset = m_dlPath.lastIndexOf('/');

    //socket
    if (init())
    {
        //开始传输文件
        if (m_nMode == DOWNLOAD_DIR)
        {//目录
            QDir dir(m_dlPath);
            //目录是否存在
            if (!dir.exists())
            {
                goto FILE_EXIT;
            }

            //遍历队列
            QQueue<QFileInfo> que;
            QList<QFileInfo> dirList;
            QList<QFileInfo> fileList;
            que.enqueue(QFileInfo(dir, m_dlPath));

            //层序遍历
            while (!que.empty())
            {
                //弹出
                QFileInfo info = que.dequeue();

                //判断是否有子文件
                if (info.isDir())
                {
                    //添加到dirList
                    dirList.push_back(info);
                    //子文件
                    QDir tmp = info.absoluteFilePath();
                    QFileInfoList infoList = tmp.entryInfoList();
                    for (int i = 0; i < infoList.count(); i++)
                    {
                        QFileInfo mfi = infoList.at(i);
                        //过滤
                        if(mfi.fileName()=="." || mfi.fileName() == "..") continue;
                        //添加到队列
                        que.enqueue(mfi);
                    }
                }
                else if(info.isFile())
                {
                    //添加到fileList
                    fileList.push_back(info);
                }
            }

            //文件总数
            int nCount = fileList.count();

            //发送文件数量
            pkt->byCmdMain = DOWNLOAD_DIR;
            pkt->byCmdSub = DOWNLOAD_FILECOUNT;
            pkt->uLength = 4;
            reinterpret_cast<int *>(pkt->Data)[0] = nCount;
            int nRet = send(m_Socket, packBuf, static_cast<int>(sizeof(PACKET) + pkt->uLength), 0);
            if (nRet == SOCKET_ERROR)
            {
                qDebug("send失败！错误码 : %d", WSAGetLastError());
                goto FILE_EXIT;
            }

            //先发送命令创建文件夹
            for (int i = 0; i < dirList.count(); i++)
            {
                //构建相对路径
                QString rPath = dirList[i].absoluteFilePath();
                rPath = rPath.right(rPath.size() - nOffset);
                QByteArray ary = rPath.toLocal8Bit();
                //清零缓冲区
                memset(packBuf, 0, 1024);
                //发送命令
                pkt->byCmdMain = DOWNLOAD_DIR;
                pkt->byCmdSub = DOWNLOAD_CREATE;
                pkt->uLength = static_cast<size_t>(ary.size());
                memcpy(pkt->Data, ary, static_cast<size_t>(ary.size()));

                int nRet = send(m_Socket, packBuf, static_cast<int>(sizeof(PACKET) + pkt->uLength), 0);
                if (nRet == SOCKET_ERROR)
                {
                    qDebug("send失败！错误码 : %d", WSAGetLastError());
                    goto FILE_EXIT;
                }
            }

            //再发送文件
            for (int i = 0; i < fileList.count(); i++)
            {
                sendFileData(fileList[i].absoluteFilePath(), nOffset);
            }

        }
        else if (m_nMode == DOWNLOAD_FILE)
        {//文件
            sendFileData(m_dlPath, nOffset);
        }
    }

    //发送完毕包
    pkt->byCmdMain = DOWNLOAD_FILE;
    pkt->byCmdSub = DOWNLOAD_FINISH;
    pkt->uLength = 0;
    if (send(m_Socket, packBuf, static_cast<int>(sizeof(PACKET) + pkt->uLength), 0) == SOCKET_ERROR)
    {
        qDebug("send失败！错误码 : %d", WSAGetLastError());
        goto FILE_EXIT;
    }

    //等待服务器传输完成
    while(recv(m_Socket, packBuf, 100, 0) > 0);

FILE_EXIT:
    //关闭连接
    closesocket(m_Socket);

    //释放自身
    std::thread([this](){
        Sleep(100);
        delete this;
    }).detach();
}

bool MyFileSender::init()
{
    //创建socket
    m_Socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (m_Socket == INVALID_SOCKET)
    {
        qDebug("socket失败！错误码 : %d", WSAGetLastError());
        return false;
    }

    //设置连接信息
    SOCKADDR_IN RemoteAddr;
    memset(&RemoteAddr, 0, sizeof(SOCKADDR_IN));
    RemoteAddr.sin_family = AF_INET;
    RemoteAddr.sin_port = m_uPort;
    RemoteAddr.sin_addr.S_un.S_addr = m_pClient->RemoteAddr.sin_addr.S_un.S_addr;
    //开始连接
    Sleep(50);
    int nRet = ::connect(m_Socket, reinterpret_cast<SOCKADDR*>(&RemoteAddr), sizeof(SOCKADDR_IN));
    if (nRet == SOCKET_ERROR)
    {
        qDebug("connect失败！错误码 : %d", WSAGetLastError());
        return false;
    }

    return true;
}
