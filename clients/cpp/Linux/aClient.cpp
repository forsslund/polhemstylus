#include <sys/un.h>
#include <unistd.h>
#include "SocketClient.h"

int main(int argc, char* argv[])
{
    std::string socketPath;
    if(argc!=2){
        printf("Prints 1000 diffrent values recieved on socketPath.");
        printf("\nUsage:\n%s <socketPath>\n", argv[0]);
        return 1;
    }
    socketPath=argv[1];

    SocketClient<uint16_t> myClient;
    myClient.Start(socketPath);
    
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
    printf("shutting down\n");
    myClient.Shutdown();
    return 0;
}
