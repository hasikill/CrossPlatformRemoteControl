#include "myshowscreen.h"
#include <QPaintEvent>
#include <QPainter>
#include "mainwindow.h"

MyShowScreen::MyShowScreen(QWidget *parent) : QObject (parent)
{
    m_parent = parent;
    m_sender = nullptr;
    m_lb = new MyScreenView(m_parent);
    m_nDefinition = 2;
}

void MyShowScreen::SetScreenAttribute(MyPacketSender *s, int nW, int nH, const uchar *img, int nBytes)
{
    //所有客户端公用
    m_sender = s;
    m_lb->setSocket(s);

    //宽高
    m_nRemoteHeight = nH;
    m_nRemoteWidth = nW;

    //当前图片数据
    m_curImg.loadFromData(img, static_cast<size_t>(nBytes));
}

void MyShowScreen::flush()
{
    m_lb->setPixmap(m_curImg);
    reinterpret_cast<MainWindow *>(m_parent)->m_scorllArea->setWidget(m_lb);
    if (m_sender->isOnScreen())
    {
        m_sender->sendToScreen(m_nDefinition);
    }
}
