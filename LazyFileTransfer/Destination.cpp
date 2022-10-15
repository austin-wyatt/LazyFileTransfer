#include "Destination.h"


void Destination::Listen() 
{
	listen(this->TransferSocket, 20);

	sockaddr_in sourceAddr;
	int addrSize = sizeof(sourceAddr);

	while (true)
	{
		std::cout << "Listening on port " << TRANSFER_PORT_NUM << std::endl;

		this->ConnectedSocket = accept(this->TransferSocket, (sockaddr*)&sourceAddr, &addrSize);
		this->ConnectedSocketAddr = sourceAddr;

		if (this->ConnectedSocket != INVALID_SOCKET) 
		{
			std::cout << "Connection opened" << std::endl;
		}
		else 
		{
			std::cout << "Connection failed" << std::endl;
		}
	
		startDataLoop();
	}
}

void Destination::startDataLoop() 
{
	int byteIndex = 0;
	int bytesReceived = 0;

	int totalBytesReceived = 0;
	long long controlData = 0;

	bool reading = false;

	int packetSize = 0;

	while (this->ConnectedSocket != INVALID_SOCKET)
	{
		bytesReceived = recv(this->ConnectedSocket, _readBuffer + totalBytesReceived, _readBufferLen - totalBytesReceived, 0);

		totalBytesReceived += bytesReceived;

		if (bytesReceived <= 0)
		{
			std::cout << "Connection closed" << std::endl;
			closesocket(this->ConnectedSocket);
			this->ConnectedSocket = INVALID_SOCKET;
			return;
		}
		else if (!reading && totalBytesReceived >= (sizeof(int) + sizeof(long long)))
		{
			packetSize = *(int*)(_readBuffer + byteIndex);
			byteIndex += sizeof(int);

			controlData = *(long long*)(_readBuffer + byteIndex);

			if (controlData == TRANSFER_BEGIN)
			{
				reading = true;
				byteIndex += sizeof(long long);
			}
			else 
			{
				std::cout << "Data transfer must begin with TRANSFER_BEGIN header" << std::endl;
				closesocket(this->ConnectedSocket);
				this->ConnectedSocket = INVALID_SOCKET;
				return;
			}
		}

		if (totalBytesReceived >= sizeof(long long))
		{
			controlData = *(long long*)(_readBuffer + totalBytesReceived - sizeof(long long));

			if (controlData == TRANSFER_END) 
			{
				reading = false;
				totalBytesReceived -= sizeof(long long) + sizeof(int);
			}
		}

		if (reading || totalBytesReceived < (sizeof(int) + sizeof(long long)))
			continue;
		

		while (byteIndex < totalBytesReceived)
		{
			packetSize = *(int*)(_readBuffer + byteIndex);
			byteIndex += sizeof(int);

			if (packetSize <= 0)
			{
				std::cout << "Connection closed" << std::endl;
				closesocket(this->ConnectedSocket);
				this->ConnectedSocket = INVALID_SOCKET;
			}
			else
			{
				long long data = *(long long*)(_readBuffer + byteIndex);

				if (data == FILE_INFO_HEADER)
				{
					processFileDataInfo(byteIndex + packetSize, totalBytesReceived);
					std::cout << "File data processed" <<endl;

					totalBytesReceived = 0;
					byteIndex = 0;
				}
				else
				{
					std::cout << "data: " << _readBuffer + byteIndex << std::endl;
				}
			}

			byteIndex += packetSize;
		}
	}
}

