#include "myremoteprocessview.h"
#include <QJsonDocument>
#include <QJsonParseError>
#include <QJsonObject>
#include <QJsonArray>
#include <QResizeEvent>
#include <QMenu>
#include "mainwindow.h"

MyRemoteProcessView::MyRemoteProcessView(MyPacketSender *sender, QWidget *parent) : QDialog (parent)
{

    m_sender = sender;
    m_pTable = new QTableWidget(this);
    m_pTable->setColumnCount(4);
    QStringList listTitle;
    listTitle << "进程ID" << "进程名" << "父进程ID" << "线程数";
    m_pTable->setHorizontalHeaderLabels(listTitle);
    m_pTable->setColumnWidth(0, 50);
    m_pTable->setColumnWidth(1, 250);
    //m_pTable->setSortingEnabled(true); 还有bug

    //设置右键菜单
    m_pTable->setContextMenuPolicy(Qt::ActionsContextMenu);

    //添加动作
    QAction *action1 = new QAction("结束进程");
    QAction *action2 = new QAction("刷新进程");
    m_pTable->addAction(action1);
    m_pTable->addAction(action2);

    //添加信号处理
    QObject::connect(action1, &QAction::triggered, this, &MyRemoteProcessView::onKillProcess);
    QObject::connect(action2, &QAction::triggered, this, &MyRemoteProcessView::onFlushProcess);

    resize(542, 298);
    m_nProcessCount = 0;
}

void MyRemoteProcessView::showProcessData(char *data)
{
    m_jDoc = data;
}

void MyRemoteProcessView::resizeEvent(QResizeEvent *ev)
{
    ev->accept();
    m_pTable->setGeometry(0, 0, width(), height());
}

void MyRemoteProcessView::slotFlush()
{
    //解析数据并且显示
    QJsonParseError json_error;
    QJsonDocument jDoc = QJsonDocument::fromJson(m_jDoc.toLocal8Bit(), &json_error);
    if(json_error.error == QJsonParseError::NoError)
    {
        //取json
        if (jDoc.isObject() == false) return;
        //取出对象
        QJsonObject jProcess = jDoc.object();
        //数量
        int count = jProcess["count"].toInt();
        m_nProcessCount = count;
        //进程list
        QJsonArray jList = jProcess["list"].toArray();
        //清空原数据
        m_pTable->setRowCount(0);
        for (int i = 0; i < jList.size(); i++)
        {
            //拿单个进程信息
            QJsonObject obj = jList[i].toObject();
            //添加新行
            m_pTable->insertRow(i);
            //进程ID
            QTableWidgetItem *newItem = new QTableWidgetItem(QObject::tr("%1").arg(obj["pId"].toInt()));
            newItem->setData(0, obj["pId"].toInt());
            m_pTable->setItem(i, 0, newItem);
            //进程名
            QString strName = obj["fileName"].toString();
            if (obj["bit"].toString() == "32")
            {
                strName += " *32";
            }
            newItem = new QTableWidgetItem(strName);
            m_pTable->setItem(i, 1, newItem);
            //父进程ID
            newItem = new QTableWidgetItem(QObject::tr("%1").arg(obj["parentId"].toInt()));
            m_pTable->setItem(i, 2, newItem);
            //线程数
            newItem = new QTableWidgetItem(QObject::tr("%1").arg(obj["threadNum"].toInt()));
            m_pTable->setItem(i, 3, newItem);
        }
    }

    tcp::endpoint pt = m_sender->socket()->remote_endpoint();
    setWindowTitle(QString::asprintf("远程进程 num=%d [", m_nProcessCount) + QString::asprintf("%s:%d", pt.address().to_string().c_str(), pt.port()) + "]");

    if (isHidden())
    {
        show();
    }

}

void MyRemoteProcessView::onKillProcess()
{
    //取进程ID
    int nCurRow = m_pTable->currentRow();
    QTableWidgetItem *item = m_pTable->item(nCurRow, 0);
    int pId = item->data(0).toInt();
    //组包
    m_sender->sendToKillProcess(pId);
}

void MyRemoteProcessView::onFlushProcess()
{
    m_sender->sendToProcess();
}
