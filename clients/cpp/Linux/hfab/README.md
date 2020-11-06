Linux:
sh make.sh
./main

to measure: 
time ./main



Windows:
open "Developer Command Prompt for VS 2019" on start menu (start typing to find)
cd [this folder]
cl main.cpp 
main.exe

to measure:  start "PowerShell", go to folder and run: 
Measure-Command {.\main.exe}




sudo cp 49-teensy.rules /etc/udev/rules.d/
tar -xf arduino-1.8.13-linux64.tar.xz 
cd arduino-1.8.13/
sudo ./install.sh
cd ..
chmod 755 TeensyduinoInstall.linux64
./TeensyduinoInstall.linux64

In arduino select Tools->Board->Teensduino->Teensy 4.0




Latency test:
https://forum.pjrc.com/threads/54711-Teensy-4-0-First-Beta-Test/page108
pjrc_latency_test.zip
