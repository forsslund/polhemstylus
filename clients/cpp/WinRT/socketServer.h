#pragma once

#include <stdint.h>

#include <winsock2.h>
#include <afunix.h>
#include <mutex>

class SocketServer {
public:
	SocketServer() : listenThread() //Default threadummy
	{

	}
	~SocketServer() {
		stopServer = true;
		if (listenThread.joinable()) listenThread.join();
	}

	bool Start(std::string socketPath);
	bool Shutdown();
	bool Send(uint16_t value);
	bool Send(std::string str);
	bool HasActiveClient();
private:
	void Listen();
	bool isRunning = false;
	WSADATA wsaData{0};
	bool stopServer = false;
	SOCKET listenSocket = INVALID_SOCKET;
	std::list<SOCKET> activeConnections;
	std::mutex activeConnections_mutex;
	std::thread listenThread;
};