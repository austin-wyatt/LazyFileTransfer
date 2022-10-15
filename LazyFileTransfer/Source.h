#pragma once

#include <iostream>
#include <WinSock2.h>
#include <string>
#include <WS2tcpip.h>
#include "public.h"
#include <vector>
#include <fstream>

#define _sendData(data) &data, sizeof(data)

using namespace std;


struct SourceFileInfo
{
	char* DestFilePath;
	int DestFilePathLen;

	char* SourceFilePath;
};

class Source 
{
public:
	Source() {}

	Source(SOCKET destSocket) 
	{
		ConnectedDest = destSocket;
	}

	SOCKET ConnectedDest;

	vector<SourceFileInfo> FilesToCopy = vector<SourceFileInfo>();

	void SendData(void* data, int len);
	void BeginDataTransfer();
	void EndDataTransfer();

	void CloseConnection();

	void AddFile(char* sourcePath, char* destinationPath);
	void SendFiles();

private:
	void sendFileInfo();
	void sendFileData();

	static const int _readBufferLen = 8 * 1024;
	char _readBuffer[_readBufferLen];
};