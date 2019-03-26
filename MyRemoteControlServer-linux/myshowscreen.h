#ifndef MYSHOWSCREEN_H
#define MYSHOWSCREEN_H

#include <QPixmap>
#include <QLabel>
#include <QObject>
#include "myscreenview.h"

class MyShowScreen : public QObject
{
public:
    MyShowScreen(QWidget *parent);
    void SetScreenAttribute(MyPacketSender *s, int nW, int nH, const uchar *img, int nBytes);
public slots:
    void flush();

    //清晰度
    int m_nDefinition;
    //远程屏幕宽高
    int m_nRemoteWidth;
    int m_nRemoteHeight;
    //当前图片数据
    QPixmap m_curImg;
    QWidget *m_parent;
    MyScreenView *m_lb;
    MyPacketSender *m_sender;
};

#endif // MYSHOWSCREEN_H
