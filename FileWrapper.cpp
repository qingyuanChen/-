#include "stdafx.h"
#include "FileWrapper.h"
#include "string.h"
#include <stdio.h>
#include <io.h>
#include <fstream>
#include <sstream>
#include "Simplelog.h"
#include <direct.h>

static int GetFileLength(const char* pFileName){
	/*FILE * file = fopen(pFileName, "rb");
	if (file){
	int size = filelength(fileno(file));
	fclose(file);
	return size;
	}*/
	WIN32_FIND_DATA fileInfo;
	HANDLE hFind;
	DWORD fileSize = 0;
	hFind = FindFirstFile(pFileName, &fileInfo);
	if (hFind != INVALID_HANDLE_VALUE)
		fileSize = fileInfo.nFileSizeLow;
	FindClose(hFind);

	//FILE*fp;
	//fp = fopen(pFileName, "rb");// localfile文件名 
	//fseek(fp, 0L, SEEK_END); /* 定位到文件末尾 */
	//
	//int flen = ftell(fp);
	//fclose(fp);

	/*ifstream inFile(pFileName);
	string contents("");
	if (inFile.is_open()){
		std::stringstream buffer;
		buffer << inFile.rdbuf();
		contents.append(buffer.str());
	}

	inFile.close();
	int fileSize = contents.length();*/

	return fileSize;
}

static void ReadFile(const char*pFileName, char* pFileContent, int len){
	/*ifstream inFile(pFileName);
	string contents("");
	if (inFile.is_open()){
	std::stringstream buffer;
	buffer << inFile.rdbuf();
	contents.append(buffer.str());
	}

	inFile.close();
	int len1 = contents.length();
	strcpy(pFileContent, contents.c_str());*/

	FILE *fp;
	fp = fopen(pFileName, "rb");
	if (!fp) return;
	fseek(fp, 0L, SEEK_SET);
	fread(pFileContent, len, 1, fp);
	fclose(fp);
}

FileWrapper::FileWrapper()
{
	m_bIsServer = false;
	m_iFolderSize = 0;
	m_mapFileSize.clear();
}

FileWrapper::~FileWrapper()
{

}

void FileWrapper::GenerateFileData(char * filename, void **dst, int* len)
{
	FileAbout info;
	info.typeId = FILEWRAPPER_FILE;
	info.NameLen = strlen(filename);
	info.dataLen = GetFileLength(filename);
	m_mapLock.Lock();
	m_mapFileSize[filename] = info.dataLen;
	m_mapLock.Unlock();
	int fileSize = FileAbout::GetSize() + info.NameLen + info.dataLen;
	if (fileSize < 0 || fileSize > 350 * 1024 * 1024){
		CSimpleLog::Error("文件数据大小超过350M");
		*dst = NULL;
		*len = 0;
		return;
	}
	*dst = new char[fileSize];
	SetFileData(filename, *dst, len);
}

void FileWrapper::GenerateDirectoryData(char* directoryName, void **dst, int*len)
{
	GetFolderSize(directoryName);
	if (m_iFolderSize < 0 || m_iFolderSize > 350 * 1024 * 1024){
		CSimpleLog::Error("目录数据大小超过350M");
		//CSimpleLog::Error("目录数据生成错误.");
		*dst = NULL;
		*len = 0;
		return;
	}
	*dst = new char[m_iFolderSize];
	*len = m_iFolderSize;
	FileLoopProcess(directoryName, *dst);
}

void FileWrapper::GenerateDownloadData(char *DownloadUrl, void **dst, int *len)
{
	int slen = strlen(DownloadUrl);
	*len = sizeof(int) + slen;
	char *tmp = new char[sizeof(int) + slen];
	*dst = tmp;
	memcpy(tmp, &slen, sizeof(int));
	tmp += sizeof(int);
	memcpy(tmp, DownloadUrl, slen);
}

void FileWrapper::ParseDataToFile(CString strIp, void* src, int len)
{
	char * cur = (char*)src;
	FileAbout info;
	memcpy(&info.typeId, cur, 1);
	cur++;
	memcpy(&info.NameLen, cur, 1);
	cur++;
	char*fileName = new char[info.NameLen + 1];
	memcpy(fileName, cur, info.NameLen);
	fileName[info.NameLen] = '\0';
	cur += info.NameLen;


	CString FinalFileName = fileName;

	if (!m_bIsServer){
		CString strFileName(fileName);
		strFileName.Replace(":", "");
		FinalFileName.Format("%sRemoteFile\\%s\\%s", GetAppDirectory(), strIp, strFileName);
	}
	else{
		FinalFileName = ConvertServerPath(fileName);
	}

	CString strDirectory = GetDir(FinalFileName);
	CheckAndCreateDir(strDirectory);

	memcpy(&info.dataLen, cur, 4);
	cur += 4;

	FILE *fp = fopen(FinalFileName, "wb");
	if (!fp)
	{
		cur += info.dataLen;
		CSimpleLog::Error("ParseDataToFile内部严重错误.");
		return;
	}
	//fwrite(cur, sizeof(char), info.dataLen, fp);
	fwrite(cur, sizeof(char), info.dataLen, fp);
	fclose(fp);
	cur += info.dataLen;
}

