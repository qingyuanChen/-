#include "stdafx.h"
#include "PacketWrapper.h"
#include <cstring>

void PacketWrapper::getPacketHead(HJCOMPACKETTYPE tType, int datalen, void **dst, int *len)
{
	static int num = 0;
	char *pDst = new char[sizeof(HJCOMPACKETHEAD)];
	HJCOMPACKETHEAD tmp;
	tmp.id = num;
	tmp.p_type = tType;
	tmp.datalen = datalen;
	tmp.subpackageNum = 0;
	tmp.subpackageId = 0;
	memcpy(pDst, &tmp, sizeof(tmp));
	*dst = pDst;
	*len = sizeof(tmp);
	num++;
}

int PacketWrapper::getPacketFromData(void *src, HJCOMPACKETHEAD& dst)
{
	if (!src) return -1;
	HJCOMPACKETHEAD tmp;
	memcpy(&tmp, src, sizeof(tmp));
	dst = tmp;
	return 0;
}
