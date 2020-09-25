#include "pch.h"
#include "BLEdeviceFinder.h"
using namespace winrt;
using namespace Windows::Foundation;

BLEdeviceFinder* BLEdeviceFinder::instance = 0;

int main()
{
    init_apartment();

    BLEdeviceFinder* pBleFinder = pBleFinder->getInstance();
    printf("Enumerate BLE devices.\n");
    pBleFinder->Enumerate();
    pBleFinder->ListDevices();
    
    
}

