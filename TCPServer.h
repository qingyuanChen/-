#include <map>
#include <afxsock.h>
#include <afxmt.h>
#include <thread>

#ifndef TCPSERVER_H
#define TCPSERVER_H

class CallBackTCPServer{
public:
	virtual void OnRecvData(CString address, char* data, int len) = 0;
	virtual void OnConnected() = 0;
	virtual void OnServerClose() = 0;
};

class TCPServer{
public:
	TCPServer();
	~TCPServer();

	bool Start();
	void SendData(CString address, char*data, int len);

	inline void OnRecvData(CString address, char* data, int len){
		if (!m_bStart) return;
		if (m_pNotify) m_pNotify->OnRecvData(address, data, len);
	};

	inline void SetCallback(CallBackTCPServer* pCallback){
		m_pNotify = pCallback;
	};

public:
	inline void SetSocket(CString address, SOCKET in){
		auto it = m_MapSocket.find(address);
		if (it == m_MapSocket.end()){
			m_MapSocket[address] = in;
		}
	};

	inline SOCKET GetSocket(CString address){
		auto it = m_MapSocket.find(address);
		if (it != m_MapSocket.end()){
			return m_MapSocket[address];
		}
		return NULL;
	}

	inline bool IsExistSocket(CString address){
		auto it = m_MapSocket.find(address);
		if (it != m_MapSocket.end()) return true;
		else return false;
	}

	inline void ReleaseSocket(CString address){
		auto it = m_MapSocket.find(address);
		if (it != m_MapSocket.end()){
			m_MapSocket.erase(address);
		}
	}

	inline void SetThread(CString address, std::thread *ptr){
		auto it = m_MapThread.find(address);
		if (it == m_MapThread.end()){
			m_MapThread[address] = ptr;
		}
	};

	inline void SetCurAddress(CString address){
		m_CurAddress = address;
	};

	inline CString GetCurAddress(){
		return m_CurAddress;
	};

	inline void LockAddress(){
		m_csLockAddress.Lock();
	};

	inline void UnLockAddress(){
		m_csLockAddress.Unlock();
	};

	inline bool IsStop(){
		return !m_bStart;
	}

	

private:
	static void ThreadAccept(DWORD_PTR);
	static void ThreadProc(DWORD_PTR);

	std::thread *m_pAcceptThread;
	CallBackTCPServer* m_pNotify;
	std::map<CString, SOCKET> m_MapSocket;
	std::map<CString, std::thread*> m_MapThread;
	CCriticalSection m_csLockAddress;
	CString m_CurAddress;
	bool m_bStart;
};

#endif