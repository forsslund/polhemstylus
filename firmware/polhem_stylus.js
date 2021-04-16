// Usage: paste the full content into right side panel of:
// https://www.espruino.com/ide/
// Power up and click connect in left top corner.
// Click "Send to Espruino" (middle panel button)
// Green led lights up when button pressed
// Counter should be written in left side terminal (after 1s)

// Pinout, see figure https://www.espruino.com/MDBT42Q
const pin_green_to_sensor_data = D25;
const pin_button = D31;
const pin_white_to_sensor_clk = D27;
const pin_dummy = D15; // Some unused pin

// As reference, the pinout of the back is, from left-to-right
// TX, RX, D7 (unused), Vin, GND
// Apply 3.3-16V to Vin and GND to GND to power it up without battery
// Suggested battery:
// https://www.kjell.com/se/produkter/el-verktyg/batterier/litiumbatterier/14500-li-ion-batteri-37-v-800-mah-p32061

// 5 Minute button timeout to enter energy saving mode
var energySavingMode = false;
let energyTimeout;
let spi;
let bluetoothInterval;

var sleep = function(){
    energySavingMode=true;
    clearInterval(bluetoothInterval);
};

var wakeup = function(){
  bluetoothInterval = setInterval(function() {
    var a=0;var b=1;var c=2; var limit=10;
    while( (a!=b || a!=c) && limit ){
      a=spi.send(0);
      b=spi.send(0);
      c=spi.send(0);
      limit--;
    }
    var d=((a&1023) + 1023-326)%1023;
    var btn = 10000*(!digitalRead(pin_button));

    Bluetooth.println((d+btn).toString());
  }, 16);
};

// Button light led2 code
global.LED2=D2;
LED2.write(false);
pinMode(pin_button, 'input_pullup');
var b2 = function(state){
  LED2.write(!state.state);

  clearTimeout(energyTimeout);
  setTimeout(sleep, 60000);

  if(energySavingMode) wakeup();
};
setWatch(b2, pin_button, {repeat:true, edge:"both", debounce:"10"}); 

// Blink for "alive"
//var on = false;
//setInterval(function() {
//  on = !on;
//  LED1.write(on);
//}, 1000);


setTimeout(function() {
  spi = new SPI(); // Software spi since we need nonstandard bit length (11)
  spi.setup({mosi:pin_dummy, miso:pin_green_to_sensor_data, sck:pin_white_to_sensor_clk, mode:2, bits:11});

  energyTimeout = setTimeout(sleep, 60000);
  wakeup();

},1000);

