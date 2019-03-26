#ifndef MYSCREENVIEW_H
#define MYSCREENVIEW_H

#include <QLabel>
#include "mypacketsender.h"

class MainWindow;
class MyScreenView : public QLabel
{
public:
    MyScreenView(QWidget *pMain);
    void setSocket(MyPacketSender *s);
    void setScreenFlag(bool flag);

    void mousePressEvent(QMouseEvent *event);        //按下
    void mouseReleaseEvent(QMouseEvent *event);      //释放
    void mouseDoubleClickEvent(QMouseEvent *event);  //双击
    void mouseMoveEvent(QMouseEvent *event);         //移动
    void wheelEvent(QWheelEvent *event);             //滑轮
    void keyPressEvent(QKeyEvent *event);
    void keyReleaseEvent(QKeyEvent *event);

private:
    bool m_bScreen;
    MainWindow *m_pMain;
    MyPacketSender *m_sender;
};

#endif // MYSCREENVIEW_H
