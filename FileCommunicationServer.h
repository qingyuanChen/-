#include <afxstr.h>
#include "FileWrapper.h"
#include "PacketWrapper.h"
#include "FileBufferManager.h"
#include "TCPServer.h"

typedef struct{
	int headLen;
	char *head;
	int dataLen;
	char *data;
}HJCOMPACKET;

class FileCommunicationServer : public CallBackTCPServer{
public:
	FileCommunicationServer();
	~FileCommunicationServer();

	void SendFile(CString serverIp, CString fileName);
	void SendDirectory(CString serverIp, CString fileName);
	//void SendDownload(CString serverIp, CString fileName, int type);

	void ParseDataToFile(CString strAddress, char*src, int len);
	void ParseDataToDirectory(CString strAddress, char*src, int len);

	inline bool IsStop(){
		return m_bStop;
	};

public:
	virtual void OnRecvData(CString serverIp, char* data, int len);
	virtual void OnConnected();
	virtual void OnServerClose();

public:
	std::map<CString, FileBufferManager*> m_mapFileBuffer;

private:
	CString GetClassName();
	static void ThreadProc(DWORD_PTR);

private:
	TCPServer m_tcpServer;
	FileWrapper m_fileWrapper;
	PacketWrapper m_packetWrapper;

	std::map<CString, bool> m_mapConnenct;
	std::thread* m_pThread;

	bool m_bStop;
};