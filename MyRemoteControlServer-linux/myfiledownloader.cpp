#include "myfiledownloader.h"
#include "mainwindow.h"
#include "common.h"
#include <thread>
#include <QFile>
#include <QDir>
#include <QDebug>

MyFileDownloader::MyFileDownloader(boost::asio::io_context &context,
                                   MyPacketSender *sender,
                                   QWidget *parent,
                                   QString dlPath,
                                   QString svPath,
                                   COMMAND_SUB nMode)
    : m_acceptor(context, tcp::endpoint()), m_sock(context)
{
    //初始赋值
    m_dlPath = dlPath;
    m_svPath = svPath;
    m_nMode = nMode;
    m_sender = sender;

    m_dlg = new QDialog(parent);

    //窗口大小
    m_dlg->resize(688, 108);

    QWidget *widget = new QWidget(m_dlg);
    widget->setGeometry(QRect(8, 1, 671, 104));

    //布局
    QVBoxLayout *verticalLayout = new QVBoxLayout(widget);
    verticalLayout->setSpacing(6);
    verticalLayout->setContentsMargins(11, 11, 11, 11);
    verticalLayout->setContentsMargins(0, 0, 0, 0);

    //标签
    m_labelNum = new QLabel(m_dlg);
    m_labelNum->setText("已下载/总文件数 0/0 " + dlPath);
    m_labelSingle = new QLabel(m_dlg);
    tcp::endpoint pt = sender->socket()->remote_endpoint();
    QString title = QString::asprintf("%s:%d->", pt.address().to_string().c_str(), pt.port()) + "正在下载: " + dlPath;
    m_labelSingle->setText("正在下载: " + dlPath);
    m_dlg->setWindowTitle(title);

    //进度条
    m_progressSingle = new QProgressBar(m_dlg);
    m_progressSingle->setAlignment(Qt::AlignCenter);
    m_progressNum = new QProgressBar(m_dlg);
    m_progressNum->setAlignment(Qt::AlignCenter);

    //添加控件
    verticalLayout->addWidget(m_labelNum);
    verticalLayout->addWidget(m_progressNum);
    verticalLayout->addWidget(m_progressSingle);
    verticalLayout->addWidget(m_labelSingle);

    //信号槽
    //m_pMain->addConnect1(this);
    connect(this, &MyFileDownloader::sigShowSingleTxt, this, &MyFileDownloader::ShowSingleTxt);
    connect(this, &MyFileDownloader::sigSetSingleProg, this, &MyFileDownloader::SetSingleProg);
    connect(this, &MyFileDownloader::sigShowTotolTxt, this, &MyFileDownloader::ShowTotolTxt);
    connect(this, &MyFileDownloader::sigSetTotolProg, this, &MyFileDownloader::SetTotolProg);

   //通知远程客户端连接
   ushort port = m_acceptor.local_endpoint().port();
   m_sender->sendDownloadFile(htons(port), dlPath, nMode);

   //显示
   m_dlg->show();
}

