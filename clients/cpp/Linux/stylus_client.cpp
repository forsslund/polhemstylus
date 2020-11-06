/*
 *
 *  GattLib - GATT Library
 *
 *  Copyright (C) 2016-2017  Olivier Martin <olivier@labapart.org>
 *
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#include <assert.h>
#include <glib.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

#include <thread>


//-------- Unix sockets example -----

#include <sys/un.h>
#include <sys/socket.h>
#include <unistd.h>     /* Prototypes for many system calls */
#include <errno.h>      /* Declares errno and defines error constants */

#define BUF_SIZE 10             /* Maximum size of messages exchanged
                                   between client and server */

#define SV_SOCK_PATH "/tmp/ud_ucase"
void errExit(const char *str) { printf("%s",str); exit(1);}
void fatal(const char *str) { printf("%s",str);  exit(1);}
void usageErr(const char *str) { printf("%s",str); exit(1);}

//-----------------------------------

extern "C" {
#include "gattlib.h"
}

// ----- Portable keyboard loop break ------------------------------------------
#ifdef WINDOWS
#include <conio.h>
#else
#include <stdio.h>
#include <sys/select.h>
#include <sys/ioctl.h>
#include <termios.h>
//#include <stropts.h>
int _kbhit(){
    static const int STDIN = 0;
    static int initialized = 0;
    if (!initialized){
        // Use termios to turn off line buffering
        struct termios term;
        tcgetattr(STDIN, &term);
        term.c_lflag &= ~ICANON;
        tcsetattr(STDIN, TCSANOW, &term);
        setbuf(stdin, NULL);
        initialized = 1;
    }
    int bytesWaiting;
    ioctl(STDIN, FIONREAD, &bytesWaiting);
    return bytesWaiting;
}
#endif
// -----------------------------------------------------------------------------


volatile bool newSocketRequest=false;


// Battery Level UUID
//const uuid_t g_battery_level_uuid = CREATE_UUID16(0x2A19);

static GMainLoop *m_main_loop;

uint8_t bt_data[] = {0,0};
uint8_t prev_bt_data[] = {0,0};
bool run_server{true};

void socket_server() {

    struct sockaddr_un svaddr, claddr;
    int sfd, j;
    ssize_t numBytes;
    socklen_t len;
    char buf[BUF_SIZE];

    sfd = socket(AF_UNIX, SOCK_DGRAM, 0);       /* Create server socket */
    if (sfd == -1)
        errExit("socket");


	struct timeval read_timeout;
	read_timeout.tv_sec = 0;
	read_timeout.tv_usec = 5000; // wait for messages 5ms, then check if thread should quit
	setsockopt(sfd, SOL_SOCKET, SO_RCVTIMEO, &read_timeout, sizeof read_timeout);


    /* Construct well-known address and bind server socket to it */

    if (remove(SV_SOCK_PATH) == -1 && errno != ENOENT)
        errExit("remove");

    memset(&svaddr, 0, sizeof(struct sockaddr_un));
    svaddr.sun_family = AF_UNIX;
    strncpy(svaddr.sun_path, SV_SOCK_PATH, sizeof(svaddr.sun_path) - 1);

    if (bind(sfd, (struct sockaddr *) &svaddr, sizeof(struct sockaddr_un)) == -1)
        errExit("bind");

    /* Receive messages, convert to uppercase, and return to client */

	printf("Starting socket server \n");
	while(run_server){

        len = sizeof(struct sockaddr_un);
        numBytes = recvfrom(sfd, buf, BUF_SIZE, 0,
                            (struct sockaddr *) &claddr, &len);
        if (numBytes == -1)
 			continue; // (timeout)           errExit("recvfrom");

        //printf("Server received %ld bytes from %s\n", (long) numBytes, claddr.sun_path);
        /*FIXME: above: should use %zd here, and remove (long) cast */

		numBytes=2;

        for (j = 0; j < numBytes; j++)
            buf[j] = bt_data[j];//toupper((unsigned char) buf[j]);

        if (sendto(sfd, buf, numBytes, 0, (struct sockaddr *) &claddr, len) !=
                numBytes)
            fatal("sendto");


		/*
		if(prev_bt_data[0]!=bt_data[0] || prev_bt_data[1]!=bt_data[1] ){
			   prev_bt_data[0]=bt_data[0];
			   prev_bt_data[1]=bt_data[1];

			   printf("Got new data in main loop: %02x %02x\n",bt_data[0],bt_data[1]);
		   }
		   */
		newSocketRequest = true;
	}
	printf("Socket server quit.\n");
}

