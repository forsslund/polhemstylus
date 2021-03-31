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

#include "socketServer.h"

#define SV_SOCK_PATH "/tmp/ud_ucase"

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

static GMainLoop *m_main_loop;

uint8_t bt_data[] = {0,0};
uint8_t prev_bt_data[] = {0,0};

gatt_connection_t* g_connection;
void* adapter=NULL;
const char* device_address=NULL;
uuid_t service_uuid;

SocketServer server;


void notification_handler(const uuid_t* uuid, const uint8_t* data, size_t data_length, void* user_data) {
	static uint16_t gnu=0;
	uint16_t stylusData;
	if( data_length == sizeof(stylusData) ){
		stylusData = *((uint16_t*) &data[0]);
	}	
	server.Send(gnu++);	
	//server.Send(stylusData);	
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

// If active client, sets up BLE connection, else discconnects to let BLE to save power
gboolean monitorConnection(gpointer user_data){
	if( server.HasActiveClient() ){
		printf("HasActiveClient\n");
		if(g_connection==NULL)
		{
			activateConnection();			
		}
	}
	else if(g_connection != NULL){		
		disconnected_handler(NULL);
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
	//std::string url = "/tmp/stylus-ttyACM0";
	//std::string url = "/dev/stylus1";
	std::string url = "kalle";
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

	printf("\nDevice address: %s\n",device_address);	

	ret = gattlib_adapter_open(adapter_name, &adapter);
	if (ret) {
		fprintf(stderr, "ERROR: Failed to open adapter %s.\n", adapter_name);
		return 1;
	}
	
	server.Start(url);
	printf("Monitor connection?\n");
	g_timeout_add(1, monitorConnection, NULL);
		
	// Catch CTRL-C
	signal(SIGINT, on_user_abort);	

	m_main_loop = g_main_loop_new(NULL, 0);
	g_main_loop_run(m_main_loop);
	
	deActivateConnection();
	server.Shutdown();


	g_main_loop_unref(m_main_loop);
	puts("Done");
	return ret;
}
