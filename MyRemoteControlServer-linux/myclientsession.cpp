#include "myclientsession.h"
#include <QtDebug>

MyClientSession::MyClientSession(tcp::socket *ps,
                                 std::function<bool(PACKET *, MyPacketSender *, MyClientSession *)> fnPacketPRoc,
                                 std::function<bool(MyClientSession*, tcp::endpoint)> exitProc,
                                 std::function<void(tcp::socket *, boost::system::error_code, size_t)> fnWriteProc,
                                 boost::asio::io_context &io_ctx)
    : m_fnPacketProc(fnPacketPRoc), m_fnExitProc(exitProc), m_sock(ps), io_context(io_ctx)
{
    m_fnWriteProc = fnWriteProc;
    m_lLastHeartTime = time(nullptr);
    m_sender = new MyPacketSender(m_sock, m_fnWriteProc);
    m_addr = m_sock->remote_endpoint();

    m_viewCmd = nullptr;
    m_viewProcess = nullptr;
    m_viewFile = nullptr;
}

MyClientSession::~MyClientSession()
{
    qDebug() << "exit";

    if (m_sender != nullptr)
    {
        delete m_sender;
        m_sender = nullptr;
    }

    close();

    if (m_sock != nullptr)
    {
        delete m_sock;
        m_sock = nullptr;
    }

    if (m_viewCmd != nullptr)
    {
        delete m_viewCmd;
        m_viewCmd = nullptr;
    }

    if (m_viewFile != nullptr)
    {
        delete m_viewFile;
        m_viewFile = nullptr;
    }

    if (m_viewProcess != nullptr)
    {
        delete m_viewProcess;
        m_viewProcess = nullptr;
    }
}

void MyClientSession::start()
{
    do_read();
}

void MyClientSession::showProcessView(char* xml)
{
    m_viewProcess->showProcessData(xml);
}

void MyClientSession::onHeart()
{
    m_lLastHeartTime = time(nullptr);
}

long MyClientSession::getPrevHeartTime()
{
    return m_lLastHeartTime;
}

void MyClientSession::showFileView(QString xml)
{
    m_viewFile->ShowXmlData(xml);
}

void MyClientSession::openFileView()
{
    m_viewFile->showFile();
}

MyPacketSender *MyClientSession::sender()
{
    return m_sender;
}

void MyClientSession::close()
{
    if (m_sock->is_open())
    {
        m_sock->close();
    }
}

void MyClientSession::createUi(QWidget *parent)
{
    //ui
    m_viewCmd = new MyCmdView(m_sender, parent);
    m_viewProcess = new MyRemoteProcessView(m_sender, parent);
    m_viewFile = new MyRemoteFileView(parent, m_sender, io_context);

    //信号
    connect(this, &MyClientSession::sigFlushCmd, m_viewCmd, &MyCmdView::slotFlush);
    connect(this, &MyClientSession::sigFlushProcess, m_viewProcess, &MyRemoteProcessView::slotFlush);
    connect(this, &MyClientSession::sigFlushFile, m_viewFile, &MyRemoteFileView::slotFlush);
}

void MyClientSession::do_read()
{
    //async request
    m_sock->async_read_some(boost::asio::buffer(g_Buf, TEMP_BUF),
    [this](boost::system::error_code ec, std::size_t length)
    {
        if (!ec)
        {
            m_mtxRecv.lock();
            qDebug() << "thread: " << QThread::currentThreadId() << ", this: " << this << length;
            //spell pack
            addBufferBytes(g_Buf, static_cast<int>(length));
            while(look())
            {
                //get packet
                PACKET *pkt = popPacket();
                if (pkt == nullptr)
                {
                    break;
                }
                //packet process
                m_fnPacketProc(pkt, m_sender, this);
            }
            m_mtxRecv.unlock();

            //async request
            memset(g_Buf, 0, TEMP_BUF);
            do_read();
        }
        else
        {
            //客户端退出
            qDebug() << "Error: " << QString::fromStdString(ec.message());
            //通知外部
            m_fnExitProc(this, m_addr);
        }
    });
}

int MyClientSession::addBufferBytes(char *buf, int nLen)
{
    if (nLen <= 0 || buf == nullptr)
    {
        return false;
    }

    //拷贝数据
    m_buf.cpData(buf, static_cast<size_t>(nLen));
    int nResidu = 0;

    do
    {
        //剩余字节数
        nResidu = static_cast<int>(m_buf.offset() - sizeof(PACKET));

        //情况1
        if (nResidu < 0)
        {
            break;
        }

        //情况2
        PACKET *pkt = reinterpret_cast<PACKET *>(m_buf.data());
        nResidu = static_cast<int>(m_buf.offset()) - static_cast<int>(sizeof(PACKET) + pkt->uLength);

        //情况3
        if (nResidu >= 0)
        {
            //包指针
            size_t nBufSize = sizeof(PACKET) + pkt->uLength;
            size_t nRealSize = nBufSize;

            // 1.取包 解压缩
            if (pkt->uLength > 0)
            {
                //解压
                QByteArray unData = qUncompress(QByteArray(pkt->Data, static_cast<int>(pkt->uLength)));
                //构建包缓冲区
                pkt->uLength = static_cast<size_t>(unData.size());
                //长度
                nBufSize = sizeof(PACKET) + pkt->uLength;
                //拷贝数据
                char *tmpBuf = new char[nBufSize + 2];
                memset(tmpBuf, 0, nBufSize + 2);
                memcpy(tmpBuf, m_buf.data(), sizeof(PACKET));
                memcpy(tmpBuf + sizeof(PACKET), unData, static_cast<size_t>(unData.size()));
                //添加到队列
                m_quePkt.enqueue(reinterpret_cast<PACKET *>(tmpBuf));
            }
            else
            {
                //拷贝数据
                char *tmpBuf = new char[nBufSize + 2];
                memset(tmpBuf, 0, nBufSize + 2);
                memcpy(tmpBuf, m_buf.data(), nBufSize);
                //添加到队列
                m_quePkt.enqueue(reinterpret_cast<PACKET *>(tmpBuf));
            }

            //转变情况1和情况2
            if (nResidu > 0)
            {
                m_buf.moveLeft(nRealSize, static_cast<size_t>(nResidu));
            }
            else if (nResidu == 0)
            {
                m_buf.clear();
            }
        }
    } while (nResidu > 0);

    return true;
}

bool MyClientSession::look()
{
    return !m_quePkt.isEmpty();
}

PACKET* MyClientSession::popPacket()
{
    if (!m_quePkt.isEmpty())
    {
        return m_quePkt.dequeue();
    }
    return nullptr;
}
