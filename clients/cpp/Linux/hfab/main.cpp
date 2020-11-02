

#ifdef _WIN32
    #define WINDOWS
#else
    #define LINUX
#endif

#include <iostream>
#include "uhaptikfabriken.h"
#include <string>
#include <regex>

// ----- Portable keyboard loop break ------------------------------------------
#ifdef WINDOWS
#include <conio.h>
#else
#include <stdio.h>
#include <sys/select.h>
#include <sys/ioctl.h>
#include <termios.h>
int _kbhit(){
    static const int STDIN = 0;
    static bool initialized = false;
    if (!initialized){
        // Use termios to turn off line buffering
        termios term;
        tcgetattr(STDIN, &term);
        term.c_lflag &= ~ICANON;
        tcsetattr(STDIN, TCSANOW, &term);
        setbuf(stdin, NULL);
        initialized = true;
    }
    int bytesWaiting;
    ioctl(STDIN, FIONREAD, &bytesWaiting);
    return bytesWaiting;
}
#endif
// -----------------------------------------------------------------------------


int main()
{

    using namespace haptikfabriken;

    HaptikfabrikenInterface hfab;
    hfab.findUSBSerialDevices();
    //hfab.serialport_name = "/dev/ttyACM0";
    std::cout << "Opening " << hfab.serialport_name << "\n";
    hfab.open();

    int printcount=0;
    while(!_kbhit()){
        fsVec3d p = hfab.getPos();
        fsRot r = hfab.getRot();
        if(++printcount%10000==0){
            //std::cout << toString(p) << " Main speed:" << hfab.getMsg().tD << " Escon speed:" << hfab.getMsg().tE << "\n";
            std::string abc = hfab.getMsg().toString();
            abc = std::regex_replace(abc,std::regex("\n"),"R");

            //std::cout << "[012345678901234567890123456789012345678901234567890123456789123]\n";
            //std::cout << "[" << abc << "]\n";
            position_hid_to_pc_message m = hfab.getMsg();
            //std::cout << m.tA;
            std::cout << toString(p) << " " << m.tA << " " << m.lambda << " " << m.tD << " " << m.tE << "\n";
            return 0;

        }

        fsVec3d f = -200*p;
        hfab.setForce(f);
    }

    hfab.close();

    return 0;
}
