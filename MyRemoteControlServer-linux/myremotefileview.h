#ifndef MYREMOTEFILEVIEW_H
#define MYREMOTEFILEVIEW_H
#include <QDialog>
#include <QTreeWidget>
#include <QtXml>
#include <QMenu>
#include <QMap>
#include "mypacketsender.h"
#include <boost/asio.hpp>

#define DOWNLOADPATH "./Download"

class MyRemoteFileView : public QDialog
{
public:
    MyRemoteFileView(QWidget *parent, MyPacketSender *sender, boost::asio::io_context &ctx);

    void resizeEvent(QResizeEvent *ev);
    void showFile();
    void ShowXmlData(QString szXml);

private:
    bool isChildExist(QTreeWidgetItem *parentItem, QDomElement &eChild);
    void buildItem(QTreeWidgetItem *parentItem, QDomElement &e);

public slots:
    void slotFlush();
    void slotItemExpanded(QTreeWidgetItem *item);
    void slotFlushData();
    void slotDownload();
    void slotUpdate();

private:
    QTreeWidget *m_tree;
    boost::asio::io_context &io_context;
    MyPacketSender *m_sender;
    QMap<QString, QTreeWidgetItem *> EchoListItem;
    bool m_bInit;
};

#endif // MYREMOTEFILEVIEW_H
