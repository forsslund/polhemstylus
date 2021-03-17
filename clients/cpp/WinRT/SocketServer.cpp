#include "pch.h"

#include <winsock2.h>
#include <WS2tcpip.h>
#include < afunix.h >

#define UNIX_PATH_MAX 108
//#define SV_SOCK_PATH "/tmp/ud_ucase"
//#define SV_SOCK_PATH "server.sock"
#define SV_SOCK_PATH "/temp/gnu"
void errExit(const char* str) { printf("%s", str); exit(1); }
void fatal(const char* str) { printf("%s", str);  exit(1); }
void usageErr(const char* str) { printf("%s", str); exit(1); }

#ifdef _WIN64
using ssize_t = __int64;
#endif

#define BUF_SIZE 10 
void socketServer() {


	// All processes (applications or DLLs) that call Winsock functions must initialize the use of the Windows Sockets DLL before making other Winsock functions calls. This also makes certain that Winsock is supported on the system.
	WSADATA wsaData;
	int iResult;

	// Initialize Winsock
	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0) {
		printf("WSAStartup failed: %d\n", iResult);
		return ;
	}
	//std::string path = Path.GetTempPath();
	struct sockaddr_un svaddr, claddr;
	int j;
	SOCKET listenSocket = INVALID_SOCKET;
	ssize_t numBytes;
	socklen_t len;
	char buf[BUF_SIZE];
	
	listenSocket = socket(AF_UNIX, SOCK_STREAM, 0);       /* Create server socket */
	if (listenSocket == INVALID_SOCKET) {
		WSACleanup();
		errExit("socket");
	}
		


	timeval read_timeout;
	read_timeout.tv_sec = 0;
	read_timeout.tv_usec = 5000; // wait for messages 5ms, then check if thread should quit
	setsockopt(listenSocket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&read_timeout, sizeof read_timeout);


	/* Construct well-known address and bind server socket to it */

	if (remove(SV_SOCK_PATH) == -1 && errno != ENOENT)
		errExit("remove");

	memset(&svaddr, 0, sizeof(struct sockaddr_un));
	svaddr.sun_family = AF_UNIX;
	strncpy(svaddr.sun_path, SV_SOCK_PATH, sizeof(svaddr.sun_path) - 1);

	if (::bind(listenSocket, (struct sockaddr*)&svaddr, sizeof(struct sockaddr_un)) == -1)
		errExit("bind");

	
	
	//aklsdjf
	int rval=0;
	int messageSocket = INVALID_SOCKET;
	printf("Starting socket server \n");
	listen(listenSocket, 5);
	do {
		    messageSocket = accept(listenSocket, 0, 0);	// Blocks untill we have a connection
		    if (messageSocket == -1)
		        perror("accept");
		    else do {
				memset(buf, 0, sizeof(buf));
				rval = recv(messageSocket, buf, BUF_SIZE,0);	//  If no messages are available at the socket, the receive calls wait for a message to arrive
		        if( rval < 0)
		            perror("reading stream message");
		        else if (rval == 0)
		            printf("Ending connection\n");
				else
				{
					printf("Connected: -->%s\n", buf);
					do {
						

					}while
					int valueToSend = 42;
					send(messageSocket, (char*)&valueToSend, sizeof(valueToSend), 0);
				}
		            
		
			} while (rval > 0);
			closesocket(messageSocket);
	} while (messageSocket != -1);
	// cleanup
	closesocket(listenSocket);
	WSACleanup();
	//asdklf
	
	/* Receive messages, convert to uppercase, and return to client */

/*	while (1) {
		
		len = sizeof(struct sockaddr_un);
		numBytes = recvfrom(listenSocket, buf, BUF_SIZE, 0,
			(struct sockaddr*)&claddr, &len);
		if (numBytes == -1)
			continue; // (timeout)           errExit("recvfrom");

		//printf("Server received %ld bytes from %s\n", (long) numBytes, claddr.sun_path);
		

		numBytes = 2;

		for (j = 0; j < numBytes; j++)
			printf("%c ",buf[j]);
		int valueToSend = 42;*/
		//send(listenSocket, (char*) &valueToSend, sizeof(valueToSend), 0);
		//if (sendto(sfd, buf, numBytes, 0, (struct sockaddr*)&claddr, len) !=
		//	numBytes)
		//	fatal("sendto");


		/*
		if(prev_bt_data[0]!=bt_data[0] || prev_bt_data[1]!=bt_data[1] ){
			   prev_bt_data[0]=bt_data[0];
			   prev_bt_data[1]=bt_data[1];

			   printf("Got new data in main loop: %02x %02x\n",bt_data[0],bt_data[1]);
		   }
		   */
		   //newSocketRequest = true;
	
	printf("Socket server quit.\n");
}
