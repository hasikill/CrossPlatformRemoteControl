#include "myscreensender.h"
#include <QScreen>
#include <QGuiApplication>
#include <QDateTime>
#include <QDebug>
#include <QApplication>
#include <QDesktopWidget>
#include <QPixmap>
#include <QBuffer>
#include "common.h"
#include "myclient.h"
#include "mainwindow.h"

MyScreenSender::MyScreenSender(MyClient *pClient) : m_pClient(pClient)
{
    m_nScreenW = QGuiApplication::primaryScreen()->size().width();
    m_nScreenH = QGuiApplication::primaryScreen()->size().height();
    m_mtx.lock();
}

void MyScreenSender::sendScreen(int nDef)
{
    m_bHigh = nDef;

    char packBuf[120];
    //发送截屏信号
    emit m_pClient->m_pMain->sigSendScreen();
    m_mtx.lock();
    //构建数据包
    PACKET *pkt = reinterpret_cast<PACKET *>(packBuf);
    pkt->byCmdMain = ON_SCREEN;
    pkt->byCmdSub = START;
    pkt->uLength = 8 + m_imgBytes.size();
    //客户端屏幕宽高
    reinterpret_cast<int *>(pkt->Data)[0] = m_nScreenW;
    reinterpret_cast<int *>(pkt->Data)[1] = m_nScreenH;

    //缓冲区
    QByteArray ary(pkt->Data, 8);
    ary.append(m_imgBytes);

    //压缩发送
    m_pClient->sendCompressData(pkt, ary);
}

void MyScreenSender::slotSendScreen()
{
    //屏幕对象
    QScreen *screen = QGuiApplication::primaryScreen();
    //截图
    QPixmap img = screen->grabWindow(QApplication::desktop()->winId());
    //转换buffer
    QBuffer buffer(&m_imgBytes);

    switch (m_bHigh)
    {
        case 1:
            img.save(&buffer, "JPG", 30);
            break;
        case 2:
            img.save(&buffer, "JPG", 60);
            break;
        case 3:
            img.save(&buffer, "PNG");
            break;
    }
    m_mtx.unlock();
}
