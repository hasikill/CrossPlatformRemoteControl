#include "myclient.h"
#include "mainwindow.h"
#include <QtXml>
#include <QDir>
#include <QFileIconProvider>
#include <QMessageBox>
#include <windows.h>
#include <tlhelp32.h>
#include <QJsonArray>
#include <QJsonDocument>
#include <windows.h>
#include "myfilesender.h"
#include <thread>
#include <QSettings>

MyClient::MyClient(MainWindow *pMain) : m_pMain(pMain)
{
    //初始化协议
        WSADATA ws;
        if (WSAStartup(static_cast<WORD>(MAKEWORD(2, 2)), &ws) != 0)
        {
            qDebug("WSAStartup失败！错误码 : %d", WSAGetLastError());
            throw "WSAStartup失败！\r\n";
        }
        m_SockClient = 0;
        m_pScreenSender = new MyScreenSender(this);
        m_pCmdExec = new MyCmdExec(this);
}

void MyClient::run()
{

    //开始连接
    if (m_SockClient != 0 && m_SockClient != INVALID_SOCKET)
    {
        closesocket(m_SockClient);
    }
    //创建socket
    m_SockClient = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (m_SockClient == INVALID_SOCKET)
    {
        qDebug("socket失败！错误码 : %d", WSAGetLastError());
        throw "socket失败！\r\n";
    }

    //读配置文件
    QString strIp = "127.0.0.1";
    if (QFile("config.ini").exists())
    {
        QSettings *config = new QSettings("config.ini", QSettings::IniFormat);
        QString str = config->value("address/ip").toString();
        if (!str.isEmpty())
        {
            strIp = str;
        }
    }

    //设置连接信息
    memset(&RemoteAddr, 0, sizeof(SOCKADDR_IN));
    RemoteAddr.sin_family = AF_INET;
    RemoteAddr.sin_port = htons(9988);
    RemoteAddr.sin_addr.S_un.S_addr = inet_addr(strIp.toStdString().c_str());
    //开始连接
    int nRet = ::connect(m_SockClient, reinterpret_cast<SOCKADDR*>(&RemoteAddr), sizeof(SOCKADDR_IN));
    if (nRet == SOCKET_ERROR)
    {
        qDebug("connect失败！错误码 : %d", WSAGetLastError());
        throw "connect失败！\r\n";
    }
    //连接成功
    qDebug("服务器连接成功!");

    //启动心跳发送线程
    std::thread([&](){
        while(true)
        {
            //组包
            char pB[1024] = { 0 };
            PACKET *pkt = reinterpret_cast<PACKET *>(pB);
            pkt->byCmdMain = ON_HEARTBEAT;
            pkt->byCmdSub = HEARTBEAT;
            //获取时间
            auto t = time(nullptr);
            memcpy(pkt->Data, &t, sizeof(t));
            pkt->uLength = sizeof(t);
            this->sendCompressData(pkt);
            //10秒一次心跳检测
            std::this_thread::sleep_for(std::chrono::seconds(60));
        }
    }).detach();

    //数据包
    char packBuf[1024] = { 0 };
    PACKET *pkt = reinterpret_cast<PACKET*>(packBuf);

    while(true)
    {
        //清零
        memset(packBuf, 0, 1024);
        //读取包12头
        if (RecvPacketHeader(m_SockClient, pkt) == false)
        {
            break;
        }
        //读取数据
        if (RecvData(m_SockClient, pkt->Data, static_cast<int>(pkt->uLength)) == false)
        {
            break;
        }

        switch (pkt->byCmdMain) {
            case ON_SCREEN:
            {//
                if (pkt->byCmdSub == START)
                {
                    int nDefin = reinterpret_cast<int *>(pkt->Data)[0];
                    m_pScreenSender->sendScreen(nDefin);
                }
                else if (pkt->byCmdSub == STOP)
                {
                    sendScreenStop();
                }
            }
            break;
            case ON_SHOT:
            {//截图
                emit m_pMain->sendScreenShot();
            }
            break;
            case ON_PROCESS:
            {//进程
                if (pkt->byCmdSub == VIEW)
                {
                    sendProcessView();
                }
                else if (pkt->byCmdSub == KILLPROCESS)
                {
                    //结束进程
                    int pId = *reinterpret_cast<int *>(pkt->Data);
                    sendKillProcessStatus(pId);
                }
            }
            break;
            case ON_FILEVIEW:
            {//文件
                if (pkt->uLength == 0)
                {
                    sendFileView(nullptr);
                }
                else
                {
                    sendFileView(pkt->Data);
                }
            }
            break;
            case ON_CMD:
            {
                if (pkt->byCmdSub == INIT)
                {
                    sendCmdInitStatus();
                }
                else if (pkt->byCmdSub == EXEC)
                {
                    m_pCmdExec->execCmd(pkt->Data);
                }
            }
            break;
            case ON_CONTROL:
            {
                //执行控制
                execControl(pkt->Data);
            }
            break;
            case ON_DOWNLOAD:
            {
                startDownload(pkt->byCmdSub, pkt->Data);
            }
            break;
        }

    }

    qDebug("客户端会话失败,错误码 : %d", WSAGetLastError());

}

