#include "stdafx.h"
#include "FileCommunicationClient.h"
#include "Simplelog.h"

FileCommunicationClient::FileCommunicationClient()
	: m_bStop(false)
	, m_pThread(NULL)
{
	m_tcpManager.SetCallback(this);
	m_mapFileBuffer.clear();
	m_mapConnenct.clear();

	m_pThread = new std::thread(FileCommunicationClient::ThreadProc, (DWORD_PTR)this);
	if (!m_pThread){
		CString strLog;
		strLog.Format("%s:Creating thread failed.", GetClassName());
		CSimpleLog::Error(strLog);
		ASSERT(0);
	}
}

FileCommunicationClient::~FileCommunicationClient()
{
	m_bStop = true;
	if (m_pThread){
		m_pThread->join();
		delete m_pThread;
		m_pThread = NULL;
	}

	for (auto it : m_mapFileBuffer){
		delete it.second;
	}

	m_mapFileBuffer.clear();
	m_mapConnenct.clear();
}

bool FileCommunicationClient::ConnectServer(CString serverIp, int port)
{
	bool ret = m_tcpManager.ConnectServer(serverIp, port);
	if (!ret){
		CString strLog;
		strLog.Format("%s:ConnectServer failed.", GetClassName());
		CSimpleLog::Warn(strLog);
		m_mapConnenct[serverIp] = false;
	}
	else{
		m_mapConnenct[serverIp] = true;
	}
	return ret;
}

void FileCommunicationClient::SendFile(CString serverIp, CString fileName)
{
	if (fileName == "") return;
	auto it = m_mapConnenct.find(serverIp);
	if (it == m_mapConnenct.end() || !m_mapConnenct[serverIp]) return;

	HJCOMPACKET sendPackect;
	m_fileWrapper.GenerateFileData(fileName.GetBuffer(), (void**)&sendPackect.data, &sendPackect.dataLen);
	m_packetWrapper.getPacketHead(HJCOMFILE, sendPackect.dataLen, (void**)&sendPackect.head, &sendPackect.headLen);

	m_tcpManager.SendData(serverIp, sendPackect.head, sendPackect.headLen);
	m_tcpManager.SendData(serverIp, sendPackect.data, sendPackect.dataLen);
}

void FileCommunicationClient::SendDirectory(CString serverIp, CString fileName)
{
	if (fileName == "") return;
	auto it = m_mapConnenct.find(serverIp);
	if (it == m_mapConnenct.end() || !m_mapConnenct[serverIp]) return;
	HJCOMPACKET sendPackect;
	m_fileWrapper.GenerateDirectoryData(fileName.GetBuffer(), (void**)&sendPackect.data, &sendPackect.dataLen);
	m_packetWrapper.getPacketHead(HJCOMDIRECTORY, sendPackect.dataLen, (void**)&sendPackect.head, &sendPackect.headLen);

	m_tcpManager.SendData(serverIp, sendPackect.head, sendPackect.headLen);
	m_tcpManager.SendData(serverIp, sendPackect.data, sendPackect.dataLen);
}

void FileCommunicationClient::SendDownload(CString serverIp, CString fileName, int type) // type:0 is file. 1 is directory.
{
	if (fileName == "") return;
	auto it = m_mapConnenct.find(serverIp);
	if (it == m_mapConnenct.end() || !m_mapConnenct[serverIp]) return;
	HJCOMPACKET sendPackect;
	int pType = (type == 0 ? HJCOMGETFILE : HJCOMGETDIRECTORY);
	m_fileWrapper.GenerateDownloadData(fileName.GetBuffer(), (void**)&sendPackect.data, &sendPackect.dataLen);
	m_packetWrapper.getPacketHead((HJCOMPACKETTYPE)pType, sendPackect.dataLen, (void**)&sendPackect.head, &sendPackect.headLen);

	m_tcpManager.SendData(serverIp, sendPackect.head, sendPackect.headLen);
	m_tcpManager.SendData(serverIp, sendPackect.data, sendPackect.dataLen);
}

void FileCommunicationClient::ParseDataToFile(CString strAddress, char*src, int len)
{
	m_fileWrapper.ParseDataToFile(strAddress, src, len);
}

void FileCommunicationClient::ParseDataToDirectory(CString strAddress, char*src, int len)
{
	m_fileWrapper.ParseDataToDirectory(strAddress, src, len);
}

void FileCommunicationClient::OnRecvData(CString serverIp, char* data, int len)
{
	auto it = m_mapFileBuffer.find(serverIp);
	if (it == m_mapFileBuffer.end()){
		m_mapFileBuffer[serverIp] = new FileBufferManager;
	}
	m_mapFileBuffer[serverIp]->PushData(data, len);
}

void FileCommunicationClient::OnConnected()
{

}

void FileCommunicationClient::OnClose()
{

}

CString FileCommunicationClient::GetClassName()
{
	return "FileCommunicationClient";
}

void FileCommunicationClient::ThreadProc(DWORD_PTR ptr)
{
	FileCommunicationClient *pThis = (FileCommunicationClient*)ptr;
	while (1){
		if (pThis->IsStop()) break;
		for (auto it : pThis->m_mapFileBuffer){
			bool bRet = it.second->CheckAndGetPacketHead();
			if (bRet){
				HJCOMPACKETHEAD head = it.second->GetHead();
				bRet = it.second->CheckPacket(head.datalen);
				if (bRet){
					char *data = new char[head.datalen];
					it.second->ReadData(data, head.datalen);
					if (head.p_type == HJCOMFILE){
						pThis->ParseDataToFile(it.first, data, head.datalen);
					}
					else if (head.p_type == HJCOMDIRECTORY){
						pThis->ParseDataToDirectory(it.first, data, head.datalen);
					}
					else{
						CString strLog;
						strLog.Format("%s:Unprocessed package found.");
						CSimpleLog::Error(strLog);
					}
				}
			}
		}

		Sleep(10);
	}
}
