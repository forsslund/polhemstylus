#pragma once

class SocketServer {
public:
	SocketServer() : listenThread() //Default threadummy
	{

	}
	~SocketServer() {
		stopServer = true;
		if (listenThread.joinable()) listenThread.join();
	}

	bool Start();
	bool Shutdown();
	bool Send(int value);
	bool Send(std::string str);
private:
	void Listen();
	bool isRunning = false;
	WSADATA wsaData;
	bool stopServer = false;
	SOCKET listenSocket = INVALID_SOCKET;
	std::list<SOCKET> activeConnections;
	std::mutex activeConnections_mutex;
	std::thread listenThread;
};