void buildXml(QDomDocument &doc, QDomElement &parent, QString path)
{
    //获取下一层
    QDir dir(path);
    dir.setFilter(QDir::AllEntries);
    QFileInfoList list = dir.entryInfoList();

    //文件数
    int file_count = list.count();
    if (file_count > 0)
    {
        for (int i = 0; i < file_count; i++)
        {
            if (i > 500) break;
            //子元素
            QDomElement child;
            //获取单个文件
            QFileInfo info = list.at(i);

            //过滤..
            if (info.isDir() && (info.fileName() == "." || info.fileName() == ".."))
            {
                continue;
            }

            //判断是文件还是目录
            if (info.isDir())
            {
                //目录
                child = doc.createElement("DIR");
                child.setAttribute("name", info.fileName());
                child.setAttribute("path", info.filePath());
                child.setAttribute("createTime", info.created().toString("yyyy-MM-dd hh:mm:ss"));
                child.setAttribute("changeTime", info.lastModified().toString("yyyy-MM-dd hh:mm:ss"));
            }
            else
            {
                //是文件
                child = doc.createElement("FILE");
                child.setAttribute("name", info.fileName());
                child.setAttribute("path", info.filePath());
                child.setAttribute("createTime", info.created().toString("yyyy-MM-dd hh:mm:ss"));
                child.setAttribute("changeTime", info.lastModified().toString("yyyy-MM-dd hh:mm:ss"));
                child.setAttribute("size", info.size());
                QFileIconProvider icon_provider;
                QIcon icon = icon_provider.icon(info);
                QByteArray bytes;
                QBuffer buffer(&bytes);
                QPixmap pixmap = icon.pixmap(20, 20);
                pixmap.save(&buffer, "PNG");
                child.setAttribute("icon", QString(bytes.toBase64()));
            }
            //添加子元素
           parent.appendChild(child);
        }
    }

}

bool MyClient::sendCompressData(PACKET *packBuf, QByteArray &data)
{
    m_mutex.lock();

    //先压缩数据
    QByteArray cpData = qCompress(data);
    packBuf->uLength = static_cast<size_t>(cpData.size());

    //先发送包头
    int nRet = send(m_SockClient, reinterpret_cast<char *>(packBuf), sizeof(PACKET), 0);
    if (nRet == SOCKET_ERROR)
    {
        qDebug("send失败！错误码 : %d", WSAGetLastError());
        return false;
    }

    //发送body
    nRet = send(m_SockClient, cpData, cpData.size(), 0);
    if (nRet == SOCKET_ERROR)
    {
        qDebug("send失败！错误码 : %d", WSAGetLastError());
        return false;
    }

    //防止服务器卡包
    //sendFlushPacket();

    m_mutex.unlock();

    return true;
}

