#include "myremotefileview.h"
#include <QResizeEvent>
#include "mainwindow.h"
#include <QByteArray>
#include <QPixmap>
#include <QFileDialog>
#include "myfiledownloader.h"

MyRemoteFileView::MyRemoteFileView(QWidget *parent,
                                   MyPacketSender *sender,
                                   boost::asio::io_context &ctx)
    : QDialog (parent), io_context(ctx)
{
    tcp::endpoint pt = sender->socket()->remote_endpoint();
    setWindowTitle("远程文件 [" + QString::asprintf("%s:%d", pt.address().to_string().c_str(), pt.port()) + "]");
    m_tree = new QTreeWidget(this);
    m_tree->setColumnCount(4);
    QStringList listTitle;
    listTitle << "文件名" << "创建时间" << "修改时间" << "文件大小";
    m_tree->setHeaderLabels(listTitle);
    m_tree->setColumnWidth(0, 220);
    m_tree->setColumnWidth(1, 150);
    m_tree->setColumnWidth(2, 150);
    m_tree->setSortingEnabled(true);

    //设置右键菜单
    m_tree->setContextMenuPolicy(Qt::ActionsContextMenu);

    QAction *act3 = new QAction(tr("刷新显示"), this);
    QAction *act2 = new QAction(tr("重新获取"), this);
    QAction *act1 = new QAction(tr("下载文件"), this);

    m_tree->addAction(act3);
    m_tree->addAction(act2);
    m_tree->addAction(act1);

    connect(act2, &QAction::triggered, this, &MyRemoteFileView::slotFlushData);
    connect(act1, &QAction::triggered, this, &MyRemoteFileView::slotDownload);
    connect(act3, &QAction::triggered, this, &MyRemoteFileView::slotUpdate);
    connect(m_tree, &QTreeWidget::itemExpanded, this, &MyRemoteFileView::slotItemExpanded);

    resize(700, 400);
    m_sender = sender;

    //创建下载目录
    QDir().mkdir(DOWNLOADPATH);
    m_bInit = false;
}

void MyRemoteFileView::resizeEvent(QResizeEvent *ev)
{
    ev->accept();
    m_tree->setGeometry(0, 0, width(), height());
}

void MyRemoteFileView::showFile()
{
    if (m_bInit)
    {
        if (m_tree->topLevelItemCount() != 0)
        {
            slotFlush();
            return;
        }
    }
    else
    {
        m_tree->clear();
        m_bInit = true;
    }
    //传nullptr表示跟目录
    m_sender->sendShowFile(nullptr);
}

bool MyRemoteFileView::isChildExist(QTreeWidgetItem *parentItem, QDomElement &eChild)
{
    for (int i = 0; i < parentItem->childCount(); i++)
    {
        QTreeWidgetItem *item = parentItem->child(i);
        if (eChild.tagName() == item->data(0, Qt::UserRole+1) && eChild.attribute("name") == item->text(0))
        {
            return true;
        }
    }
    return false;
}

void MyRemoteFileView::buildItem(QTreeWidgetItem *parentItem, QDomElement &e)
{
    //添加二级目录列表
    QDomNodeList list = e.childNodes();
    for (int i = 0; i < list.count(); i++)
    {
        //取出二级文件
        QDomNode n = list.at(i);
        QDomElement eChild = n.toElement();
        //判断是否存在
        if (isChildExist(parentItem, eChild))
        {
            continue;
        }
        //子项
        QTreeWidgetItem *pChildItem = new QTreeWidgetItem();
        //判断是文件还是目录
        pChildItem->setData(0, Qt::UserRole, eChild.attribute("path"));
        pChildItem->setText(0, eChild.attribute("name"));
        pChildItem->setText(1, eChild.attribute("createTime"));
        pChildItem->setText(2, eChild.attribute("changeTime"));

        QString strTag = eChild.tagName();
        if (eChild.tagName() == "DIR")
        {
            pChildItem->setIcon(0, QIcon(":/res/dir.png"));
            pChildItem->setData(0, Qt::UserRole+1, "DIR");
            pChildItem->setData(0, Qt::UserRole+2, 0);
        }
        else if (eChild.tagName() == "FILE")
        {
            QString strFileSize;
            int nFileSize = eChild.attribute("size").toInt();
            if (nFileSize > 1024 * 1024 * 1024)
            {
                strFileSize = QString::asprintf("%.2f GB", (float)nFileSize / (1024.0 * 1024.0 * 1024.0));
            }
            else if (nFileSize > 1024 * 1024)
            {
                strFileSize = QString::asprintf("%.2f MB", (float)nFileSize / (1024.0 * 1024.0));
            }
            else if (nFileSize > 1024)
            {
                strFileSize = QString::asprintf("%.2f KB", (float)nFileSize / 1024.0);
            }
            else
            {
                strFileSize = QString::asprintf("%d Byte", nFileSize);
            }
            pChildItem->setText(3, strFileSize);
            QString strBase64Icon = eChild.attribute("icon");
            pChildItem->setData(0, Qt::UserRole+1, "FILE");
            //转换
            QByteArray iconAry = QByteArray::fromBase64(strBase64Icon.toLocal8Bit());
            QPixmap iconPixmap;
            iconPixmap.loadFromData(iconAry);
            pChildItem->setIcon(0, iconPixmap);
        }
        //添加Driver
        parentItem->addChild(pChildItem);
    }
}

