#ifndef MYSCREENSENDER_H
#define MYSCREENSENDER_H

#include <QObject>
#include <QMutex>

class MyClient;
class MyScreenSender : public QObject
{
public:
    MyScreenSender(MyClient *pClient);
    void sendScreen(int nDef);

public slots:
    void slotSendScreen();
    bool isStart();

private:
    MyClient *m_pClient;
    int m_bHigh;
    QByteArray m_imgBytes;
    int m_nScreenH;
    int m_nScreenW;
    QMutex m_mtx;

    friend class MyClient;
};

#endif // MYSCREENSENDER_H
