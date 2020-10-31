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
//#include <glib.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

#include "gattlib.h"

// ----- Portable keyboard loop break ------------------------------------------
#ifdef WINDOWS
#include <conio.h>
#else
#include <stdio.h>
#include <sys/select.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <stropts.h>
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




// Battery Level UUID
const uuid_t g_battery_level_uuid = CREATE_UUID16(0x2A19);

//static GMainLoop *m_main_loop;

uint8_t bt_data[] = {0,0};
uint8_t prev_bt_data[] = {0,0};

void notification_handler(const uuid_t* uuid, const uint8_t* data, size_t data_length, void* user_data) {
	int i;

	printf("Notification Handler: ");

	for (i = 0; i < data_length; i++) {
		printf("%02x ", data[i]);
		if(i<2)
			bt_data[i]=data[i];
	}
	printf("\n");
}

/*
static void on_user_abort(int arg) {
	g_main_loop_quit(m_main_loop);
}
*/

int main(int argc, char *argv[]) {
	int ret;
	gatt_connection_t* connection;

	const char* adapter_name=NULL;
	void* adapter;
	const char* device_address;
	char uuid_str[MAX_LEN_UUID_STR];
	uuid_t service_uuid;
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

	connection = gattlib_connect(adapter, device_address, GATTLIB_CONNECTION_OPTIONS_LEGACY_DEFAULT);
	if (connection == NULL) {
		fprintf(stderr, "Fail to connect to the bluetooth device.\n");
		return 1;
	}

	gattlib_register_notification(connection, notification_handler, NULL);
	gattlib_uuid_to_string(&service_uuid, uuid_str, sizeof(uuid_str));
	puts(uuid_str);
	ret = gattlib_notification_start(connection, &service_uuid);
	if (ret) {
		fprintf(stderr, "Fail to start notification.\n");
		goto DISCONNECT;
	}

	// Catch CTRL-C
	//signal(SIGINT, on_user_abort);

	//m_main_loop = g_main_loop_new(NULL, 0);
	//g_main_loop_run(m_main_loop);
	printf("Press any key to quit...\n");
    while(!_kbhit()){
		if(prev_bt_data[0]!=bt_data[0] || prev_bt_data[1]!=bt_data[1] ){
			   prev_bt_data[0]=bt_data[0];
			   prev_bt_data[1]=bt_data[1];

			   printf("Got new data in main loop: %02x %02x\n",bt_data[0],bt_data[1]);
		   }
		// Wait for socket message here
		// if message, then reply prev_bt_data
	}

	// In case we quit the main loop, clean the connection
	gattlib_notification_stop(connection, &service_uuid);
	//g_main_loop_unref(m_main_loop);

DISCONNECT:
	gattlib_disconnect(connection);
	puts("Done");
	return ret;
}
