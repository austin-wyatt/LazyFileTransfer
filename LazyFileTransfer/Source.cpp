#include "Source.h"

void Source::SendData(void* data, int len)
{
	send(this->ConnectedDest, (char*)&len, sizeof(int), 0);
	send(this->ConnectedDest, (char*)data, len, 0);
}

void Source::BeginDataTransfer()
{
	long long data = TRANSFER_BEGIN;
	SendData(_sendData(data));
}

void Source::EndDataTransfer()
{
	long long data = TRANSFER_END;
	SendData(_sendData(data));
}

void Source::CloseConnection() 
{
	if (this->ConnectedDest != INVALID_SOCKET) 
	{
		SendData(nullptr, 0);
		closesocket(this->ConnectedDest);
		this->ConnectedDest = INVALID_SOCKET;
	}
}


void Source::SendFiles() 
{
	sendFileInfo();
}

void Source::sendFileInfo()
{
	BeginDataTransfer();

	long long data;

	data = FILE_INFO_HEADER;
	SendData(_sendData(data));

	for (int i = 0; i < FilesToCopy.size(); i++)
	{
		data = FILE_INFO_FILE_BEGIN;
		SendData(_sendData(data));

		data = FILE_INFO_FILEPATH;
		SendData(_sendData(data));

		SendData(FilesToCopy.at(i).DestFilePath, FilesToCopy.at(i).DestFilePathLen);

		data = FILE_INFO_FILE_COMPLETE;
		SendData(_sendData(data));
	}

	data = FILE_INFO_HEADER_COMPLETE;
	SendData(_sendData(data));

	EndDataTransfer();

	sendFileData();
}

void Source::sendFileData() 
{
	int bytesReceived = recv(this->ConnectedDest, _readBuffer, _readBufferLen, 0);

	ifstream file;
	streampos size;
	streampos currPos;

	long long data = 0;

	const int BLOCK_SIZE = 4 * 1024;

	if (bytesReceived == sizeof(long long) && *(long long*)_readBuffer == DEST_REQ_DATA)
	{
		SourceFileInfo* currFile;

		for (int i = 0; i < FilesToCopy.size(); i++) 
		{
			currFile = &FilesToCopy.at(i);

			file.open(currFile->SourceFilePath, fstream::binary | fstream::out | fstream::ate);

			size = file.tellg();
			currPos = 0;

			file.seekg(0);

			int bytesToRead = 0;

			if (file.is_open()) 
			{
				while (currPos < size) 
				{
					bytesToRead = size - currPos;
					bytesToRead = bytesToRead > BLOCK_SIZE ? BLOCK_SIZE : bytesToRead;

					file.read(_readBuffer, bytesToRead);
					SendData(_readBuffer, bytesToRead);
					currPos += bytesToRead;
				}

				data = FILE_INFO_EOF;
				SendData(_sendData(data));
				file.close();

				cout << "File " << currFile->SourceFilePath << " sent" << endl;
			}
			else 
			{
				cout << "File " << currFile->SourceFilePath << " could not be opened" << endl;

				data = FILE_INFO_FILE_CANCELED;
				SendData(_sendData(data));
			}
		}
	}
}

void Source::AddFile(char* sourcePath, char* destinationPath)
{
	SourceFileInfo info = SourceFileInfo();

	info.SourceFilePath = sourcePath;
	info.DestFilePath = destinationPath;

	int i = 0;
	for (i = 0; *(destinationPath + i) != '\0'; i++) 
	{
		info.DestFilePathLen = i + 1;
	}

	FilesToCopy.push_back(info);
}