void MyRemoteFileView::ShowXmlData(QString szXml)
{
    //构建xml树
    QDomDocument doc;
    QString errmsg;
    int errline, errcol;
    if (!doc.setContent(szXml, &errmsg, &errline, &errcol))
    {
        qDebug() << errmsg;
        return;
    }
    //根节点
    QDomElement root = doc.documentElement();
    //取第一个子节点
    QDomNode node = root.firstChild();
    //遍历子节点
    while(!node.isNull())
    {
        //如果是元素
        if (node.isElement())
        {
            //转换为元素
            QDomElement e = node.toElement();
            //取元素名
            if (e.tagName() == "driver")
            {//磁盘驱动器
                //创建跟节点
                QTreeWidgetItem *pDriverItem = new QTreeWidgetItem();
                pDriverItem->setData(0, Qt::UserRole, e.attribute("name"));
                pDriverItem->setText(0, e.attribute("name").replace("\\", " "));
                pDriverItem->setIcon(0, QIcon(":/res/disk.png"));
                //构建treeWidget节点
                buildItem(pDriverItem, e);
                //添加到树控件
                m_tree->addTopLevelItem(pDriverItem);
            }
            else if (e.tagName() == "child")
            {//子目录
                QString path = e.attribute("path");
                QString name = e.attribute("name");
                //找到TreeWidgetItem节点
                QTreeWidgetItem *targetItem = EchoListItem[path];
                //如果找到
                if (targetItem != nullptr)
                {
                    //构建treeWidget节点
                    buildItem(targetItem, e);
                    //标记已遍历
                    targetItem->setData(0, Qt::UserRole+2, 1);
                }
            }
        }
        //往后遍历
        node = node.nextSibling();
    }

}

void MyRemoteFileView::slotFlush()
{
    if (isHidden())
    {
        show();
    }
}

void MyRemoteFileView::slotItemExpanded(QTreeWidgetItem *item)
{
    qDebug() << item->data(0, Qt::UserRole);

    for (int i = 0; i < item->childCount(); i ++)
    {
        QTreeWidgetItem *pChild = item->child(i);
        QString fileType = pChild->data(0, Qt::UserRole + 1).toString();
        int nFlag = pChild->data(0, Qt::UserRole + 2).toInt();
        if (fileType == "DIR" && nFlag == 0)
        {
            //路径
            QString path = pChild->data(0, Qt::UserRole).toString();
            //保存当前请求节点 等待回显
            if (!EchoListItem.contains(path))
            {
                EchoListItem.insert(path, pChild);
            }
            //请求客户端
            m_sender->sendShowFile(const_cast<char *>(path.toStdString().c_str()));
        }
    }
}

void MyRemoteFileView::slotFlushData()
{
    QTreeWidgetItem *item = m_tree->currentItem();
    if (item == nullptr)
    {
    }
    else
    {
        QString fileType = item->data(0, Qt::UserRole + 1).toString();
        if (fileType == "DIR")
        {
            //拿到path
            QString path = item->data(0, Qt::UserRole).toString();
            //请求包
            m_sender->sendShowFile(const_cast<char *>(path.toStdString().c_str()));
        }
    }
}

void MyRemoteFileView::slotDownload()
{
    QTreeWidgetItem *item = m_tree->currentItem();
    if (item == nullptr)
    {
        return;
    }
    //文件路径
    QString strPath = item->data(0, Qt::UserRole).toString();
    //类型
    QString fileType = item->data(0, Qt::UserRole + 1).toString();
    //名字
    QString name = item->text(0);
    QString fileName = DOWNLOADPATH;
    if (fileType == "DIR")
    {
        fileName = QFileDialog::getExistingDirectory(this, "设置下载目录", fileName);
        if (fileName.isEmpty())
        {
            return;
        }
        else
        {
            MyFileDownloader *downloader = new MyFileDownloader(io_context, m_sender, this, strPath, fileName, DOWNLOAD_DIR);
            downloader->start();
        }
    }
    else if (fileType == "FILE")
    {
        fileName += "/" + name;
        fileName = QFileDialog::getSaveFileName(this, "设置保存目录", fileName);
        if (fileName.isEmpty())
        {
            return;
        }
        else
        {
            MyFileDownloader *downloader = new MyFileDownloader(io_context, m_sender, this, strPath, fileName, DOWNLOAD_FILE);
            downloader->start();
        }
    }
}

void MyRemoteFileView::slotUpdate()
{
    QTreeWidgetItem *item = m_tree->currentItem();
    if (item == nullptr)
    {
        return;
    }
    QTreeWidgetItem *p = item->parent();
    if (p != nullptr)
    {
        p->setExpanded(false);
        p->setExpanded(true);
    }
}
