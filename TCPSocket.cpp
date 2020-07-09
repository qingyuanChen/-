#include "stdafx.h"
#include "TCPSocket.h"
#include "Simplelog.h"
#include "PacketWrapper.h"

TCPManager::TCPManager() 
{
	m_bStop = false;

	WORD wVersionRequested;
	WSADATA wsaData;
	int err;

	wVersionRequested = MAKEWORD(1, 1);
	err = WSAStartup(wVersionRequested, &wsaData);
	if (err != 0)
	{
		ASSERT(0);
		return;
	}

	if (LOBYTE(wsaData.wVersion) != 1 || HIBYTE(wsaData.wVersion) != 1){
		WSACleanup();
		return;
	}
}

TCPManager::~TCPManager()
{
	m_bStop = true;

	for (auto it : m_MapSocket){
		closesocket(it.second);
	}

	for (auto it : m_MapThread){
		it.second->join();
		delete it.second;
	}
	WSACleanup();
}

bool TCPManager::ConnectServer(CString address, int port)
{
	auto it = m_MapSocket.find(address);
	if (it != m_MapSocket.end()){
		CString strLog;
		strLog.Format("到%s的连接已经存在", address);
		CSimpleLog::Info(strLog);
		return true;
	}

	SOCKET sockconn = socket(AF_INET, SOCK_STREAM, 0);

	SOCKADDR_IN addrClt;
	addrClt.sin_addr.S_un.S_addr = inet_addr(address);
	addrClt.sin_family = AF_INET;
	addrClt.sin_port = htons(port);

	int err = ::connect(sockconn, (SOCKADDR*)&addrClt, sizeof(SOCKADDR));
	if (err != 0){
		CString strLog;
		strLog.Format("连接到服务器%s错误，错误标识码：%d", address, err);
		CSimpleLog::Info(strLog);
		return false;
	}

	CString strLog;
	strLog.Format("连接到服务器%s(%d)成功.", address, port);
	CSimpleLog::Info(strLog);

	m_MapSocket[address] = sockconn;

	LockAddress();
	m_curAddress = address;
	std::thread *pThread = new std::thread(TCPManager::ThreadProc, (DWORD_PTR)this);
	m_MapThread[address] = pThread;
	return true;
}

void TCPManager::SendData(CString address, char* data, int len)
{
	SOCKET pSocket = m_MapSocket[address];
	if (pSocket){
#ifdef TEST_TCP_LOG
		if (len != 0){
			CString strLog = "";
			for (int i = 0; i < len; i++){
				CString strTmp;
				strTmp.Format("%x ", data[i]);
				strLog += strTmp;
			}
			strLog += "\n";
			OutputDebugString(strLog.GetBuffer());
			if (len == 16){
				HJCOMPACKETHEAD tmp;
				PacketWrapper m_tmp;
				m_tmp.getPacketFromData(data, tmp);
				OutputDebugString((tmp.ToString()+"\n").GetBuffer());
			}
		}
#endif
		send(pSocket, data, len, 0);
	}
}

void TCPManager::ThreadProc(DWORD_PTR ptr)
{
	TCPManager *pThis = (TCPManager *)ptr;
	CString address = pThis->getCurAddress();
	SOCKET sockTmp = pThis->getSocket(address);
	pThis->UnlockAddress();

	while (1){
		if (pThis->IsStop()){
			break;
		}

		char dataT[1024];
		int revlen = ::recv(sockTmp, dataT, 1024, 0);
		if (revlen == -1){
			CSimpleLog::Info("close socket.");
			pThis->ReleaseSocket(address);
			break;
		}
		if (revlen != 0) pThis->OnRecvData(address, dataT, revlen);
	}
}

