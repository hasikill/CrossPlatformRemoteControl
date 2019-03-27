#include "common.h"

bool RecvData(SOCKET s, char *pBuf, int nData)
{
    int nResidueSize = nData;
    int nRecvLen = 0;
    while(nResidueSize != 0)
    {
        nRecvLen = recv(s, pBuf + (nData - nResidueSize), nResidueSize, 0);
        if (nRecvLen > 0)
        {
            nResidueSize -= nRecvLen;
        }
        else
        {
            return false;
        }

    }
    return true;
}

bool RecvPacketHeader(SOCKET s, PACKET *pack)
{
    return RecvData(s, reinterpret_cast<char *>(pack), sizeof(PACKET));
}
