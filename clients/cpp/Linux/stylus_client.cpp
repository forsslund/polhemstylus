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
#include <glib.h>	// Gattlib internal messaging requires glibmain to run
//#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

#include <thread>

#include "socketServer.h"
#include <unistd.h>
//-----------------------------------

extern "C" {
#include "gattlib.h"
}

static GMainLoop *m_main_loop;
gatt_connection_t* g_connection;

void* adapter=NULL;
const char* device_address=NULL;
uuid_t service_uuid;

SocketServer server;

void Sleep(int ms){
    usleep(ms * 1000);
}

void notification_handler(const uuid_t* uuid, const uint8_t* data, size_t data_length, void* user_data) {	
	uint16_t stylusData;
	if( data_length == sizeof(stylusData) ){
		stylusData = *((uint16_t*) &data[0]);
		server.Send( stylusData );	
		//printf("sending %d",stylusData);
	}
	printf("Gotti %d bytes, d[0]=%d d[1]=%d\n", int(data_length), data[0], data[1]);	
	//printf("got %d bytes", uint16_t( data_length ));
}

bool deActivateConnection(){
		if(g_connection==NULL){
			printf("\nNo existing connection\n");
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
		//g_log(NULL, G_LOG_LEVEL_DEBUG, "Disconnected %i",nrDisconnects);
	//	printf("Disconnected from Stylus.\n");
		deActivateConnection();	
}


void activateConnection(){
	printf("activateConnection()\n");
		if(g_connection!=NULL){
			printf("\nExisting connection, skipping.");
			return;
		}
		
		g_connection = gattlib_connect(adapter, device_address, GATTLIB_CONNECTION_OPTIONS_LEGACY_DEFAULT);
		if (g_connection == NULL) {
			fprintf(stderr, "Fail to connect to the bluetooth device.\n");
			return;
		}
		
		gattlib_register_on_disconnect(g_connection, disconnected_handler, NULL);

		gattlib_register_notification(g_connection, notification_handler, NULL);

		//gattlib_uuid_to_string(&service_uuid, uuid_str, sizeof(uuid_str));		
		int ret = gattlib_notification_start(g_connection, &service_uuid);
		if (ret) {
			fprintf(stderr, "Fail gattlib_notification_start.\n");
			gattlib_disconnect(g_connection);						
			return;
		}

		printf("Done\n");
}

bool doQuit=false;
static void on_user_abort(int arg) {
	printf("on_user_abort\n");
	doQuit = true;
	deActivateConnection();
	printf("deactivated BLE\n");
	server.Shutdown();
	printf("socket server shut down\n");
	g_main_loop_quit(m_main_loop);
}

// If active client, sets up BLE connection, else discconnects to let BLE to save power
gboolean monitorConnection(gpointer user_data){
		if( server.HasActiveClient() && !doQuit ){		
			if(g_connection==NULL)
			{
				activateConnection();
			}			
		}
		else if(g_connection != NULL){		
			disconnected_handler(NULL);
			printf("disconnect");
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
		printf("%s hci<adapter_nr> <device_address>\n", argv[0]);
		return 1;		
	}

	printf("\nDevice address: %s\n",device_address);	

	ret = gattlib_adapter_open(adapter_name, &adapter);
	if (ret) {
		fprintf(stderr, "ERROR: Failed to open adapter %s.\n", adapter_name);
		return 1;
	}
	
	server.Start(url);
	printf("\nMonitor connection?\n");
	
	
	//
	// Set up glib stuf
	//
	g_log_set_handler (NULL, GLogLevelFlags(G_LOG_LEVEL_DEBUG | G_LOG_LEVEL_WARNING | G_LOG_FLAG_FATAL
                   			| G_LOG_FLAG_RECURSION), my_log_handler, NULL);
	// Catch CTRL-C
	signal(SIGINT, on_user_abort);

	g_timeout_add(1, monitorConnection, NULL);	// This is our task!
	m_main_loop = g_main_loop_new(NULL, 0);
	g_main_loop_run(m_main_loop);

	on_user_abort(0);	// Cleanupcode in here, make sure it is allways run
	
	return ret;
}
