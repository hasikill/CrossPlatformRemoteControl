#ifndef COMMON_H
#define COMMON_H
#include <winsock2.h>

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


bool RecvData(SOCKET s, char *pBuf, int nData);

bool RecvPacketHeader(SOCKET s, PACKET *pack);
#endif // COMMON_H