void FileWrapper::ParseDataToDirectory(CString strIp, void*src, int len)
{
	char * cur = (char*)src;
	int iReadLen = 0;
	while (iReadLen < len){
		FileAbout info;
		memcpy(&info.typeId, cur, 1);
		cur++;
		iReadLen++;
		memcpy(&info.NameLen, cur, 1);
		cur++;
		iReadLen++;

		char*strName = new char[info.NameLen + 1];
		memcpy(strName, cur, info.NameLen);
		strName[info.NameLen] = '\0';
		cur += info.NameLen;
		iReadLen += info.NameLen;
		
		CString FinalName = strName;

		if (!m_bIsServer){
			CString strFileName(strName);
			strFileName.Replace(":", "");
			FinalName.Format("%sRemoteFile\\%s\\%s", GetAppDirectory(), strIp, strFileName);
		}
		else{
			FinalName = ConvertServerPath(strName);
		}
		CString strDirectory = GetDir(FinalName);
		CheckAndCreateDir(strDirectory);

		memcpy(&info.dataLen, cur, 4);
		cur += 4;
		iReadLen += 4;
		if (info.typeId == FILEWRAPPER_DIRECTORY){
			CheckAndCreateDir(FinalName);
		}
		else if(info.typeId == FILEWRAPPER_FILE){
			FILE *fp = fopen(FinalName, "wb");
			if (!fp){
				cur += info.dataLen;
				iReadLen += info.dataLen;
				CSimpleLog::Error("ParseDataToDirectory failed.(file open failed.)");
				return;
			}
			fwrite(cur, sizeof(char), info.dataLen, fp);
			fclose(fp);
			cur += info.dataLen;
			iReadLen += info.dataLen;
		}
		else {
			CSimpleLog::Error("ParseDataToDirectory failed.(info.typeId error)");
			return;
		}
	}

}

void FileWrapper::SetFileData(char*filename, void *dst, int*len)
{
	int fileSize = 0;
	m_mapLock.Lock();
	fileSize = m_mapFileSize[filename]; // 保证filename的key一定存在。
	m_mapLock.Unlock();
	FileAbout info;
	info.typeId = FILEWRAPPER_FILE;
	info.NameLen = strlen(filename);
	info.dataLen = fileSize;

	char *cur = (char*)dst;
	memcpy(cur, &info.typeId, 1);
	cur++;
	memcpy(cur, &info.NameLen, 1);
	cur++;
	memcpy(cur, filename, info.NameLen);
	cur += info.NameLen;
	memcpy(cur, &info.dataLen, sizeof(int));
	cur += sizeof(int);
	ReadFile(filename, cur, info.dataLen);
	*len = FileAbout::GetSize() + info.NameLen + info.dataLen;
}

void FileWrapper::SetDirectoryData(char* directoryName, void*dst, int*len)
{
	FileAbout info;
	info.typeId = FILEWRAPPER_DIRECTORY;
	info.NameLen = strlen(directoryName);
	info.dataLen = 0;
	//dst = new char[sizeof(info) + info.NameLen + info.dataLen];
	char *cur = (char*)dst;
	memcpy(cur, &info.typeId, 1);
	cur++;
	memcpy(cur, &info.NameLen, 1);
	cur++;
	memcpy(cur, directoryName, info.NameLen);
	cur += info.NameLen;
	memcpy(cur, &info.dataLen, sizeof(int));
	cur += sizeof(int);
	*len = FileAbout::GetSize() + info.NameLen + info.dataLen;
}

