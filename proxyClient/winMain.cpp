// unixsocket1.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>


#ifdef _WIN64

 //   #include <winrt/Windows.Foundation.h>
 // #include <winrt/Windows.Foundation.Collections.h>
   #include "SocketClient.h"

   // #include <winsock2.h>    
    //#include <afunix.h>
    #pragma comment(lib,"ws2_32.lib")
#else
    #include <sys/un.h>
    #include <sys/socket.h>
    //#include <ctype.h>
    //#include "tlpi_hdr.h"
    #include <unistd.h>     /* Prototypes for many system calls */
#endif 

//#include <sys/types.h>  /* Type definitions used by many programs */
#include <stdio.h>      /* Standard I/O functions */


int main(int argc, char* argv[])
{
    SocketClient<uint16_t> myClient;
    myClient.Start("/dev/stylus1");
    
    uint16_t value = 0;
    for(int i=0;i<100;i++){
        while (value == myClient.Get()) {
            Sleep(1);
        }
        value = myClient.Get();
        char *pValue = (char *) &value;
        int rotation = (0x03 & pValue[0]) * 256 + pValue[1];
        printf("%x rotation=%d\n", value, rotation);
    }
    return 0;
}
