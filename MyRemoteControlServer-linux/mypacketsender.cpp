#include "mypacketsender.h"

MyPacketSender::MyPacketSender(tcp::socket *s, std::function<void(tcp::socket *, boost::system::error_code, size_t)> WriteProc)
{
    m_sock = s;
    m_fnWriteProc = WriteProc;
}

void MyPacketSender::sendToScreen(int m_nDefinition)
{
    if (m_sock == nullptr) return;
    char packBuf[1024] = { 0 };
    PACKET *pkt = reinterpret_cast<PACKET*>(packBuf);
    pkt->byCmdMain = ON_SCREEN;
    pkt->byCmdSub = START;
    //取屏幕显示宽高
    reinterpret_cast<int *>(pkt->Data)[0] = m_nDefinition;
    pkt->uLength = 4;
    //发包
    encSend(pkt);
}

void MyPacketSender::sendToScreenStop()
{
    if (m_sock == nullptr) return;
    char packBuf[128] = { 0 };
    PACKET *pkt = reinterpret_cast<PACKET*>(packBuf);
    pkt->byCmdMain = ON_SCREEN;
    pkt->byCmdSub = STOP;
    pkt->uLength = 0;
    //发包
    encSend(pkt);
}

void MyPacketSender::sendShowFile(char *path)
{
    if (m_sock == nullptr) return;
    char packBuf[1024] = { 0 };
    PACKET *pkt = reinterpret_cast<PACKET*>(packBuf);
    pkt->byCmdMain = ON_FILEVIEW;
    //传递路径
    if (path != nullptr)
    {
        strcpy(pkt->Data, path);
        pkt->uLength = strlen(path);
    }
    else
    {
        pkt->uLength = 0;
    }
    //发包
    encSend(pkt);
}

void MyPacketSender::sendToShot()
{
    if (m_sock == nullptr) return;
    char packBuf[16] = { 0 };
    PACKET *pkt = reinterpret_cast<PACKET*>(packBuf);
    pkt->byCmdMain = ON_SHOT;
    pkt->uLength = 0;
    //发包
    encSend(pkt);
}

void MyPacketSender::sendToProcess()
{
    if (m_sock == nullptr) return;
    char packBuf[16] = { 0 };
    PACKET *pkt = reinterpret_cast<PACKET*>(packBuf);
    pkt->byCmdMain = ON_PROCESS;
    pkt->byCmdSub = VIEW;
    pkt->uLength = 0;
    //发包
    encSend(pkt);
}

void MyPacketSender::sendToCmdInit()
{
    if (m_sock == nullptr) return;
    char packBuf[16] = { 0 };
    PACKET *pkt = reinterpret_cast<PACKET*>(packBuf);
    pkt->byCmdMain = ON_CMD;
    pkt->byCmdSub = INIT;
    pkt->uLength = 0;
    //发包
    encSend(pkt);
}

void MyPacketSender::sendToCmdExec(QString cmd)
{
    if (m_sock == nullptr) return;
    char packBuf[4096] = { 0 };
    PACKET *pkt = reinterpret_cast<PACKET*>(packBuf);
    pkt->byCmdMain = ON_CMD;
    pkt->byCmdSub = EXEC;
    QByteArray ary = cmd.toLocal8Bit();
    pkt->uLength = static_cast<size_t>(ary.size());
    memcpy(pkt->Data, ary, static_cast<size_t>(ary.size()));
    //发包
    encSend(pkt);
}

void MyPacketSender::SendToControl(MYINPUT *input)
{
    if (m_sock == nullptr) return;
    char packBuf[128] = { 0 };
    PACKET *pkt = reinterpret_cast<PACKET*>(packBuf);
    pkt->byCmdMain = ON_CONTROL;
    pkt->uLength = sizeof(MYINPUT);
    memcpy(pkt->Data, input, sizeof(MYINPUT));
    //发包
    encSend(pkt);
}

void MyPacketSender::sendToKillProcess(int pId)
{
    if (m_sock == nullptr) return;
    char packBuf[16] = { 0 };
    PACKET *pkt = reinterpret_cast<PACKET*>(packBuf);
    pkt->byCmdMain = ON_PROCESS;
    pkt->byCmdSub = KILLPROCESS;
    pkt->uLength = 4;
    *reinterpret_cast<int *>(pkt->Data) = pId;
    //发包
    encSend(pkt);
}

void MyPacketSender::sendDownloadFile(u_short port, QString dlPath, COMMAND_SUB sub)
{
    if (m_sock == nullptr) return;
    char packBuf[MAX_PATH + 20] = { 0 };
    PACKET *pkt = reinterpret_cast<PACKET*>(packBuf);
    pkt->byCmdMain = ON_DOWNLOAD;
    pkt->byCmdSub = sub;
    reinterpret_cast<u_short *>(pkt->Data)[0] = port;
    QByteArray ary = dlPath.toUtf8();
    memcpy(&reinterpret_cast<u_short *>(pkt->Data)[1], ary, static_cast<size_t>(ary.size()));
    pkt->uLength = static_cast<size_t>(ary.size() + 2);
    //发包
    encSend(pkt);
}

bool MyPacketSender::encSend(PACKET *pkt)
{
    if (m_sock == nullptr) return false;
    m_mtxSend.lock();
    m_sock->async_send(boost::asio::buffer(pkt, sizeof(PACKET) + pkt->uLength),
    [this](boost::system::error_code ec, std::size_t len)
    {
        m_fnWriteProc(m_sock, ec, len);
    });
    m_mtxSend.unlock();
    return true;
}

bool MyPacketSender::isOnScreen()
{
    return m_bScreen;
}

void MyPacketSender::setScreen(bool bFlag)
{
    m_bScreen = bFlag;
}

tcp::socket *MyPacketSender::socket()
{
    return m_sock;
}
