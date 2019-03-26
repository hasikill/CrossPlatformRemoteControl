#ifndef MYSERVER_H
#define MYSERVER_H

#include <boost/asio.hpp>
#include <QMap>
#include <memory>
#include <functional>
#include <QMutex>
#include <QThread>
#include "myclientsession.h"

using boost::asio::ip::tcp;
typedef boost::system::error_code er_code;

class MyServer : public QThread
{
public:
    MyServer(boost::asio::io_context &context, unsigned short port);

    QMap<tcp::socket *, MyClientSession*> &clients();

    void removeClient(tcp::socket *ps);

    void do_accept(std::function<void(tcp::socket*)> apt,
                   std::function<bool(PACKET *, MyPacketSender *, MyClientSession*)> PacketProc,
                   std::function<bool(MyClientSession*, tcp::endpoint)> exitProc,
                   std::function<void(tcp::socket *, boost::system::error_code, size_t)> fnWriteProc);

private:
    virtual void run();

private:
    //low-level IO Interface
    boost::asio::io_context &io_context;
    //acceptor
    tcp::acceptor m_acceptor;
    //session set
    QMap<tcp::socket *, MyClientSession*> m_mapClients;
    //mutex
    QMutex m_mtxClients;
};

#endif // MYSERVER_H
