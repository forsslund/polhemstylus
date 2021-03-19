#include "SocketClient.h"

#define UNIX_PATH_MAX 108
//#define SV_SOCK_PATH "/tmp/ud_ucase"
//#define SV_SOCK_PATH "server.sock"
#define SV_SOCK_PATH "/temp/gnu"


#ifdef _WIN64
using ssize_t = __int64;
#endif


template <class T>
bool SocketClient<T>::Start() {
	if (isRunning) return true;
	int iResult;

	// Initialize Winsock
	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0) {
		printf("WSAStartup failed: %d\n", iResult);
		return true;
	}
	
	// Now, fire up a thread to take care of incomming connections
	listenThread = std::thread(&SocketClient<class T>::Listen, this);

	WSACleanup();
	return false;	// No error
}


template <class T>
void SocketClient<T>::Listen() {
	printf("\nIn listening thread\n");
	if (!isRunning) {
		isRunning = true;
		struct sockaddr_un addr;
		int dataSize = sizeof(data);
		char* dataBuffer = new char[dataSize];
		isRunning = true;
		bool connectionLost = false;
		while (!stopClient) {
			// Try setting up a connection
			SOCKET listenSocket = INVALID_SOCKET;
			listenSocket = socket(AF_UNIX, SOCK_STREAM, 0);
			if (listenSocket != INVALID_SOCKET) {

				memset(&addr, 0, sizeof(struct sockaddr_un));
				addr.sun_family = AF_UNIX;
				strncpy_s(addr.sun_path, SV_SOCK_PATH, sizeof(addr.sun_path) - 1);
				int res = connect(listenSocket, (struct sockaddr*)&addr, sizeof(struct sockaddr_un));
				if (res > 0) {
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
			}
			// Wait before trying again?
		}
	}
	isRunning = false;
}

template <class T>
bool SocketClient<T>::Shutdown() {
	stopClient = true;
	return false;
}
