#ifndef MYCMDEXEC_H
#define MYCMDEXEC_H

#include <QThread>
#include <windows.h>

DWORD ExitProc(LPVOID lpThreadParameter);
class MyClient;
class MyCmdExec : public QThread
{
public:
    MyCmdExec(MyClient *pClient);

    bool init();
    virtual void run();
    void execCmd(char* cmd);

    MyClient *m_pClient;
    bool m_bInit;
    bool m_bStart;

    //管道1 My  ------->  Cmd
    HANDLE hMyWrite;
    HANDLE hCmdRead;


    //管道2 My <-------  Cmd
    HANDLE hMyRead;
    HANDLE hCmdWrite;

    PROCESS_INFORMATION pi;

    friend class MyClient;
};

#endif // MYCMDEXEC_H