void MyFileDownloader::run()
{
    //客户端地址数据
    boost::system::error_code ec;
    m_acceptor.accept(m_sock, ec);
    if (ec)
    {
        qDebug("accept失败！,Msg: %s, code: %d", ec.message().c_str(), ec.value());
        return;
    }

    //数据包
    char packBuf[128] = { 0 };
    PACKET *pkt = reinterpret_cast<PACKET*>(packBuf);

    //文件大小
    qint64 fileSize = 0;
    QFile file;

    //文件数 默认1
    int nFiles = 1;
    int nCurFile = 0;
    //计时开始 测速
    clock_t timeStart = clock();
    clock_t timeTotol = clock();
    qint64 nTotolDataSize = 0;
    qint64 nTransport = 0;

    //临时拷贝缓冲区
    char *buf = new char[BUFFERSIZE];
    if (buf == nullptr)
    {
        goto  FILE_EXIT;
    }

    //接收文件数据
    while(true)
    {
        //清零
        memset(packBuf, 0, 128);
        //读取包12头
        if (RecvPacketHeader(pkt) == false)
        {
            break;
        }

        //读包
        if (pkt->byCmdMain == DOWNLOAD_FILE)
        {
            if (pkt->byCmdSub == DOWNLOAD_READY)
            {
                //读取数据
                if (RecvData(pkt->Data, pkt->uLength) == false)
                {
                    qDebug() << "RecvData Error: " << __LINE__;
                    break;
                }
                //文件大小
                fileSize = reinterpret_cast<qint64 *>(pkt->Data)[0];
                //文件路径
                QString strPath;
                if (m_nMode == DOWNLOAD_FILE)
                {
                    strPath = m_svPath;
                }
                else if (m_nMode == DOWNLOAD_DIR)
                {
                    strPath = m_svPath + QString::fromLocal8Bit(pkt->Data + sizeof(qint64));
                }
                //显示ui
                emit this->sigShowTotolTxt(QString::asprintf("已下载/总文件数 %d/%d %s", nCurFile, nFiles, m_dlPath.toStdString().c_str()));
                emit this->sigShowSingleTxt("正在下载: " + strPath);
                emit this->sigSetSingleProg(0);
                //创建文件
                file.setFileName(strPath);
                if (!file.open(QIODevice::WriteOnly))
                {
                    qDebug() << "file.open Error: " << __LINE__;
                    qDebug() << strPath;
                    break;
                }
            }
            else if (pkt->byCmdSub == DOWNLOAD_DATA)
            {
                //下载进度
                int nSchedule = 0;
                //读socket
                qint64 nTotolSize = 0;

                while (nTotolSize < fileSize)
                {
                    //读一次
                    int bufSize = (fileSize - nTotolSize) > BUFFERSIZE ? BUFFERSIZE : (fileSize - nTotolSize);
                    boost::system::error_code ec;
                    int nReadSize = static_cast<int>(m_sock.read_some(boost::asio::buffer(buf, static_cast<size_t>(bufSize)), ec));
                    if (ec || nReadSize <= 0)
                    {
                        qDebug() << "recv Error: " << __LINE__;
                        goto FILE_EXIT;
                    }
                    //写文件
                    int nTotolWirteSize = 0;
                    while (nTotolWirteSize < nReadSize)
                    {
                        qint64 nWriteSize = file.write(buf + nTotolWirteSize, nReadSize - nTotolWirteSize);
                        if (nWriteSize <= 0)
                        {
                            qDebug() << "file.write Error: " << __LINE__;
                            goto FILE_EXIT;
                        }
                        //
                        nTotolWirteSize += nWriteSize;
                    }
                    nTotolSize += nReadSize;
                    nTransport += nReadSize;
                    nTotolDataSize += nReadSize;
                    //计时结束
                    clock_t timeLost = clock() - timeStart;
                    if (timeLost > 1000)
                    {
                        QString strTransport = "[ " + getStringSize(nTransport) + "/秒 ]";
                        emit this->sigShowSingleTxt("正在下载" + strTransport + ": " + m_svPath);
                        timeStart = clock();
                        nTransport = 0;
                    }
                    //计算进度
                    nSchedule = static_cast<int>(static_cast<double>(nTotolSize) / static_cast<double>(fileSize) * 100.0);
                    emit this->sigSetSingleProg(nSchedule);
                }

                //下载成功 关闭文件
                file.close();
            } 
            else if (pkt->byCmdSub == DOWNLOAD_ENDFILE)
            {
                //完成一个++
                nCurFile ++;
                //下载进度
                int nSchedule = static_cast<int>(static_cast<double>(nCurFile) / static_cast<double>(nFiles) * 100.0);
                emit this->sigSetTotolProg(nSchedule);
                emit this->sigShowTotolTxt(QString::asprintf("已下载/总文件数 %d/%d %s", nCurFile, nFiles, m_dlPath.toStdString().c_str()));
            }
            else if (pkt->byCmdSub == DOWNLOAD_FINISH)
            {
                //总耗时
                timeTotol = clock() - timeTotol;
                //计算平均速率
                QString avgSpeed = "[ " + getStringSize((double)nTotolDataSize / ((double)timeTotol / 1000.0)) + "/秒 ]";
                emit this->sigShowSingleTxt(QString::asprintf("下载完成( 文件数[ %d ] 总数据[ %s ] 耗时[ %.2f秒 ] 速率%s )", nFiles, getStringSize(nTotolDataSize).toStdString().c_str(), ((double)timeTotol / 1000.0), avgSpeed.toStdString().c_str()));
                m_sock.shutdown(tcp::socket::shutdown_both);
                break;
            }
        }
        else if (pkt->byCmdMain == DOWNLOAD_DIR)
        {
            if (pkt->byCmdSub == DOWNLOAD_FILECOUNT)
            {
                //读取文件任务数量
                if (RecvData(reinterpret_cast<char *>(&nFiles), pkt->uLength) == false)
                {
                    qDebug() << "RecvData Error: " << __LINE__;
                    break;
                }
            }
            else if (pkt->byCmdSub == DOWNLOAD_CREATE)
            {
                //读取创建目录
                if (RecvData(pkt->Data, pkt->uLength) == false)
                {
                    qDebug() << "RecvData Error: " << __LINE__;
                    break;
                }
                //相对路径
                QString dirPath = QString::fromLocal8Bit(pkt->Data);
                //构建绝对路径
                dirPath = m_svPath + dirPath;
                //创建目录
                QDir dir;
                dir.mkpath(dirPath);
            }
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

    //收尾处理
    m_acceptor.close();
    m_sock.close();
}

QString MyFileDownloader::getStringSize(qint64 nTransport)
{
    QString strTransport;

    if (nTransport > 1024 * 1024 * 1024)
    {
        strTransport = QString::asprintf("%.2fGB", (float)nTransport / (1024.0 * 1024.0 * 1024.0));
    }
    else if (nTransport > 1024 * 1024)
    {
        strTransport = QString::asprintf("%.2fMB", (float)nTransport / (1024.0 * 1024.0));
    }
    else if (nTransport > 1024)
    {
        strTransport = QString::asprintf("%dKB", (float)nTransport / 1024.0);
    }
    else
    {
        strTransport = QString::asprintf("%dByte", nTransport);
    }

    return strTransport;
}

void MyFileDownloader::ShowSingleTxt(QString txt)
{
    m_labelSingle->setText(txt);
}

void MyFileDownloader::SetSingleProg(int val)
{
    m_progressSingle->setValue(val);

}

void MyFileDownloader::ShowTotolTxt(QString txt)
{
    m_labelNum->setText(txt);
}

void MyFileDownloader::SetTotolProg(int val)
{
    m_progressNum->setValue(val);
}

bool MyFileDownloader::RecvPacketHeader(PACKET *pkt)
{
    return RecvData(reinterpret_cast<char *>(pkt), sizeof(PACKET));
}

bool MyFileDownloader::RecvData(char *pBuf, size_t nData)
{
    size_t nResidueSize = nData;
    size_t nRecvLen = 0;
    while(nResidueSize != 0)
    {
        boost::system::error_code ec;
        nRecvLen = m_sock.read_some(boost::asio::buffer(pBuf + (nData - nResidueSize), nResidueSize), ec);
        if (!ec && nRecvLen > 0)
        {
            nResidueSize -= nRecvLen;
        }
        else
        {
            return false;
        }

    }
    return true;
}
