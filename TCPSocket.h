#include<map>
#include <afxsock.h>
#include <afxmt.h>
#include <thread>

#ifndef TCPMANAGER_H
#define TCPMANAGER_H

class CallBackTCP{
public:
	virtual void OnRecvData(CString address, char* data, int len) = 0;
	virtual void OnConnected() = 0;
	virtual void OnClose() = 0;
};

class TCPManager{
public:
	TCPManager();
	~TCPManager();

	bool ConnectServer(CString address, int port);
	
	void SendData(CString address, char* data, int len);

	inline void OnRecvData(CString address, char* data, int len){
		if (m_bStop) return;
		if (m_Notify) m_Notify->OnRecvData(address, data, len);
	};

	inline void SetCallback(CallBackTCP* pCallback){
		m_Notify = pCallback;
	};

public:
	inline SOCKET getSocket(CString address){
		return m_MapSocket[address];
	}

	inline void ReleaseSocket(CString address){
		auto it = m_MapSocket.find(address);
		if (it != m_MapSocket.end()){
			m_MapSocket.erase(address);
		}
	}

	inline CString getCurAddress(){
		return m_curAddress;
	}

	inline void StopAll(){
		m_bStop = true;
	};
	inline bool IsStop(){
		return m_bStop;
	};

	inline void LockAddress(){
		m_csLockAddress.Lock();
	};
	inline void UnlockAddress(){
		m_csLockAddress.Unlock();
	};

private:
	static void ThreadProc(DWORD_PTR);

	CallBackTCP* m_Notify;
	CCriticalSection m_csLockAddress;
	CString m_curAddress;
	std::map<CString, SOCKET> m_MapSocket;
	std::map<CString, std::thread*> m_MapThread;
	BOOL m_bStop;
};
#endif