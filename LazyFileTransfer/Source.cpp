#include "Source.h"

void Source::SendData(void* data, int len)
{
	send(this->ConnectedDest, (char*)&len, sizeof(int), 0);
	send(this->ConnectedDest, (char*)data, len, 0);
}

void Source::BeginDataTransfer()
{
	fillIOBuffer(TRANSFER_BEGIN);
	sendIOBuffer();
}

void Source::EndDataTransfer()
{
	fillIOBuffer(TRANSFER_END);
	sendIOBuffer();
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

	fillIOBuffer(FILE_INFO_HEADER);
	sendIOBuffer();

	for (int i = 0; i < FilesToCopy.size(); i++)
	{
		fillIOBuffer(FILE_INFO_FILE_BEGIN);
		sendIOBuffer();

		fillIOBuffer(FILE_INFO_FILEPATH);
		sendIOBuffer();

		fillIOBuffer(FILE_INFO_FILEPATH);
		sendIOBuffer();

		SendData(FilesToCopy.at(i).DestFilePath, FilesToCopy.at(i).DestFilePathLen);

		fillIOBuffer(FILE_INFO_FILE_COMPLETE);
		sendIOBuffer();
	}

	fillIOBuffer(FILE_INFO_HEADER_COMPLETE);
	sendIOBuffer();

	EndDataTransfer();

	sendFileData();
}

void Source::sendFileData() 
{
	int bytesReceived = recv(this->ConnectedDest, _ioBuffer, _ioBufferLen, 0);

	ifstream file;
	streampos size;
	streampos currPos;

	long long data = 0;

	const int BLOCK_SIZE = 4 * 1024;

	if (bytesReceived == sizeof(long long) && *(long long*)_ioBuffer == DEST_REQ_DATA)
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

					file.read(_ioBuffer + 4, bytesToRead);
					*(int*)(_ioBuffer) = bytesToRead;
					sendIOBuffer();

					currPos += bytesToRead;
				}

				fillIOBuffer(FILE_INFO_EOF);
				sendIOBuffer();
				file.close();

				cout << "File " << currFile->SourceFilePath << " sent" << endl;
			}
			else 
			{
				cout << "File " << currFile->SourceFilePath << " could not be opened" << endl;

				fillIOBuffer(FILE_INFO_FILE_CANCELED);
				sendIOBuffer();
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
