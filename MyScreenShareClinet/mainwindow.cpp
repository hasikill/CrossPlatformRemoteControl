#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QScreen>
#include <QGuiApplication>
#include <QDateTime>
#include <QDebug>
#include <QApplication>
#include <QDesktopWidget>
#include <QPixmap>
#include <QBuffer>
#include <QByteArray>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    m_client = new MyClient(this);

    connect(this, &MainWindow::sigSendScreen, m_client->m_pScreenSender, &MyScreenSender::slotSendScreen);
    connect(this, &MainWindow::sendScreenShot, this, &MainWindow::ScreenShot);

    m_client->start();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::ScreenShot()
{
    //屏幕对象
    QScreen *screen = QGuiApplication::primaryScreen();

    //截图
    QPixmap img = screen->grabWindow(QApplication::desktop()->winId());
    //转换buffer
    QByteArray bytes;
    QBuffer buffer(&bytes);
    img.save(&buffer, "BMP");

    //构建数据包
    char packBuf[120];
    PACKET *pkt = reinterpret_cast<PACKET *>(packBuf);
    pkt->byCmdMain = ON_SHOT;
    pkt->byCmdSub = 0;
    pkt->uLength = static_cast<size_t>(bytes.size());

    //压缩发送
    m_client->sendCompressData(pkt, bytes);
}
