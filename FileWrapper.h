#include <map>
#include <afxmt.h>

typedef enum{
	FILEWRAPPER_FILE = 0,
	FILEWRAPPER_DIRECTORY
}FILEWRAPPER_TYPE;

typedef struct{
	byte typeId;  // 0 is file. 1 is directory.
	byte NameLen;
	int dataLen;

	static int GetSize(){
		int iSize = sizeof(byte) + sizeof(byte) + sizeof(int);
		return iSize;
	}
}FileAbout;

class FileWrapper{
public:







	FileWrapper();
	~FileWrapper();

	void GenerateFileData(char * filename, void **dst, int* len);
	void GenerateDirectoryData(char* directoryName, void **dst, int*len);
	void GenerateDownloadData(char *DownloadUrl, void **dst, int *len);
	void ParseDataToFile(CString strIp, void* src, int len);
	void ParseDataToDirectory(CString strIp, void*src, int len);

	inline void SetServer(){
		m_bIsServer = true;
	};

private:
	void SetFileData(char*filename, void *dst, int*len);
	void SetDirectoryData(char* directoryName, void*dst, int*len);
	int FileLoopProcess(CString strFolder, void *dst);
	int GetFolderSize(CString strFolder);
	void FileLoopProcessGetSize(CString strFolder);
	void CheckAndCreateDir(CString strFolder);
	CString GetDir(CString fileName);
	CString GetAppDirectory();
	CString ConvertServerPath(CString fileName);
	
	std::map<CString, int> m_mapFileSize;
	CCriticalSection m_mapLock;
	int  m_iFolderSize;
	bool m_bIsServer;
};