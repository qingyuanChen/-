#include <queue>
#include <afxmt.h>

typedef struct{
	char *data;
	int readLen;
	int size;
}BufferNode;

class readDataBuffer{
public:
	readDataBuffer();
	~readDataBuffer();

	void ReadData(char *dst, int len);
	void PushData(char *src, int len);
	inline bool CheckLen(int len){
		return len > m_iBufferSize ? false : true;
	};

private:
	void readData(char* dst, int len);
	void pushData(char*src, int len);
	void clearQueue(std::queue<BufferNode>&);
	
	inline void LockQueue(){
		m_queueLock.Lock();
	};
	inline void UnLockQueue(){
		m_queueLock.Unlock();
	};
	inline int maxTwo(int v1, int v2){
		return v1 < v2 ? v2 : v1;
	};
	inline CString GetClassName(){
		return "readDataBuffer";
	};

	std::queue<BufferNode> m_queueBuffer; 
	CCriticalSection m_queueLock;
	BufferNode m_curReadNode;
	BufferNode m_curWriteNode;
	CCriticalSection m_curWriteNodeLock;
	int m_iBufferSize;
	int m_iWriteLen;
};