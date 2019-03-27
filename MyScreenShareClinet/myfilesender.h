#ifndef MYFILESENDER_H
#define MYFILESENDER_H

#include <QThread>
#include <winsock2.h>

class MyClient;
class MyFileSender : public QThread
{
public:
    MyFileSender(MyClient *client, ushort port, QString dlPath, int nMode);

    bool sendFileData(QString fileName, int nOffset);
    virtual void run();
    bool init();

private:
    MyClient *m_pClient;
    QString m_dlPath;
    int m_nMode;
    SOCKET m_Socket;
    ushort m_uPort;
};

#endif // MYFILESENDER_H