gatt_connection_t* g_connection;
void* adapter=NULL;
const char* device_address=NULL;
uuid_t service_uuid;

void notification_handler(const uuid_t* uuid, const uint8_t* data, size_t data_length, void* user_data) {
	int i;
	char str[256];
	
	for (i = 0; i < data_length; i++) {
		g_log(NULL, G_LOG_LEVEL_DEBUG, "%02x ", data[i]);		
		if(i<2)
			bt_data[i]=data[i];
	}
	newSocketRequest = false;
}



bool deActivateConnection(){
		if(g_connection==NULL){
			printf("\nNo existing connection");
			return false;
		}

		// Clean up old connection
		gattlib_notification_stop(g_connection, &service_uuid);
		gattlib_disconnect(g_connection);
		g_connection=NULL;
		newSocketRequest=false;
		return false;
}

void disconnected_handler(void *){
		static int nrDisconnects=0;
		nrDisconnects++;
		g_log(NULL, G_LOG_LEVEL_DEBUG, "Disconnected %i",nrDisconnects);
		deActivateConnection();	
}


void activateConnection(){
		if(g_connection!=NULL){
			printf("\nExisting connection, skipping.");
			return;
		}
		
		g_connection = gattlib_connect(adapter, device_address, GATTLIB_CONNECTION_OPTIONS_LEGACY_DEFAULT);
		if (g_connection == NULL) {
			fprintf(stderr, "Fail to connect to the bluetooth device.\n");
			return;
		}

		gattlib_register_notification(g_connection, notification_handler, NULL);
		//gattlib_uuid_to_string(&service_uuid, uuid_str, sizeof(uuid_str));		
		int ret = gattlib_notification_start(g_connection, &service_uuid);
		if (ret) {
			fprintf(stderr, "Fail to start notification.\n");
			gattlib_disconnect(g_connection);			
			puts("Done");
			return;
		}
		gattlib_register_on_disconnect(g_connection, disconnected_handler, NULL);
}

static void on_user_abort(int arg) {
	g_main_loop_quit(m_main_loop);
}

gboolean monitorConnection(gpointer user_data){
	static int ideling;
	if( newSocketRequest == false ){
		if(g_connection != NULL){
			ideling++;
			if(ideling>10000){
				disconnected_handler(NULL);
			}
		}
	}
	else
	{
		ideling=0;
		if(g_connection==NULL)
		{
			activateConnection();
			newSocketRequest=false; // Only attempt reconnect once per socket request
		}
	}
	return true;	
};

void my_log_handler(const gchar *log_domain,
             GLogLevelFlags log_level,
             const gchar *message,
             gpointer user_data){
				 puts(message);
			 }


int main(int argc, char *argv[]) {
	int ret;

	g_log_set_handler (NULL, GLogLevelFlags(G_LOG_LEVEL_DEBUG | G_LOG_LEVEL_WARNING | G_LOG_FLAG_FATAL
                   			| G_LOG_FLAG_RECURSION), my_log_handler, NULL);


	const char* adapter_name=NULL;		
	char uuid_str[MAX_LEN_UUID_STR];	
	gattlib_string_to_uuid("90ad0001-662b-4504-b840-0ff1dd28d84e", MAX_LEN_UUID_STR, &service_uuid);

	if(argc==2){
		device_address = argv[1];
	}
	else if(argc==3){
		adapter_name = argv[1];
		device_address = argv[2];
	} else  {
		printf("%s <device_address>\n", argv[0]);
		printf("%s <adapter_name> <device_address>\n", argv[0]);
		return 1;
	}

	ret = gattlib_adapter_open(adapter_name, &adapter);
	if (ret) {
		fprintf(stderr, "ERROR: Failed to open adapter %s.\n", adapter_name);
		return 1;
	}

	g_timeout_add(1, monitorConnection, NULL);
		
	// Catch CTRL-C
	signal(SIGINT, on_user_abort);

	std::thread srv(socket_server);

	m_main_loop = g_main_loop_new(NULL, 0);
	g_main_loop_run(m_main_loop);

	run_server = false;
	srv.join();
	
	deActivateConnection();

	g_main_loop_unref(m_main_loop);
	puts("Done");
	return ret;
}
