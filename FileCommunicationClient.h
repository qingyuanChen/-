#include <afxstr.h>
#include "TCPSocket.h"
#include "PacketWrapper.h"
#include "FileWrapper.h"
#include "FileBufferManager.h"
#include <thread>

typedef struct{
	int headLen;
	char *head;
	int dataLen;
	char *data;
}HJCOMPACKET;

class FileCommunicationClient : public CallBackTCP{
public:
	FileCommunicationClient();
	~FileCommunicationClient();

	static FileCommunicationClient& Instance(){
		static FileCommunicationClient obj;
		return obj;
	}
	
	bool ConnectServer(CString serverIp, int port);
	void SendFile(CString serverIp, CString fileName);
	void SendDirectory(CString serverIp, CString fileName);
	void SendDownload(CString serverIp, CString fileName, int type);

	void ParseDataToFile(CString strAddress, char*src, int len);
	void ParseDataToDirectory(CString strAddress, char*src, int len);

	inline bool IsStop(){
		return m_bStop;
	};


public:
	virtual void OnRecvData(CString serverIp, char* data, int len);
	virtual void OnConnected();
	virtual void OnClose();

public:
	std::map<CString, FileBufferManager*> m_mapFileBuffer;

private:
	CString GetClassName();
	static void ThreadProc(DWORD_PTR);

private:
	TCPManager m_tcpManager;
	FileWrapper m_fileWrapper;
	PacketWrapper m_packetWrapper;

	std::map<CString, bool> m_mapConnenct;
	std::thread* m_pThread;

	bool m_bStop;
};