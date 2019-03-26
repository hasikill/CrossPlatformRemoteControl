#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QScrollArea>
#include "myserver.h"
#include "myclientsession.h"
#include "myshowscreen.h"
#include <QMessageBox>
#include <QListWidgetItem>

#define MYLOINT(X) static_cast<unsigned int>(X & 0xFFFFFFFF)
#define MYHIINT(X) static_cast<unsigned int>((X >> 32) & 0xFFFFFFFF)
#define MYMAKELONG(a, b) static_cast<long>((static_cast<long>(a) << 32) | b);

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

private:
    void initUi();

    void startServer();

    void packetProc(PACKET *pkt, MyPacketSender *sender, MyClientSession *client);

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    MyPacketSender* getCurSender();
    MyClientSession* getCurClientSession();
    void saveShot(const uchar* data, uint nLen);

    //信号
signals:
    void sigUpdateList();
    //设置显示属性
    void sigFlushImg();
    void sigShowMsg(QString msg);
    void sigMsgBox(QString msg);
    void sigCreateView(MyClientSession *client);
    void sigReleaseUi(MyClientSession *client);

    void sigShowSingleTxt(QString txt);
    void sigSetSingleProg(int val);
    void sigShowTotolTxt(QString txt);
    void sigSetTotolProg(int val);
    void sigDisableActStart(bool flag);
    void sigRemoveClientItem(tcp::endpoint *pt);
    void sigAddClientItem(tcp::socket *ps);

private slots:
    void on_listWidget_customContextMenuRequested(const QPoint &pos);
    void slotUpdateList();
    void OnScreen();
    void OnStopScreen();
    void slotShowMsg(QString msg);
    void slotMsgBox(QString msg);
    void createView(MyClientSession *client);
    void releaseUi(MyClientSession *client);
    void disableActStart(bool flag);
    void removeClientItem(tcp::endpoint *pt);
    void addClientItem(tcp::socket *ps);

    void on_actions_screen_triggered();

    void on_action_file_triggered();

    void on_action_cmd_triggered();

    void on_action_exec_triggered();

    void on_action_shot_triggered();

    void on_action_process_triggered();

    void on_action_defin_high_triggered();

    void on_action_comm_triggered();

    void on_action_blur_triggered();

private:
    Ui::MainWindow *ui;
    QDockWidget *dockBottom;
    QDockWidget *dockRight;
    QScrollArea *m_scorllArea;
    MyServer *m_pServer;
    boost::asio::io_context io_context;
    QMenu *m_pMenu;

    MyShowScreen *m_screen;
    MyPacketSender *m_pPrevSender;
    QListWidgetItem *m_pPrevItem;
    QAction *m_actStart;
    QAction *m_actStop;

    friend class MyShowScreen;
    friend class MyScreenView;
};

#endif // MAINWINDOW_H
