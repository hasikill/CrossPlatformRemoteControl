#ifndef MYCMDVIEW_H
#define MYCMDVIEW_H

#include <QDialog>
#include <QTextEdit>
#include <QMutex>
#include "mypacketsender.h"

class MyCmdView : public QDialog
{
public:
    MyCmdView(MyPacketSender *sender, QWidget *p = nullptr);

    void resizeEvent(QResizeEvent *ev);
    void execCmd();

    bool eventFilter(QObject *target, QEvent *event);//事件过滤器

public slots:
    void slotFlush(QString data);

private:
    MyPacketSender *m_sender;
    QTextEdit *m_pTxtEcho;
    QTextEdit *m_pTxtInput;
    QString m_buffer;
    QMutex m_mtx;
};

#endif // MYCMDVIEW_H
