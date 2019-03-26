#include "myscreenview.h"
#include <QDebug>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QWheelEvent>
#include "mainwindow.h"
#include "ui_mainwindow.h"

MyScreenView::MyScreenView(QWidget *pMain) : QLabel (pMain)
{
    m_pMain = reinterpret_cast<MainWindow *>(pMain);
    m_sender = nullptr;
    m_bScreen = false;
    setMouseTracking(true);
}

void MyScreenView::setSocket(MyPacketSender *s)
{
    m_sender = s;
}

void MyScreenView::setScreenFlag(bool flag)
{
    m_bScreen = flag;
}

void MyScreenView::mousePressEvent(QMouseEvent *ev)
{
    setFocus();
    if (m_pMain->ui->actions_control->isChecked() == false || m_bScreen == false) return;
    MYINPUT input;
    memset(&input, 0, sizeof(MYINPUT));
    input.type = INPUT_MOUSE;
    //构建鼠标消息
    MYMOUSEINPUT mouseInput;
    memset(&mouseInput, 0, sizeof(MYMOUSEINPUT));
    mouseInput.dx = ev->pos().x() * 65535 / width();
    mouseInput.dy = ev->pos().y() * 65535 / height();
    mouseInput.mouseData = 0;
    if (ev->button() == Qt::LeftButton)
    {
        mouseInput.dwFlags = MOUSEEVENTF_LEFTDOWN;
    }
    else if (ev->button() == Qt::RightButton)
    {
        mouseInput.dwFlags = MOUSEEVENTF_RIGHTDOWN;
    }
    mouseInput.time = 0;
    mouseInput.dwExtraInfo = 0;

#ifdef __C89_NAMELESS
    input.mi = mouseInput;
#else
    input.DUMMYUNIONNAME.mi = mouseInput;
#endif

    //发送消息
    m_sender->SendToControl(&input);
}

void MyScreenView::mouseReleaseEvent(QMouseEvent *ev)
{
    setFocus();
    if (m_pMain->ui->actions_control->isChecked() == false || m_bScreen == false) return;
    MYINPUT input;
    memset(&input, 0, sizeof(MYINPUT));
    input.type = INPUT_MOUSE;
    //构建鼠标消息
    MYMOUSEINPUT mouseInput;
    memset(&mouseInput, 0, sizeof(MYMOUSEINPUT));
    mouseInput.dx = ev->pos().x();
    mouseInput.dy = ev->pos().y();
    mouseInput.mouseData = 0;
    if (ev->button() == Qt::LeftButton)
    {
        mouseInput.dwFlags = MOUSEEVENTF_LEFTUP;
    }
    else if (ev->button() == Qt::RightButton)
    {
        mouseInput.dwFlags = MOUSEEVENTF_RIGHTUP;
    }
    mouseInput.time = 0;
    mouseInput.dwExtraInfo = 0;
#ifdef __C89_NAMELESS
    input.mi = mouseInput;
#else
    input.DUMMYUNIONNAME.mi = mouseInput;
#endif
    //发送消息
    m_sender->SendToControl(&input);
}

void MyScreenView::mouseDoubleClickEvent(QMouseEvent *ev)
{

}

void MyScreenView::mouseMoveEvent(QMouseEvent *ev)
{
    if (m_pMain->ui->actions_control->isChecked() == false || m_bScreen == false) return;
    MYINPUT input;
    memset(&input, 0, sizeof(MYINPUT));
    input.type = INPUT_MOUSE;
    //构建鼠标消息
    MYMOUSEINPUT mouseInput;
    memset(&mouseInput, 0, sizeof(MYMOUSEINPUT));
    mouseInput.dx = ev->pos().x() * 65535 / width();
    mouseInput.dy = ev->pos().y() * 65535 / height();
    mouseInput.mouseData = 0;
    mouseInput.dwFlags = MOUSEEVENTF_MOVE | MOUSEEVENTF_ABSOLUTE;
    mouseInput.time = 0;
    mouseInput.dwExtraInfo = 0;
#ifdef __C89_NAMELESS
    input.mi = mouseInput;
#else
    input.DUMMYUNIONNAME.mi = mouseInput;
#endif
    //发送消息
    m_sender->SendToControl(&input);
}

void MyScreenView::wheelEvent(QWheelEvent *ev)
{
    if (m_pMain->ui->actions_control->isChecked() == false || m_bScreen == false) return;
    MYINPUT input;
    memset(&input, 0, sizeof(MYINPUT));
    input.type = INPUT_MOUSE;
    //构建鼠标消息
    MYMOUSEINPUT mouseInput;
    memset(&mouseInput, 0, sizeof(MYMOUSEINPUT));
    mouseInput.dx = 0;
    mouseInput.dy = 0;
    mouseInput.mouseData = static_cast<unsigned int>(ev->delta());
    mouseInput.dwFlags = MOUSEEVENTF_WHEEL;
    mouseInput.time = 0;
    mouseInput.dwExtraInfo = 0;
#ifdef __C89_NAMELESS
    input.mi = mouseInput;
#else
    input.DUMMYUNIONNAME.mi = mouseInput;
#endif
    //发送消息
    m_sender->SendToControl(&input);
}

void MyScreenView::keyPressEvent(QKeyEvent *ev)
{
    if (m_pMain->ui->actions_control->isChecked() == false || m_bScreen == false) return;
    MYINPUT input;
    memset(&input, 0, sizeof(MYINPUT));
    input.type = INPUT_KEYBOARD;
    //构建按键消息
    MYKEYBDINPUT keyInput;
    keyInput.wVk = static_cast<unsigned short>(ev->key());
    //keyInput.wScan = static_cast<unsigned short>(MapVirtualKeyA(keyInput.wVk, 0));
    keyInput.time = 0;
    keyInput.dwFlags = 0;
    keyInput.dwExtraInfo = 0;

#ifdef __C89_NAMELESS
    input.ki = keyInput;
#else
    input.DUMMYUNIONNAME.ki = keyInput;
#endif


    //发送消息
    m_sender->SendToControl(&input);
}

void MyScreenView::keyReleaseEvent(QKeyEvent *ev)
{
    if (m_pMain->ui->actions_control->isChecked() == false || m_bScreen == false) return;
    MYINPUT input;
    memset(&input, 0, sizeof(MYINPUT));
    input.type = INPUT_KEYBOARD;
    //构建按键消息
    MYKEYBDINPUT keyInput;
    keyInput.wVk = static_cast<unsigned short>(ev->key());
    //keyInput.wScan = static_cast<unsigned short>(MapVirtualKeyA(keyInput.wVk, 0));
    keyInput.time = 0;
    keyInput.dwFlags = KEYEVENTF_KEYUP;
    keyInput.dwExtraInfo = 0;

#ifdef __C89_NAMELESS
    input.ki = keyInput;
#else
    input.DUMMYUNIONNAME.ki = keyInput;
#endif

    //发送消息
    m_sender->SendToControl(&input);
}