void MyClient::sendFileView(char *path)
{
    //建立XML
    QDomDocument doc;
    //写入xml头部
    QDomProcessingInstruction instruction; //添加处理命令
    //instruction=doc.createProcessingInstruction("xml","version=\"1.0\" encoding=\"UTF-8\"");
    //doc.appendChild(instruction);
    //跟节点
    QDomElement root=doc.createElement("data");
    doc.appendChild(root);
    //表示从根目录开始发
    if (path == nullptr)
    {
        DWORD dwSize = MAX_PATH;
        char szLogicalDrives[MAX_PATH] = {0};
        //获取逻辑驱动器号字符串
        DWORD dwResult = GetLogicalDriveStringsA(dwSize,szLogicalDrives);
        //处理获取到的结果
        if (dwResult > 0 && dwResult <= MAX_PATH)
        {
            char* szSingleDrive = szLogicalDrives;  //从缓冲区起始地址开始
            while(*szSingleDrive)
            {
                //创建磁盘节点
                QDomElement driver = doc.createElement("driver");
                driver.setAttribute("name", szSingleDrive);
                //根据路径构建xml
                buildXml(doc, driver, szSingleDrive);
                //添加到跟节点
                root.appendChild(driver);
                // 获取下一个驱动器号起始地址
                szSingleDrive += strlen(szSingleDrive) + 1;
            }
        }
    }
    else
    {//表示访问某目录
        //创建父节点
        QDomElement parent = doc.createElement("child");
        parent.setAttribute("path", path);
        QString name = path;
        parent.setAttribute("name", name.right(name.length() - name.lastIndexOf('/') - 1));
        //根据路径构建xml
        buildXml(doc, parent, path);
        //添加到根节点
        root.appendChild(parent);
        qDebug() << "num: " << parent.childNodes().count();
    }

    //打印xml
    QString strXml;
    QTextStream outText(&strXml);
    doc.save(outText, 2);

//    //保存文件
//    QFile file("test.xml");
//    file.open(QFile::WriteOnly|QFile::Truncate);
//    QTextStream out(&file);
//    doc.save(out, 4);
//    file.close();

    //取数据
    QByteArray data = strXml.toUtf8();
    //构建数据包
    char packBuf[120];
    PACKET *pkt = reinterpret_cast<PACKET *>(packBuf);
    pkt->byCmdMain = ON_FILEVIEW;
    pkt->byCmdSub = 0;
    pkt->uLength = static_cast<size_t>(data.size());

    qDebug() << "file: " << pkt->uLength;

    //压缩并发送数据
    sendCompressData(pkt, data);

}

bool MyClient::IsWow64(DWORD dwPid)
{
    typedef BOOL(WINAPI *LPFN_ISWOW64PROCESS) (HANDLE, PBOOL);
    LPFN_ISWOW64PROCESS fnIsWow64Process;
    //返回True则是32位进程
    //返回FALSE64位进程
    BOOL bIsWow64 = FALSE;
    HANDLE h = nullptr;

    h = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_QUERY_LIMITED_INFORMATION, FALSE, dwPid);

    fnIsWow64Process = (LPFN_ISWOW64PROCESS)GetProcAddress(
        GetModuleHandle(TEXT("kernel32")), "IsWow64Process");

    if (nullptr != fnIsWow64Process)
    {
        if (!fnIsWow64Process(h, &bIsWow64))
        {
            //handle error
        }
    }

    return bIsWow64;
}

bool MyClient::sendCompressData(PACKET *packBuf)
{
    m_mutex.lock();
    QByteArray data(packBuf->Data, static_cast<int>(packBuf->uLength));
    QByteArray cpData = qCompress(data);
    packBuf->uLength = static_cast<size_t>(cpData.size());
    memcpy(packBuf->Data, cpData, static_cast<size_t>(cpData.size()));
    //发包
    int nRet = send(m_SockClient, reinterpret_cast<char *>(packBuf), static_cast<int>(sizeof(PACKET) + packBuf->uLength), 0);
    if (nRet == SOCKET_ERROR)
    {
        qDebug("send失败！错误码 : %d", WSAGetLastError());
        return false;
    }

    //防止服务器卡包
    //sendFlushPacket();
    m_mutex.unlock();

    return true;
}

void MyClient::sendFlushPacket()
{
    //组包
    char packBuf[128];
    PACKET *pkt = reinterpret_cast<PACKET *>(packBuf);
    pkt->byCmdMain = ON_USELESS;
    pkt->byCmdSub = 0;
    pkt->uLength = 0;

    int nRet = send(m_SockClient, reinterpret_cast<char *>(packBuf), static_cast<int>(sizeof(PACKET) + pkt->uLength), 0);
    if (nRet == SOCKET_ERROR)
    {
        qDebug("send失败！错误码 : %d", WSAGetLastError());
        return;
    }
}

