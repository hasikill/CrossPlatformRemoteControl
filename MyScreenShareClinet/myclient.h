#ifndef MYCLIENT_H
#define MYCLIENT_H

#include <QThread>
#include <windows.h>
#include <winsock2.h>
#include <QMutex>
#include "common.h"
#include "myscreensender.h"
#include "mycmdexec.h"

class MainWindow;
class MyClient : public QThread
{
public:
    MyClient(MainWindow *pMain);

    virtual void run();
    bool sendCompressData(PACKET *packBuf, QByteArray &data);
    bool sendCompressData(PACKET *packBuf);

    void sendFlushPacket();

    void sendFileView(char *path);
    void sendProcessView();
    bool IsWow64(DWORD dwPid);
    void sendKillProcessStatus(int pId);
    void sendCmdInitStatus();
    void sendScreenStop();
    void sendCmdEcho(QString str);
    void execControl(char *input);
    void startDownload(int nMode, char *data);

    SOCKET m_SockClient;
    MyScreenSender *m_pScreenSender;
    MyCmdExec *m_pCmdExec;
    MainWindow *m_pMain;
    QMutex m_mutex;
    SOCKADDR_IN RemoteAddr;
};

#endif // MYCLIENT_H
