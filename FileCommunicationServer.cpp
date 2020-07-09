#include "stdafx.h"
#include "FileCommunicationServer.h"
#include "SimpleLog.h"

FileCommunicationServer::FileCommunicationServer()
	: m_bStop(false)
	, m_pThread(NULL)
{
	m_tcpServer.SetCallback(this);
	m_tcpServer.Start();
	m_fileWrapper.SetServer();
	m_mapFileBuffer.clear();
	m_mapConnenct.clear();


	m_pThread = new std::thread(FileCommunicationServer::ThreadProc, (DWORD_PTR)this);
	if (!m_pThread){
		CString strLog;
		strLog.Format("%s:Creating thread failed.", GetClassName());
		CSimpleLog::Error(strLog);
		ASSERT(0);
	}
}

FileCommunicationServer::~FileCommunicationServer()
{

}

void FileCommunicationServer::SendFile(CString serverIp, CString fileName)
{
	if (fileName == "") return;
	if (!m_tcpServer.IsExistSocket(serverIp))  return;
	HJCOMPACKET sendPackect;
	m_fileWrapper.GenerateFileData(fileName.GetBuffer(), (void**)&sendPackect.data, &sendPackect.dataLen);
	m_packetWrapper.getPacketHead(HJCOMFILE, sendPackect.dataLen, (void**)&sendPackect.head, &sendPackect.headLen);

	m_tcpServer.SendData(serverIp, sendPackect.head, sendPackect.headLen);
	m_tcpServer.SendData(serverIp, sendPackect.data, sendPackect.dataLen);
}

void FileCommunicationServer::SendDirectory(CString serverIp, CString fileName)
{
	if (fileName == "") return;
	if (!m_tcpServer.IsExistSocket(serverIp))  return;
	HJCOMPACKET sendPackect;
	m_fileWrapper.GenerateDirectoryData(fileName.GetBuffer(), (void**)&sendPackect.data, &sendPackect.dataLen);
	m_packetWrapper.getPacketHead(HJCOMDIRECTORY, sendPackect.dataLen, (void**)&sendPackect.head, &sendPackect.headLen);

	m_tcpServer.SendData(serverIp, sendPackect.head, sendPackect.headLen);
	m_tcpServer.SendData(serverIp, sendPackect.data, sendPackect.dataLen);
}

void FileCommunicationServer::ParseDataToFile(CString strAddress, char*src, int len)
{
	m_fileWrapper.ParseDataToFile(strAddress, src, len);
}

void FileCommunicationServer::ParseDataToDirectory(CString strAddress, char*src, int len)
{
	m_fileWrapper.ParseDataToDirectory(strAddress, src, len);
}

void FileCommunicationServer::OnRecvData(CString serverIp, char* data, int len)
{
	auto it = m_mapFileBuffer.find(serverIp);
	if (it == m_mapFileBuffer.end()){
		m_mapFileBuffer[serverIp] = new FileBufferManager;
	}
	m_mapFileBuffer[serverIp]->PushData(data, len);
}

void FileCommunicationServer::OnConnected()
{

}

void FileCommunicationServer::OnServerClose()
{

}

CString FileCommunicationServer::GetClassName()
{
	return "FileCommunicationServer";
}

void FileCommunicationServer::ThreadProc(DWORD_PTR ptr)
{
	FileCommunicationServer *pThis = (FileCommunicationServer*)ptr;
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
					else if (head.p_type == HJCOMGETFILE){
						int iLen = 0;
						memcpy(&iLen, data, sizeof(int));
						if (iLen > 0 && iLen < 256){
							char *fileName = new char[iLen + 1];
							fileName[iLen] = '\0';
							memcpy(fileName, data + 4, iLen);
							pThis->SendFile(it.first, fileName);
						}
						else{
							CString strLog;
							strLog.Format("%s:package GetFile analyse made a serious mistake.");
							CSimpleLog::Error(strLog);
							continue;
						}
					}
					else if (head.p_type == HJCOMGETDIRECTORY){
						int iLen = 0;
						memcpy(&iLen, data, sizeof(int));
						if (iLen > 0 && iLen < 256){
							char *fileName = new char[iLen + 1];
							fileName[iLen] = '\0';
							memcpy(fileName, data + 4, iLen);
							pThis->SendDirectory(it.first, fileName);
						}
						else{
							CString strLog;
							strLog.Format("%s:package GetDirectory analyse made a serious mistake.");
							CSimpleLog::Error(strLog);
							continue;
						}
					}
					else{
						CString strLog;
						strLog.Format("%s:Unprocessed package found.");
						CSimpleLog::Error(strLog);
						continue;
					}
					
				}
			}
		}

		Sleep(10);
	}
}
