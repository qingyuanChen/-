#include "readDataBuffer.h"
#include "PacketWrapper.h"

class FileBufferManager{
public:
	FileBufferManager();
	~FileBufferManager();

	bool CheckAndGetPacketHead();
	inline HJCOMPACKETHEAD GetHead(){
		return m_head;
	};

	bool CheckPacket(int len);
	void ReadData(char *dst, int len);
	void PushData(char *src, int len);

private:
	readDataBuffer m_readDataBuffer;
	HJCOMPACKETHEAD m_head;	
	bool m_bHaveHead;
};