#ifndef MYPACKETSENDER_H
#define MYPACKETSENDER_H

#include "common.h"
#include <boost/asio.hpp>
#include <QString>
#include <QSize>
#include <QMutex>

using boost::asio::ip::tcp;
#define MAX_PATH 1024

class MyPacketSender
{
public:
    MyPacketSender(tcp::socket *s, std::function<void(tcp::socket*, boost::system::error_code ec, size_t)> WriteProc);

    void sendToScreen(int m_nDefinition);
    void sendToScreenStop();
    void sendShowFile(char* path);
    void sendToShot();
    void sendToProcess();
    void sendToCmdInit();
    void sendToCmdExec(QString cmd);
    void SendToControl(MYINPUT *input);
    void sendToKillProcess(int pId);
    void sendDownloadFile(u_short port, QString dlPath, COMMAND_SUB sub);

    bool encSend(PACKET *pkt);
    bool isOnScreen();
    void setScreen(bool bFlag);
    tcp::socket* socket();

private:
    QMutex m_mtxSend;
    bool m_bScreen;
    tcp::socket *m_sock;
    std::function<void(tcp::socket*, boost::system::error_code ec, size_t)> m_fnWriteProc;
};

#endif // MYPACKETSENDER_H
