#pragma once

#include <stdint.h>

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