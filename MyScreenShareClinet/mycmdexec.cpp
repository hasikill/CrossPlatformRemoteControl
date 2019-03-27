#include "mycmdexec.h"
#include <windows.h>
#include "myclient.h"

struct tagExitParam
{
    HANDLE hCmd;
    MyCmdExec *obj;
};

DWORD ExitProc(LPVOID lpThreadParameter)
{
    tagExitParam *param = reinterpret_cast<tagExitParam *>(lpThreadParameter);
    WaitForSingleObject(param->hCmd, INFINITE);
    param->obj->m_bInit = false;
    param->obj->m_bStart = false;
    delete param;
}

MyCmdExec::MyCmdExec(MyClient *pClient) : m_pClient(pClient)
{
    m_bInit = false;
    m_bStart = false;
}

bool MyCmdExec::init()
{
    if (m_bInit)
    {
        return true;
    }

    hMyRead = nullptr;
    hMyWrite = nullptr;

    hCmdRead = nullptr;
    hCmdWrite = nullptr;

    SECURITY_ATTRIBUTES sa;
    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    //我进程的句柄是否可以被子进程继承
    sa.bInheritHandle = true;
    sa.lpSecurityDescriptor = nullptr;

    //创建管道
    BOOL bRet = CreatePipe(&hMyRead, &hCmdWrite, &sa, 0);
    if (!bRet)
    {
        return false;
    }

    bRet = CreatePipe(&hCmdRead, &hMyWrite, &sa, 0);
    if (!bRet)
    {
        return false;
    }

    char szBuf[256] = "cmd.exe";

    STARTUPINFOA si;
    memset(&si, 0, sizeof(STARTUPINFOA));
    si.cb = sizeof(STARTUPINFOA);

    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdInput = hCmdRead;
    si.hStdOutput = hCmdWrite;
    si.hStdError = hCmdWrite;

    memset(&pi, 0, sizeof(PROCESS_INFORMATION));

    bRet = CreateProcessA(
                nullptr,
                szBuf,
                nullptr,
                nullptr,
                TRUE,
                CREATE_NO_WINDOW,
                nullptr,
                nullptr,
                &si,
                &pi
                );
    if (!bRet)
    {
        return false;
    }

    tagExitParam *param = new tagExitParam;
    param->hCmd = pi.hProcess;
    param->obj = this;
    //创建监视线程
    CreateThread(nullptr, 0, (LPTHREAD_START_ROUTINE)ExitProc, param, 0, nullptr);

    m_bInit = true;
    start();

    return true;
}

void MyCmdExec::run()
{
    m_bStart = true;
    char *szBuf = nullptr;
    BOOL bRet;

    szBuf = new char[1024 * 1024 * 5] { 0 };

    while (m_bStart)
    {
        DWORD dwReadedBytes = 0;
        memset(szBuf, 0, 1024 * 1024 * 5);

        //读取数据
        bRet = ReadFile(
                    hMyRead,
                    szBuf,
                    1024 * 1024 * 5,
                    &dwReadedBytes,
                    nullptr
                    );

        if (bRet)
        {
            //编码转换
            QString strText(QString::fromLocal8Bit(szBuf));
            //发送数据
            m_pClient->sendCmdEcho(strText);
            //
            Sleep(100);
        }

    }

    if (szBuf != nullptr)
    {
        delete szBuf;
        szBuf = nullptr;
    }

    m_bStart = false;
    m_bInit = false;
}

void MyCmdExec::execCmd(char *cmd)
{
    char buf[81] = { 0 };

    strcpy(buf, cmd);
    strcat_s(buf, "\n");

    DWORD dwWritedBytes = 0;
    WriteFile(
                hMyWrite,
                buf,
                strlen(buf),
                &dwWritedBytes,
                nullptr
                );
}
