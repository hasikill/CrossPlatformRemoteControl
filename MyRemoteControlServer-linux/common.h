#ifndef COMMON_H
#define COMMON_H

#define NAME_LEN 128

//cmd
enum COMMAND_MAIN
{
    ON_HEARTBEAT,
    ON_SCREEN,
    ON_FILEVIEW,
    ON_SHOT,
    ON_PROCESS,
    ON_EXEC,
    ON_CMD,
    ON_CONTROL,
    ON_DOWNLOAD,
    ON_USELESS
};

enum COMMAND_SUB
{
    HEARTBEAT,//心跳
    VIEW,
    KILLPROCESS,
    INIT,
    EXEC,
    START,
    STOP,
    DOWNLOAD_FILE,
    DOWNLOAD_DIR,
    DOWNLOAD_READY,
    DOWNLOAD_ENDFILE,
    DOWNLOAD_DATA,
    DOWNLOAD_FINISH,
    DOWNLOAD_FILECOUNT,
    DOWNLOAD_CREATE
};

#pragma pack(push, 1)
typedef struct tagPacket
{
    unsigned char byCmdMain;
    unsigned char byCmdSub;
    unsigned int uLength;
    char Data[0];
} PACKET;
#pragma pack(pop)

#ifdef _WIN64
  __MINGW_EXTENSION typedef __int64 INT_PTR,*PINT_PTR;
  __MINGW_EXTENSION typedef unsigned __int64 UINT_PTR,*PUINT_PTR;
  __MINGW_EXTENSION typedef __int64 LONG_PTR,*PLONG_PTR;
  __MINGW_EXTENSION typedef unsigned __int64 ULONG_PTR,*PULONG_PTR;
#define __int3264 __int64
#else
  typedef int INT_PTR,*PINT_PTR;
  typedef unsigned int UINT_PTR,*PUINT_PTR;
  typedef long LONG_PTR,*PLONG_PTR;
  typedef unsigned long ULONG_PTR,*PULONG_PTR;
#endif

//linux
#define INPUT_MOUSE             0
#define INPUT_KEYBOARD          0
#define MOUSEEVENTF_LEFTDOWN    0x0002
#define KEYEVENTF_KEYUP         0X0002
#define MOUSEEVENTF_RIGHTDOWN   0x0008
#define MOUSEEVENTF_LEFTUP      0x0004
#define MOUSEEVENTF_RIGHTUP     0x0010
#define MOUSEEVENTF_MOVE        0x0001
#define MOUSEEVENTF_ABSOLUTE    0x8000
#define MOUSEEVENTF_WHEEL       0x0800

typedef struct __tagMOUSEINPUT {
  long dx;
  long dy;
  unsigned int mouseData;
  unsigned int dwFlags;
  unsigned int time;
  ULONG_PTR dwExtraInfo;
} MYMOUSEINPUT;

typedef struct __tagKEYBDINPUT {
  unsigned short wVk;
  unsigned short wScan;
  unsigned int dwFlags;
  unsigned int time;
  ULONG_PTR dwExtraInfo;
} MYKEYBDINPUT;

typedef struct __tagHARDWAREINPUT {
  unsigned int uMsg;
  unsigned short wParamL;
  unsigned short wParamH;
} MYHARDWAREINPUT;

typedef struct __tagINPUT {
  unsigned int type;
  union {
    MYMOUSEINPUT mi;
    MYKEYBDINPUT ki;
    MYHARDWAREINPUT hi;
  } DUMMYUNIONNAME;
} MYINPUT;

#endif // COMMON_H
