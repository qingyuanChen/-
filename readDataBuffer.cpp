#include "stdafx.h"
#include "readDataBuffer.h"
#include "Simplelog.h"

readDataBuffer::readDataBuffer() 
	: m_iBufferSize(0)
	, m_iWriteLen(0)
{
	clearQueue(m_queueBuffer);
	memset(&m_curReadNode, 0, sizeof(BufferNode));
	memset(&m_curWriteNode, 0, sizeof(BufferNode));
}

readDataBuffer::~readDataBuffer()
{
	while (!m_queueBuffer.empty()){
		BufferNode curNode = m_queueBuffer.front();
		m_queueBuffer.pop();
		delete curNode.data;
	}
}

void readDataBuffer::ReadData(char *dst, int len)
{
	if (len == 0 || m_iBufferSize == 0) return;
	if (len > m_iBufferSize) return;
	if (len > m_iBufferSize - m_iWriteLen || (len < m_iBufferSize && m_queueBuffer.size() == 0)){
		if (m_iWriteLen != 0){
			m_curWriteNodeLock.Lock();
			m_curWriteNode.size = m_iWriteLen;
			LockQueue();
			m_queueBuffer.push(m_curWriteNode);
			UnLockQueue();
			memset(&m_curWriteNode, 0, sizeof(BufferNode));
			m_curWriteNodeLock.Unlock();
			m_iWriteLen = 0;
		}		
	}
	readData(dst, len);
}

void readDataBuffer::PushData(char *src, int len)
{
	pushData(src, len);
}

void readDataBuffer::readData(char* dst, int len)
{
	if (len == 0 || m_iBufferSize == 0) return;
	if (len > m_iBufferSize) return;
	if (m_curReadNode.size == 0){
		LockQueue();
		m_curReadNode = m_queueBuffer.front();
		m_queueBuffer.pop();
		UnLockQueue();
		if (m_curReadNode.size == 0) {
			CString strLog;
			strLog.Format("%s:m_queueBuffer.front() made a serious mistake.", GetClassName());
			CSimpleLog::Error(strLog);
			ASSERT(0);
			return ;
		}
	}
	char *cur = dst;
	int readLen = 0, remainLen = 0;
	if (len + m_curReadNode.readLen > m_curReadNode.size){
		readLen = m_curReadNode.size - m_curReadNode.readLen;
		remainLen = len - readLen;
	}
	else{
		readLen = len;
		remainLen = 0;
	}
	memcpy(cur, m_curReadNode.data + m_curReadNode.readLen, readLen);
#ifdef TEST_TCP_LOG
	if (len != 0){
		CString strLog = "readData:";
		for (int i = 0; i < readLen; i++){
			CString strTmp;
			strTmp.Format("%x ", (m_curReadNode.data + m_curReadNode.readLen)[i]);
			strLog += strTmp;
		}
		strLog += "\n";
		OutputDebugString(strLog.GetBuffer());
	}
#endif
	m_iBufferSize -= readLen;
	cur += readLen;
	m_curReadNode.readLen += readLen;
	if (m_curReadNode.readLen == m_curReadNode.size){
		delete m_curReadNode.data;
		memset(&m_curReadNode, 0, sizeof(BufferNode));
	}
	if (remainLen != 0) readData(cur, remainLen);
}

void readDataBuffer::pushData(char*src, int len)
{
	if (len <= 0)return;
	m_curWriteNodeLock.Lock();
	if (m_curWriteNode.size == 0){
		m_curWriteNode.size = maxTwo(10 * 1024, len);
		m_curWriteNode.data = new char[m_curWriteNode.size];
		m_curWriteNode.readLen = 0;
		m_iWriteLen = 0;
	}
	int curWriteLen = 0, remainLen = 0;
	if (len + m_iWriteLen > m_curWriteNode.size){
		curWriteLen = m_curWriteNode.size - m_iWriteLen;
		remainLen = len - curWriteLen;
	}
	else{
		curWriteLen = len;
		remainLen = 0;
	}
	memcpy(m_curWriteNode.data + m_iWriteLen, src, curWriteLen);
#ifdef TEST_TCP_LOG
	if (len != 0){
		CString strLog = "pushData:";
		for (int i = 0; i < curWriteLen; i++){
			CString strTmp;
			strTmp.Format("%x ", (m_curWriteNode.data + m_iWriteLen)[i]);
			strLog += strTmp;
		}
		strLog += "\n";
		OutputDebugString(strLog.GetBuffer());
	}
#endif
	m_iBufferSize += curWriteLen;
	m_iWriteLen += curWriteLen;
	if (m_iWriteLen == m_curWriteNode.size){
		LockQueue();
		m_queueBuffer.push(m_curWriteNode);
		UnLockQueue();
		memset(&m_curWriteNode, 0, sizeof(m_curWriteNode));
		m_iWriteLen = 0;
	}
	m_curWriteNodeLock.Unlock();
	if (remainLen != 0) pushData(src + curWriteLen, remainLen);
}

void readDataBuffer::clearQueue(std::queue<BufferNode>& sQueue)
{
	std::queue<BufferNode> empty;
	swap(empty, sQueue);
}
