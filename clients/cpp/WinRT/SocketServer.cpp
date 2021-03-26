#include "pch.h"

#include <winsock2.h>
#include <WS2tcpip.h>
#include < afunix.h >

void errExit(const char* str) { printf("%s", str); exit(1); }
void fatal(const char* str) { printf("%s", str);  exit(1); }
void usageErr(const char* str) { printf("%s", str); exit(1); }

#ifdef _WIN64
using ssize_t = __int64;
#endif

#define BUF_SIZE 10 
#include <mutex>
#include "socketServer.h"

	bool SocketServer::Start(std::string socketPath) {
		if (isRunning) return true;
		int iResult;

		// Initialize Winsock
		iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
		if (iResult != 0) {
			printf("WSAStartup failed: %d\n", iResult);
			return true;
		}

		listenSocket = socket(AF_UNIX, SOCK_STREAM, 0);       /* Create server socket */
		if (listenSocket == INVALID_SOCKET) {
			WSACleanup();
			return true;
		}

		// Make sure adress is ok
		if ( remove( socketPath.c_str() ) == -1 && errno != ENOENT) {
			return true;
		}

		// Create adress structure
		struct sockaddr_un svaddr;
		memset(&svaddr, 0, sizeof(struct sockaddr_un));
		svaddr.sun_family = AF_UNIX;
		strncpy_s(svaddr.sun_path, socketPath.c_str(), sizeof(svaddr.sun_path) - 1);

		// Bind adress
		if (::bind(listenSocket, (struct sockaddr*)&svaddr, sizeof(struct sockaddr_un)) == -1)
			errExit("bind");
		
		// Now, fire up a thread to take care of incomming connections
		listenThread = std::thread(&SocketServer::Listen, this);

		isRunning = true;
		return false;	// No error
	}

	bool SocketServer::Shutdown() {
		stopServer = true;
		return false;
	}

	bool SocketServer::Send(uint16_t value) {
		const std::lock_guard<std::mutex> lock(activeConnections_mutex);
		auto i = activeConnections.begin();
		while (i != activeConnections.end()) {
			int result = send(*i, (char*)&value, sizeof(value), 0);
			if (result < 0) {
				// Inactive or broken connection, close and remove
				closesocket(*i);
				i = activeConnections.erase(i);
			}
			else {
				i++;
			}
		}
		return false;
	}

	bool SocketServer::Send(std::string str)
	{
		const std::lock_guard<std::mutex> lock(activeConnections_mutex);
		auto i = activeConnections.begin();
		while (i != activeConnections.end()) {
			int result = send(*i, str.c_str(), (int)str.length(), 0);
			if (result < 0) {
				// Inactive or broken connection, close and remove
				closesocket(*i);
				i = activeConnections.erase(i);
			}
			else {
				i++;
			}
		}		
		return false;
	}

	bool SocketServer::HasActiveClient()
	{	
		const std::lock_guard<std::mutex> lock(activeConnections_mutex);
		return activeConnections.size() > 0;
	}

	void SocketServer::Listen() {		
		isRunning = true;
		int rval = 0;
		SOCKET messageSocket = INVALID_SOCKET;
		
		listen(listenSocket, 5);
		while(!stopServer)
		{			
			//
			// Check if there is a connection waiting
			//
			fd_set set;
			FD_ZERO(&set); // clear the set
			FD_SET(listenSocket, &set);
			struct timeval timeout;			
			timeout.tv_sec = 2;
			timeout.tv_usec = 0;
			int rv = select(1, &set, NULL, NULL, &timeout);	// Any waiting connection? Timeout after 2 sec and recheck

			if (rv > 0) {
				// Waiting connection, accept it to a new socket and store
				messageSocket = accept(listenSocket, 0, 0);	// Blocks untill we have a connection
				if (messageSocket != INVALID_SOCKET) {
					const std::lock_guard<std::mutex> lock(activeConnections_mutex);
					activeConnections.push_back(messageSocket);	// Store the socket
					printf("Client connected!\n");
				}
			}			
		}

		// close all connections
		const std::lock_guard<std::mutex> lock(activeConnections_mutex);
		for (SOCKET a : activeConnections) {
			shutdown(a, 0);
			closesocket(a);
		}
		activeConnections.clear();

		shutdown(listenSocket, 0);
		closesocket(listenSocket);
		WSACleanup();
		isRunning = false;
	}
