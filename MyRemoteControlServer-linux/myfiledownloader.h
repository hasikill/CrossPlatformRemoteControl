#ifndef MYFILEDOWNLOADER_H
#define MYFILEDOWNLOADER_H

#include <QDialog>
#include <QThread>
#include <QProgressBar>
#include <QVBoxLayout>
#include <QLabel>
#include "common.h"
#include "mypacketsender.h"
#include <boost/asio.hpp>

#define BUFFERSIZE (1024 * 1024 * 10)

class MyFileDownloader : public QThread
{
    Q_OBJECT
public:
    MyFileDownloader(boost::asio::io_context &context,
                     MyPacketSender *sender,
                     QWidget *parent,
                     QString dlPath,
                     QString svPath,
                     COMMAND_SUB nMode);

    virtual void run();
    QString getStringSize(qint64 nTransport);

signals:
    void sigShowSingleTxt(QString txt);
    void sigSetSingleProg(int val);
    void sigShowTotolTxt(QString txt);
    void sigSetTotolProg(int val);

public slots:
    void ShowSingleTxt(QString txt);
    void SetSingleProg(int val);
    void ShowTotolTxt(QString txt);
    void SetTotolProg(int val);
private:
    bool RecvPacketHeader(PACKET *pkt);
    bool RecvData(char *pBuf, size_t nData);

private:
    QDialog *m_dlg;
    QString m_dlPath;
    QString m_svPath;
    COMMAND_SUB m_nMode;
    MyPacketSender *m_sender;
    tcp::acceptor m_acceptor;
    tcp::socket m_sock;
    QLabel *m_labelNum;
    QLabel *m_labelSingle;
    QProgressBar *m_progressSingle;
    QProgressBar *m_progressNum;
};

#endif // MYFILEDOWNLOADER_H
