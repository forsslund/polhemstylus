#pragma once
#include <winsock2.h>
#include <afunix.h>
#include <list>
#include <thread>
#include <mutex>

template <class T> 
class SocketClient {
public:
	void Start(std::string socketPath);

	SocketClient() : socketPath(""), listenThread() //Default threadummy
	{
		// Initialize Winsock
		int iResult;
		iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
		if (iResult != 0) {
			printf("WSAStartup failed: %d\n", iResult);
		}
	}
	~SocketClient() {
		stopClient = true;
		if (listenThread.joinable()) listenThread.join();
	}
	
	void Shutdown() { 
		stopClient = true;
	}	
	T Get() { 
		const std::lock_guard<std::mutex> lock(dataMutex);
		return data; 
	}

private:
	void Listen();
	bool isRunning = false;
	WSADATA wsaData;
	bool stopClient = false;	
	bool connected = false;
	T data = { 0 };
	std::mutex dataMutex;	
	std::thread listenThread;
	std::string socketPath;
};


///////////////////// SocketClient.cpp ////////

template <class T>
void SocketClient<T>::Start(std::string socketPath_) {
	// Fire up a thread to take care of incomming connections. But first check that thread is not running (i.e. not joinable)
	if (!listenThread.joinable()) {
		socketPath = socketPath_;
		listenThread = std::thread(&SocketClient<T>::Listen, this);
	}
		
}

#ifdef _WIN64
using ssize_t = __int64;
#endif

template <class T>
void SocketClient<T>::Listen() {
	printf("\nIn listening thread\n");

	struct sockaddr_un addr;
	int dataSize = sizeof(data);
	char* dataBuffer = new char[dataSize];
	
	while (!stopClient) {
		bool connectionLost = false;
		// Try setting up a connection
		SOCKET listenSocket = INVALID_SOCKET;
		listenSocket = socket(AF_UNIX, SOCK_STREAM, 0);
		if (listenSocket != INVALID_SOCKET) {
			connected = true;
			memset(&addr, 0, sizeof(struct sockaddr_un));
			addr.sun_family = AF_UNIX;
			strncpy_s(addr.sun_path, socketPath.c_str(), sizeof(addr.sun_path) - 1);
			int res = connect(listenSocket, (struct sockaddr*)&addr, sizeof(struct sockaddr_un));
			if (res >= 0) {
				printf("Connected to server \n");
				while (!stopClient && !connectionLost) {

					// Recive until the whole datapackage is filled
					int byteCounter = 0;
					while (byteCounter < dataSize) {
						int res = recvfrom(listenSocket, &dataBuffer[byteCounter], dataSize - byteCounter, 0, NULL, NULL);
						if (res < 0) {
							connectionLost = true;
							break;
						}
						else {
							byteCounter += res;
						}
					}

					if (byteCounter == dataSize) {
						// Atomically store data
						const std::lock_guard<std::mutex> lock(dataMutex);
						memcpy((void*)&data, dataBuffer, sizeof(data));						
					}
				}
			}
			shutdown(listenSocket, 0);
			closesocket(listenSocket);
			listenSocket = INVALID_SOCKET;
			connected = false;			
		}
		Sleep(100); // Wait before trying again
	}
	WSACleanup();
	delete [] dataBuffer;
	return;
}
