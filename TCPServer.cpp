#include "stdafx.h"
#include "TCPServer.h"
#include "Simplelog.h"

TCPServer::TCPServer() 
	: m_bStart(false) 
	, m_CurAddress("")
{
	WORD wVersionRequested;
	WSADATA wsaData;
	int err;

	wVersionRequested = MAKEWORD(1, 1);
	err = WSAStartup(wVersionRequested, &wsaData);
	
	if (err != 0){
		CSimpleLog::Info("WSAStartup failed.");
		ASSERT(0);
		return;
	}

	if (LOBYTE(wsaData.wVersion) != 1 || HIBYTE(wsaData.wVersion) != 1){
		WSACleanup();
		ASSERT(0);
		return;
	}
}

TCPServer::~TCPServer()
{
	m_bStart = false;

	for (auto it : m_MapSocket){
		closesocket(it.second);
	}

	for (auto it : m_MapThread){
		it.second->join();
		delete it.second;
	}
	WSACleanup();
}

bool TCPServer::Start()
{
	if (m_bStart) return false;
	m_bStart = true;
	m_pAcceptThread = new std::thread(ThreadAccept, (DWORD_PTR)this);
	return true;
}

void TCPServer::SendData(CString address, char*data, int len)
{
	auto it = m_MapSocket.find(address);
	if (it != m_MapSocket.end()){
		SOCKET sendSocket = m_MapSocket[address];
		send(sendSocket, data, len, 0);
	}
}

void TCPServer::ThreadAccept(DWORD_PTR ptr)
{
	TCPServer *pThis = (TCPServer *)ptr;

	SOCKET sockSrv = socket(AF_INET, SOCK_STREAM, 0);

	SOCKADDR_IN addrSrv;
	addrSrv.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
	addrSrv.sin_family = AF_INET;
	addrSrv.sin_port = htons(20051);

	int err;

	err = ::bind(sockSrv, (SOCKADDR*)&addrSrv, sizeof(SOCKADDR)); 
	if (err == -1){
		int err = WSAGetLastError();
		CSimpleLog::Info("bind failed.");
		return;
	}

	err = ::listen(sockSrv, 5);
	if (err == -1){
		int err = WSAGetLastError();
		CSimpleLog::Info("listen failed.");
		return;
	}

	SOCKADDR_IN addrClient;
	int len = sizeof(SOCKADDR);

	while (1){
		SOCKET sockConn = ::accept(sockSrv, (SOCKADDR*)&addrClient, &len);

		if (sockConn == INVALID_SOCKET){
			int err = WSAGetLastError();
			CSimpleLog::Info("INVALID_SOCKET");
			return;
		}

		CString address = inet_ntoa(addrClient.sin_addr);

		CString strLog;
		strLog.Format("Enter new connect(%s).", address);
		CSimpleLog::Info(strLog);

		if (!pThis->IsExistSocket(address)){
			pThis->SetSocket(address, sockConn);

			pThis->LockAddress();
			pThis->SetCurAddress(address);

			std::thread* pThread = new std::thread(TCPServer::ThreadProc, (DWORD_PTR)pThis);
			pThis->SetThread(address, pThread);
		}
		else {
			strLog.Format("%s连接已经存在，关闭该连接。", address);
			CSimpleLog::Info(strLog);
			closesocket(sockConn);
		}		
	}

}

void TCPServer::ThreadProc(DWORD_PTR ptr)
{
	TCPServer * pThis = (TCPServer*)ptr;
	CString address = pThis->GetCurAddress();
	pThis->UnLockAddress();

	SOCKET dealSocket = pThis->GetSocket(address);
	if (!dealSocket){
		CString strLog;
		strLog.Format("%s GetSocket failed.", address);
		CSimpleLog::Warn(strLog);
		return;
	}

	while (1){
		if (pThis->IsStop()){
			break;
		}

		char pData[1024];
		int len = ::recv(dealSocket, pData, 1024, 0);
		if (len == -1){
			CSimpleLog::Info("close socket.");
			pThis->ReleaseSocket(address);
			break;
		}
#ifdef TEST_TCP_LOG
		if(len != 0){
			CString strLog = "";
			for (int i = 0; i< len; i++){
				CString strTmp;
				strTmp.Format("%x ", pData[i]);
				strLog += strTmp;
			}
			strLog += "\n";
			OutputDebugString(strLog.GetBuffer());
		}
#endif
		if (len != 0) pThis->OnRecvData(address, pData, len);
	}

	CString strLog;
	strLog.Format("%s dealing Thread end.", address);
	CSimpleLog::Info(strLog);
}
