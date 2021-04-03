#include <assert.h>
#include <stdio.h>
#include <unistd.h>	// usleep
#include <glib.h>	// Gattlib internal messaging requires glibmain to run

#include "SocketServer.h"

extern "C" {
#include "gattlib.h"
}

static GMainLoop *hMainloop = NULL;		// glib mainloop handle
gatt_connection_t* gattConnection = NULL;

void* bleAdapter=NULL;
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
	}
}

bool deActivateConnection(){
		if(gattConnection==NULL){
			printf("\nNo existing connection\n");
			return false;
		}

		// Clean up old connection
		gattlib_notification_stop(gattConnection, &service_uuid);
		gattlib_disconnect(gattConnection);
		gattConnection=NULL;		
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
	if(gattConnection!=NULL){
		printf("\nExisting connection, skipping.");
		return;
	}

	printf("Connecting to Stylus...");	
	gattConnection = gattlib_connect(bleAdapter, device_address, GATTLIB_CONNECTION_OPTIONS_LEGACY_DEFAULT);
	if (gattConnection == NULL) {
		fprintf(stderr, "Fail to connect to the bluetooth device.\n");
		return;
	}
	
	gattlib_register_on_disconnect(gattConnection, disconnected_handler, NULL);
	gattlib_register_notification(gattConnection, notification_handler, NULL);
	
	int ret = gattlib_notification_start(gattConnection, &service_uuid);
	if (ret) {
		fprintf(stderr, "Fail gattlib_notification_start.\n");
		gattlib_disconnect(gattConnection);						
		return;
	}

	printf("...stylus connected.\n");
}

bool doQuit=false;
static void on_user_abort(int arg) {
	printf("\nDeactivated BLE...");	
	doQuit = true;
	deActivateConnection();
	printf("shuting down socket server...");
	server.Shutdown();	
	printf("exiting.\n");
	g_main_loop_quit(hMainloop);
}

// If active client, sets up BLE connection, else discconnects to let BLE to save power
gboolean monitorConnection(gpointer user_data){
	if( server.HasActiveClient() && !doQuit ){		
		if(gattConnection==NULL)
		{
			activateConnection();
		}			
	}
	else if(gattConnection != NULL){		
		disconnected_handler(NULL);
		printf("Styrlus disconnected.\n");
	}	
	return true;	
};

int main(int argc, char *argv[]) {	
	const char* bleAdapter_name=NULL;
	std::string url;
	char uuid_str[MAX_LEN_UUID_STR];
	gattlib_string_to_uuid("90ad0001-662b-4504-b840-0ff1dd28d84e", MAX_LEN_UUID_STR, &service_uuid);

	if(argc==3){
		device_address = argv[1];
		url = argv[2];
	}
	else if(argc==4){
		bleAdapter_name = argv[1];
		device_address = argv[2];		
		url = argv[3];
	} else  {
		printf("%s <deviceAddress> <socketPath>\n", argv[0]);
		printf("%s <BluetoothAdapterName> <deviceAddress> <socketPath>\n", argv[0]);
		printf("Example \n\t%s hci0 C5:61:54:49:57:92 /tmp/stylus1\n", argv[0]);
		return 1;		
	}

	if(bleAdapter_name!=NULL){
		printf("Bluetooth dapter: %s\n", bleAdapter_name);
	}
	printf("Device address: %s\n", device_address);
	printf("Socket path: %s\n", url.c_str());
	
	if ( gattlib_adapter_open(bleAdapter_name, &bleAdapter) !=0 ) {
		fprintf(stderr, "ERROR: Failed to open Bluetooth adapter %s.\n", bleAdapter_name);
		return 1;
	}
	
	// Start the socket server
	server.Start(url);	
	printf("Press ctrl-c to quit.");	
	//
	// Register event and startup glib message loop
	//
	signal(SIGINT, on_user_abort); 				// Catch CTRL-C, closes connections and cleans up, then stopps mainloop
	g_timeout_add(1, monitorConnection, NULL);	// This is our task!
	hMainloop = g_main_loop_new(NULL, 0);
	g_main_loop_run(hMainloop);					// Blocking untill mainloop stopped (by on_user_abort)
	
	return 0;
}
