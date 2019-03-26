#include "mycmdview.h"

#include <QDockWidget>
#include <QResizeEvent>
#include "mainwindow.h"

MyCmdView::MyCmdView(MyPacketSender *sender, QWidget *p) : QDialog (p)
{
    m_sender = sender;
    tcp::endpoint pt = sender->socket()->remote_endpoint();
    setWindowTitle("远程终端 [" + QString::asprintf("%s:%d", pt.address().to_string().c_str(), pt.port()) + "]");

    //界面
    m_pTxtEcho = new QTextEdit(this);
    m_pTxtEcho->setReadOnly(true);

    //命令输入
    m_pTxtInput = new QTextEdit(this);
    m_pTxtInput->installEventFilter(this);

    m_pTxtInput->setFocus();
    resize(800, 600);
}

void MyCmdView::resizeEvent(QResizeEvent *ev)
{
    ev->accept();

    m_pTxtEcho->setGeometry(0, 0, width(), static_cast<int>(height() * 0.7));
    m_pTxtInput->setGeometry(0, static_cast<int>(height() * 0.7), width(), height() - m_pTxtEcho->height());
}

void MyCmdView::execCmd()
{
    if (m_pTxtInput->toPlainText().isEmpty())
    {
        return;
    }
    m_sender->sendToCmdExec(m_pTxtInput->toPlainText());
    //清空输入框
    m_pTxtInput->clear();
}

bool MyCmdView::eventFilter(QObject *target, QEvent *event)
{
    if (target == m_pTxtInput)
    {
        if(event->type() == QEvent::KeyPress)//回车键
        {
            QKeyEvent *k = static_cast<QKeyEvent *>(event);
            if (k->key() == Qt::Key_Return)
            {
                execCmd();
                return true;
            }
        }
    }
    return QWidget::eventFilter(target, event);
}

void MyCmdView::slotFlush(QString data)
{
    m_buffer += data;
    if (!m_buffer.isEmpty())
    {
        m_pTxtEcho->setText(m_buffer);
        m_pTxtEcho->moveCursor(QTextCursor::End);
    }
    if (isHidden())
    {
        show();
    }
}