void MyClient::sendKillProcessStatus(int pId)
{
    //组包
    char packBuf[128];
    PACKET *pkt = reinterpret_cast<PACKET *>(packBuf);
    pkt->byCmdMain = ON_PROCESS;
    pkt->byCmdSub = KILLPROCESS;
    pkt->uLength = 8;

    //打开进程
    HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, static_cast<DWORD>(pId));

    //错误检查
    if (hProcess == nullptr)
    {
        reinterpret_cast<int *>(pkt->Data)[0] = false;
        reinterpret_cast<DWORD *>(pkt->Data)[1] = GetLastError();
    }
    else
    {
        //结束进程
        if (TerminateProcess(hProcess, 0))
        {
            reinterpret_cast<int *>(pkt->Data)[0] = true;
        }
        else
        {
            reinterpret_cast<int *>(pkt->Data)[0] = false;
            reinterpret_cast<DWORD *>(pkt->Data)[1] = GetLastError();
        }
    }

    //压缩发送
    sendCompressData(pkt);
}

void MyClient::sendCmdInitStatus()
{
    //组包
    char packBuf[256] { 0 };
    PACKET *pkt = reinterpret_cast<PACKET *>(packBuf);
    pkt->byCmdMain = ON_CMD;
    pkt->byCmdSub = INIT;
    pkt->uLength = 4;
    //初始化成功
    if (m_pCmdExec->init())
    {
        reinterpret_cast<int *>(pkt->Data)[0] = true;
    }

    //压缩发送
    sendCompressData(pkt);
}

void MyClient::sendScreenStop()
{
    //组包
    char packBuf[256] { 0 };
    PACKET *pkt = reinterpret_cast<PACKET *>(packBuf);
    pkt->byCmdMain = ON_SCREEN;
    pkt->byCmdSub = STOP;
    pkt->uLength = 0;

    //压缩发送
    sendCompressData(pkt);
}

void MyClient::sendCmdEcho(QString str)
{
    QByteArray bytes = str.toLocal8Bit();
    //组包
    char packBuf[128] { 0 };
    PACKET *pkt = reinterpret_cast<PACKET *>(packBuf);
    pkt->byCmdMain = ON_CMD;
    pkt->byCmdSub = EXEC;
    pkt->uLength = static_cast<size_t>(bytes.size());

    //压缩发送
    sendCompressData(pkt, bytes);
}

void MyClient::execControl(char *input)
{
    INPUT *pIn = reinterpret_cast<INPUT *>(input);
    SendInput(1, pIn, sizeof(INPUT));
}

void MyClient::startDownload(int nMode, char *data)
{
    u_short port = reinterpret_cast<u_short *>(data)[0];
    QString dlPath = QString::fromUtf8(&data[2]);
    //创建文件发送对象
    MyFileSender *sender = new MyFileSender(this, port, dlPath, nMode);
    sender->start();
}

void MyClient::sendProcessView()
{
    //创建进程快照
    HANDLE hProcessSnapShot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

    //进程PE信息
    PROCESSENTRY32 pPe;
    pPe.dwSize = sizeof(PROCESSENTRY32);
    //行数
    int nRow = 0;

    //遍历进程
    if (Process32First(hProcessSnapShot, &pPe) == false)
    {
        return;
    }
    else
    {
        //清空原数据
        QJsonDocument jDoc;
        QJsonObject jProcess;
        QJsonArray jList;
        do
        {
            QJsonObject obj;
            obj.insert("pId", static_cast<int>(pPe.th32ProcessID));
            obj.insert("threadNum", static_cast<int>(pPe.cntThreads));
            obj.insert("bit", !IsWow64(pPe.th32ProcessID) ? "64" : "32");
            obj.insert("parentId", static_cast<int>(pPe.th32ParentProcessID));
            obj.insert("fileName", QString::fromStdWString(pPe.szExeFile));

            //添加到列表
            jList.insert(nRow, obj);

            //行数+1
            nRow++;
        }
        while(Process32Next(hProcessSnapShot, &pPe));

        //构建json
        jProcess.insert("count", nRow);
        jProcess.insert("list", jList);
        jDoc.setObject(jProcess);

        //转换成string
        QByteArray byary = jDoc.toJson(QJsonDocument::Compact);

        //组包
        char packBuf[16];
        PACKET *pkt = reinterpret_cast<PACKET *>(packBuf);
        pkt->byCmdMain = ON_PROCESS;
        pkt->byCmdSub = VIEW;
        pkt->uLength = static_cast<size_t>(byary.size());

        //压缩发送
        sendCompressData(pkt, byary);
    }
}
