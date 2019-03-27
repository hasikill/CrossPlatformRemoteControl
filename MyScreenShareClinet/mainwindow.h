#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <windows.h>
#include <winsock2.h>
#include "myclient.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

signals:
    void sigSendScreen();
    void sendScreenShot();

public slots:
    void ScreenShot();

private:
    Ui::MainWindow *ui;
    MyClient *m_client;
};

#endif // MAINWINDOW_H