int FileWrapper::FileLoopProcess(CString strFolder, void *dst)
{
	char szFind[260];
	char szFile[1000] = { 0 };

	int iOffset = 0;

	// 目录处理
	char* pCur = (char*)dst;
	int iLen = 0;
	SetDirectoryData(strFolder.GetBuffer(), pCur, &iLen);
	pCur += iLen;
	iOffset += iLen;

	WIN32_FIND_DATA FindFileData;
	strFolder += "\\";
	strcpy(szFind, strFolder.GetBuffer());
	strcat(szFind, "*.*");


	//找任务配置文件
	CString strConfFile;
	HANDLE hFind = ::FindFirstFile(szFind, &FindFileData);
	if (INVALID_HANDLE_VALUE == hFind)
		return iOffset;
	while (TRUE)
	{
		if (FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			if (FindFileData.cFileName[0] != '.')
			{
				CString strSubName = FindFileData.cFileName;
				CString strSubPath = strFolder + strSubName;
				iLen = FileLoopProcess(strSubPath, pCur);
				pCur += iLen;
				iOffset += iLen;
			}
		}
		else
		{
			CString strTmp = FindFileData.cFileName; //保存文件名，包括后缀名
			CString strCheckFilePath = strFolder + "\\" + strTmp;

			//文件处理
			SetFileData(strCheckFilePath.GetBuffer(), pCur, &iLen);
			pCur += iLen;
			iOffset += iLen;
		}
		if (!FindNextFile(hFind, &FindFileData))
			break;
	}

	FindClose(hFind);
	return iOffset;

}

int FileWrapper::GetFolderSize(CString strFolder)
{
	m_iFolderSize = 0;
	FileLoopProcessGetSize(strFolder);
	return m_iFolderSize;
}

void FileWrapper::FileLoopProcessGetSize(CString strFolder)
{
	char szFind[260];
	char szFile[1000] = { 0 };

	//目录处理
	FileAbout info;
	info.typeId = FILEWRAPPER_DIRECTORY;
	info.NameLen = strlen(strFolder);
	info.dataLen = 0;
	m_iFolderSize += FileAbout::GetSize() + info.NameLen + info.dataLen;

	WIN32_FIND_DATA FindFileData;
	strFolder += "\\";
	strcpy(szFind, strFolder.GetBuffer());
	strcat(szFind, "*.*");


	//找任务配置文件
	CString strConfFile;
	HANDLE hFind = ::FindFirstFile(szFind, &FindFileData);
	if (INVALID_HANDLE_VALUE == hFind)
		return;
	while (TRUE)
	{
		if (FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			if (FindFileData.cFileName[0] != '.')
			{
				CString strSubName = FindFileData.cFileName;
				CString strSubPath = strFolder + strSubName;
				FileLoopProcessGetSize(strSubPath);
			}
		}
		else
		{
			CString strTmp = FindFileData.cFileName; //保存文件名，包括后缀名
			CString strCheckFilePath = strFolder + "\\" + strTmp;

			//文件处理
			info.typeId = FILEWRAPPER_FILE;
			info.NameLen = strlen(strCheckFilePath);
			info.dataLen = GetFileLength(strCheckFilePath);
			m_mapLock.Lock();
			m_mapFileSize[strCheckFilePath] = info.dataLen;
			m_mapLock.Unlock();
			m_iFolderSize += FileAbout::GetSize() + info.NameLen + info.dataLen;
		}
		if (!FindNextFile(hFind, &FindFileData))
			break;
	}

	FindClose(hFind);
}

void FileWrapper::CheckAndCreateDir(CString strFolder)
{
	std::string str = strFolder.GetBuffer();
	std::string::size_type pos = str.find('\\');
	while (pos != std::string::npos){
		std::string curStr = str.substr(0, pos);
		int n = _access(curStr.c_str(), 0);
		if (n == -1){
			_mkdir(curStr.c_str());
		}
		pos = str.find('\\', pos + 1);
	}
	int n = _access(str.c_str(), 0);
	if (n == -1){
		_mkdir(str.c_str());
	}
}

CString FileWrapper::GetDir(CString fileName)
{
	std::string str = fileName.GetBuffer();
	std::string::size_type pos = str.find_last_of('\\');
	std::string strDir = "";
	if (pos != std::string::npos){
		strDir = str.substr(0, pos);
	}
	return CString(strDir.c_str());
}

CString FileWrapper::GetAppDirectory()
{
	TCHAR szFilePath[MAX_PATH + 1] = { 0 };
	GetModuleFileName(NULL, szFilePath, MAX_PATH);
	(_tcsrchr(szFilePath, _T('\\')))[1] = 0; // 删除文件名，只获得路径字串
	CString str_url = szFilePath;
	return str_url;
}

CString FileWrapper::ConvertServerPath(CString fileName)
{
	CString FinalName;
	std::string strFileName = fileName;
	std::string::size_type pos = strFileName.find("RemoteFile\\");
	if (pos != std::string::npos){
		pos = strFileName.find("\\", pos + 11);
		if (pos != std::string::npos){
			std::string path = strFileName.substr(pos + 1);
			pos = path.find('\\'); //盘符后面 '\'
			if (pos != std::string::npos){
				std::string finalPath = path.substr(0, pos) + ':' + path.substr(pos);
				FinalName.Format("%s", finalPath.c_str());
				return FinalName;
			}
		}
	}
	FinalName.Format("%s", fileName.GetBuffer());
	return FinalName;
}
