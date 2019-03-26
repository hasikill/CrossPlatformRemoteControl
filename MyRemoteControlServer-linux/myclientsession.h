#ifndef MYCLIENTSESSION_H
#define MYCLIENTSESSION_H

#include "common.h"
#include <boost/asio.hpp>
#include <memory>
#include <utility>
#include <cstdlib>
#include <iostream>
#include <functional>
#include <QQueue>
#include <QMutex>
#include "mypacketsender.h"
#include "mycmdview.h"
#include "myremoteprocessview.h"
#include "myremotefileview.h"
#include "mybuffer.h"

using boost::asio::ip::tcp;

#define TEMP_BUF 1024 * 1024 * 2
static char g_Buf[TEMP_BUF];

class MyClientSession : public QObject
{
    Q_OBJECT
public:
    MyClientSession(tcp::socket *ps,
                    std::function<bool(PACKET *,MyPacketSender *, MyClientSession*)> fnPacketPRoc,
                    std::function<bool(MyClientSession*, tcp::endpoint)> fnExitProc,
                    std::function<void(tcp::socket*, boost::system::error_code ec, size_t)> fnWriteProc,
                    boost::asio::io_context &io_ctx);
    ~MyClientSession();

    void start();
    void onHeart();
    long getPrevHeartTime();
    void showProcessView(char *xml);
    void showFileView(QString xml);
    void openFileView();
    MyPacketSender* sender();
    void close();

signals:
    void sigFlushCmd(QString data);
    void sigFlushProcess();
    void sigFlushFile();

public slots:
    void createUi(QWidget *parent);

private:
    void do_read();
    int addBufferBytes(char *buf, int nLen);
    bool look();
    PACKET *popPacket();

private:
    std::function<bool(PACKET *, MyPacketSender *, MyClientSession *)> m_fnPacketProc;
    std::function<bool(MyClientSession*, tcp::endpoint)> m_fnExitProc;
    std::function<void(tcp::socket*, boost::system::error_code ec, size_t)> m_fnWriteProc;
    tcp::socket* m_sock;
    tcp::endpoint m_addr;
    MyPacketSender *m_sender;
    MyBuffer m_buf;
    long m_lLastHeartTime;
    QMutex m_mtxRecv;
    QQueue<PACKET *> m_quePkt;
    boost::asio::io_context &io_context;

    //ui
    MyCmdView *m_viewCmd;
    MyRemoteProcessView *m_viewProcess;
    MyRemoteFileView *m_viewFile;
};

#endif // MYCLIENTSESSION_H
