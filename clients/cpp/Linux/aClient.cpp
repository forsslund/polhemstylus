	#include <sys/un.h>
	#include <unistd.h>
#include "SocketClient.h"

int main(int argc, char* argv[])
{
    SocketClient<uint16_t> myClient;
    myClient.Start("kalle");
    
    uint16_t value = 0;
    for(int i=0;i<1000;i++){
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
