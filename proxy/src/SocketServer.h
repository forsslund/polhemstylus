#pragma once

#include <stdint.h>

#ifdef _WIN32
	#include <winsock2.h>
	#include < afunix.h >
	using ssize_t = __int64;
#else
	#include <sys/socket.h>
	#define SOCKET int


	/*
	* Below stolen from WinSock2: 
	* This is used instead of -1, since the
	* SOCKET type is unsigned.
	*/
	#define INVALID_SOCKET  (SOCKET)(~0)
	#define SOCKET_ERROR            (-1)
#endif

#include <thread>
#include <mutex>
#include <list>

class SocketServer {
public:
	SocketServer() : listenThread() //Default threadummy
	{

	}
	~SocketServer() {
		Shutdown();
	}

	bool Start(std::string socketPath);
	bool Shutdown();
	bool Send(uint16_t value);
	bool Send(std::string str);
	bool HasActiveClient();
private:
	void Listen();
	bool isRunning = false;
	#ifdef _WIN32
	WSADATA wsaData{0};
	#endif
	bool stopServer = false;
	SOCKET listenSocket = INVALID_SOCKET;
	std::list<SOCKET> activeConnections;
	std::mutex activeConnections_mutex;
	std::thread listenThread;
};