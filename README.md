# polhemstylus

2021-05-25

Note Windows: If you get "not a polhem device" error it can be that Windows has paired the device. 
Go to "Bluetooth settings" and if you see Polhem there, click "Remove device".





NOTE 2021 EDITON: it is stylus_service.js That is running on POLHEM!

simple.js is a simulator just counting up and sending
polhem_stylus.js is the old version running on polhem

Clients (init the connection and,) read the bluetooth data



Hardware design as of 2020-12-15:

Looking from service hole upwards and open:
Sensor cables: red, blue, white, green 
                3v   gnd  clock  data 
                           D27    D25
Button: two pins closest to sensor are same,
        two pins further away from sensore are same,
        when button is pressed they are closed together

Button pin D31 uses internal pull-up, should be grounded when button is pressed.

RBWG
||||

.B       .continue to SpringPCB using blue wire

.Purple   .NC

SpringPCB-continue to GND with blue (or yellow) wire 
Spring is soldered to SpringPCB, which is glued to back/pocket with Loctite 3450

Main PCB:
Insert 5 pin connector in base

5-pin connector looking from back, funny shape up:
Left: Tx, Rx, D7, V+, GND

Finally we have:
Red          3.3v 
Blue/yellow  GND
White        D27
Green        D25
Purple       D31
 


