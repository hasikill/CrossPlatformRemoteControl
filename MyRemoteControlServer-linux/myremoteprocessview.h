#ifndef MYREMOTEPROCESSVIEW_H
#define MYREMOTEPROCESSVIEW_H

#include <QDialog>
#include <QTableWidget>
#include "mypacketsender.h"

class MyRemoteProcessView : public QDialog
{
public:
    MyRemoteProcessView(MyPacketSender *sender, QWidget *parent = nullptr);

    void showProcessData(char *data);
    void resizeEvent(QResizeEvent *ev);

public slots:
    void slotFlush();
    void onKillProcess();
    void onFlushProcess();

private:
    MyPacketSender *m_sender;
    QTableWidget *m_pTable;
    int m_nProcessCount;
    QString m_jDoc;
};

#endif // MYREMOTEPROCESSVIEW_H
