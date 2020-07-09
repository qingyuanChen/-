#include "stdafx.h"
#include "FileBufferManager.h"

FileBufferManager::FileBufferManager() : m_bHaveHead(false)
{
	memset(&m_head, 0, sizeof(HJCOMPACKETHEAD));
}

FileBufferManager::~FileBufferManager()
{

}

bool FileBufferManager::CheckAndGetPacketHead()
{
	if (m_bHaveHead) return true;
	int len = sizeof(HJCOMPACKETHEAD);
	bool ret = m_readDataBuffer.CheckLen(len);
	if (ret){
		memset(&m_head, 0, len);
		m_readDataBuffer.ReadData((char*)&m_head, len);
		m_bHaveHead = true;
		return true;
	}
	else {
		return false;
	}
}

bool FileBufferManager::CheckPacket(int len)
{
	bool ret = m_readDataBuffer.CheckLen(len);
	if (ret){
		m_bHaveHead = false;
	}
	return ret;
}

void FileBufferManager::ReadData(char *dst, int len)
{
	m_readDataBuffer.ReadData(dst, len);
}

void FileBufferManager::PushData(char *src, int len)
{
	m_readDataBuffer.PushData(src, len);
}
