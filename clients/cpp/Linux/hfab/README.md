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