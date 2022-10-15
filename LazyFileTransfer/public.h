#pragma once

const int TRANSFER_PORT_NUM = 27500;

const long long DEST_REQ_DATA = 0x25;

const long long TRANSFER_BEGIN = 0xFFFFFFFFFFFF0;
const long long TRANSFER_END = 0xFFFFFFFFFFFF1;

const long long FILE_INFO_HEADER = 0xFFFFFFFF0;
const long long FILE_INFO_HEADER_COMPLETE = 0xFFFFFFFFF;

const long long FILE_INFO_FILE_BEGIN = 0xFFFFFFFF1;
const long long FILE_INFO_FILEPATH = 0xFFFFFFFF2;
const long long FILE_INFO_FILE_LENGTH = 0xFFFFFFFF3;
const long long FILE_INFO_FILE_COMPLETE = 0xFFFFFFFF4;

const long long FILE_INFO_FILE_CANCELED = 0xFFFFFFFFD;
const long long FILE_INFO_EOF = 0xFFFFFFFFE;


class FileInfo 
{
public:

	~FileInfo() 
	{
		delete[] FilePath;
	}

	char* FilePath = nullptr;
	int FilePathLen = 0;
};
