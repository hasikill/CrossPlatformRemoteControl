#include "myserver.h"

MyServer::MyServer(boost::asio::io_context &context, unsigned short port) :
    io_context(context), m_acceptor(io_context, tcp::endpoint(tcp::v4(), port))
{
}

QMap<tcp::socket*, MyClientSession* > &MyServer::clients()
{
    return m_mapClients;
}

void MyServer::removeClient(tcp::socket *ps)
{
    if (ps == nullptr) return;

    m_mtxClients.lock();
    m_mapClients.remove(ps);
    m_mtxClients.unlock();
}

void MyServer::run()
{
    io_context.run();
}

void MyServer::do_accept(std::function<void(tcp::socket*)> cb_accept,
                         std::function<bool(PACKET *, MyPacketSender *, MyClientSession*)> PacketProc,
                         std::function<bool(MyClientSession*, tcp::endpoint)> exitProc,
                         std::function<void(tcp::socket*, boost::system::error_code ec, size_t)> fnWriteProc)
{
    m_acceptor.async_accept([this, cb_accept, PacketProc, exitProc, fnWriteProc](er_code ec, tcp::socket s)
    {
        if (!ec)
        {
            tcp::socket *ps = new tcp::socket(std::move(s));
            m_mtxClients.lock();
            //add in map
            auto client = new MyClientSession(ps, PacketProc, [this, exitProc](MyClientSession* client, tcp::endpoint pt)->bool
            {
                removeClient(client->sender()->socket());
                exitProc(client, pt);
                return true;
            },
            fnWriteProc,
            io_context);
            m_mapClients.insert(ps, client);
            //start async recv
            client->start();
            m_mtxClients.unlock();
            //notic
            cb_accept(ps);
        }
        //send accept again
        do_accept(cb_accept, PacketProc, exitProc, fnWriteProc);
    });
}
