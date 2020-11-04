

#ifdef _WIN32
    #define WINDOWS
#else
    #define LINUX
#endif

#include <iostream>
#include "uhaptikfabriken.h"
#include <string>
#include <regex>


#include <chrono>
#include <thread>

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
    int max=1000;
    while(!_kbhit() && max){
        max--;
        fsVec3d p = hfab.getPos();
        //fsRot r = hfab.getRot();
        if(++printcount%100==0){
            //std::cout << toString(p) << " Main speed:" << hfab.getMsg().tD << " Escon speed:" << hfab.getMsg().tE << "\n";
            //std::string abc = hfab.getMsg().toString();
            //abc = std::regex_replace(abc,std::regex("\n"),"R");

            //std::cout << "[1234567890123456789012345678901234567890123456789012345678901234] = LENGTH\n";
            //std::cout << "[" << abc << "]\n";
            position_hid_to_pc_message m = hfab.getMsg();
            //std::cout << m.tA;
            std::cout << toString(p*1000) << " mm " << m.tA << " " << m.lambda << " " << m.tD << " " << m.tE << "\n";
            //return 0;

        }

        fsVec3d f = fsVec3d(0,0,0);//-100*p;
        using namespace std::chrono_literals;
        //std::this_thread::sleep_for(1000us);
        hfab.setForce(f);
        //std::this_thread::sleep_for(100ms);
    }

    hfab.close();

    return 0;
}