void Destination::processFileDataInfo(int byteIndex, int totalbytesReceived) 
{
	vector<FileInfo*> filesToProcess = vector<FileInfo*>();

	FileInfo* currFileInfo = nullptr;

	long long currentSection = 0;

	int newLength;
	char* tempCharBuf;

	int packetSize;

	while (byteIndex < totalbytesReceived)
	{
		packetSize = *(int*)(_readBuffer + byteIndex);
		byteIndex += sizeof(int);

		if (packetSize == sizeof(long long))
		{
			currentSection = *(long long*)(_readBuffer + byteIndex);

			switch (currentSection)
			{
			case FILE_INFO_FILE_BEGIN:
				currFileInfo = new FileInfo();
				break;
			case FILE_INFO_FILE_COMPLETE:
				filesToProcess.push_back(currFileInfo);
				break;
			case FILE_INFO_HEADER_COMPLETE:
				processFileData(&filesToProcess);
				return;
			case FILE_INFO_FILE_LENGTH:
				byteIndex += packetSize + sizeof(int);
				//currFileInfo->FileSize = *(long long*)(_readBuffer + byteIndex);
				break;
			}
		}
		else
		{
			switch (currentSection)
			{
			case FILE_INFO_FILEPATH:
				//create a new string large enough to hold the passed information
				newLength = currFileInfo->FilePathLen + packetSize;
				tempCharBuf = new char[newLength + 1];

				if (currFileInfo->FilePathLen != 0)
				{
					memcpy(tempCharBuf, currFileInfo->FilePath, currFileInfo->FilePathLen);
				}

				memcpy(tempCharBuf + currFileInfo->FilePathLen, (_readBuffer + byteIndex), packetSize);

				delete[] currFileInfo->FilePath;

				currFileInfo->FilePath = tempCharBuf;
				currFileInfo->FilePathLen = newLength;
				tempCharBuf[newLength] = '\0';
				break;
			}
		}

		byteIndex += packetSize;
	}
}

void Destination::processFileData(vector<FileInfo*>* fileInfo) 
{
	long long data = DEST_REQ_DATA;
	send(this->ConnectedSocket, (char*)&data, sizeof(long long), 0);

	FileInfo* curr;

	ofstream file;

	int currentBytesReceived = 0;
	int fileBytesRecieved = 0;
	int readBufferOffset = 0;

	int packetSize = 0;

	int totalBytesRecieved = 0;

	for (int i = 0; i < fileInfo->size(); i++) 
	{
		curr = fileInfo->at(i);

		file.open(curr->FilePath, fstream::binary | fstream::trunc | fstream::in);

		long long controlVal = 0;

		bool success = true;

		while (controlVal != FILE_INFO_EOF)
		{
			int bytesReceived = recv(this->ConnectedSocket, _readBuffer + currentBytesReceived, _readBufferLen - currentBytesReceived, 0);

			currentBytesReceived += bytesReceived;

			totalBytesRecieved += bytesReceived;

			//act on bytes in the queue and ensure that we have more than just a packet header to work with
			while (readBufferOffset < currentBytesReceived && ((currentBytesReceived - readBufferOffset) > sizeof(int)))
			{
				packetSize = *(int*)(_readBuffer + readBufferOffset);
				readBufferOffset += sizeof(int);

				if(packetSize == sizeof(long long))
				{
					controlVal = *(long long*)(_readBuffer + readBufferOffset);
					if (controlVal == FILE_INFO_EOF) 
					{
						readBufferOffset += sizeof(long long);
						break;
					}
					else if (controlVal == FILE_INFO_FILE_CANCELED) 
					{
						readBufferOffset += sizeof(long long);
						success = false;
						break;
					}
				}

				if (currentBytesReceived - readBufferOffset < packetSize) 
				{
					readBufferOffset -= sizeof(int);
					break;
				}
				else 
				{
					file.write(_readBuffer + readBufferOffset, packetSize);
					readBufferOffset += packetSize;
				}
			}

			if (readBufferOffset > 0)
			{
				currentBytesReceived -= readBufferOffset;
				memmove(_readBuffer, _readBuffer + readBufferOffset, currentBytesReceived);
				readBufferOffset = 0;
			}
		}

		if (currentBytesReceived > 0)
		{
			currentBytesReceived -= readBufferOffset;
			memmove(_readBuffer, _readBuffer + readBufferOffset, currentBytesReceived);
		}
		else
		{
			currentBytesReceived = 0;
		}

		readBufferOffset = 0;

		if (success) 
		{
			cout << "File " << curr->FilePath << " copied successfully" << endl;
		}
		else 
		{
			cout << "File " << curr->FilePath << " copy unsuccessful" << endl;
		}
		
		file.close();

		delete curr;
	}
}