// Usage: paste the full content into right side panel of:
// https://www.espruino.com/ide/
// Power up and click connect in left top corner.
// Click "Send to Espruino" (middle panel button) USING FLASH MODE
// Do not worry about "Can't update services until BLE restart"
// Reboot device (off/on power)
// Red light flash every second when alive
// Green led lights up when button pressed

// Pinout, see figure https://www.espruino.com/MDBT42Q
const pin_green_to_sensor_data = D25;
const pin_button = D31;
const pin_white_to_sensor_clk = D27;
const pin_dummy = D15; // Some unused pin

const READ_INTERVAL_LOW = 1000;  // ms OBS device will not go to sleep if there is a timer that
                                 // will end within 1500 ms (according to 
                                 // https://www.espruino.com/Power+Consumption,seems incorrect...)
const READ_INTERVAL_DEEP_SLEEP = 10000;  // ms
const READ_INTERVAL_HI  = 20;    // ms
const INACTIVE_THRESHOLD= 1000;  // number of READ_INTERVAL_HI before going to low activity mode

var a=1;
var b=0;
var start=true;
let StylusData = new Uint8Array(2);
let spi = false;

NRF.setServices({
  "90ad0000-662b-4504-b840-0ff1dd28d84e": {
    "90ad0001-662b-4504-b840-0ff1dd28d84e": {
      notify: true,
      value : StylusData,
      maxLen : 2,
      readable : true,
      writable : true,

      onWrite : function(evt) {
        start=true;
      }
    }
  }
}, { uart : true });

// Change the name that's adveprtised
NRF.setAdvertising({}, {name:"Polhem Stylus"});

getRotation = function() {
  let d=0;
  if(spi){
    var a=0;var b=1;var c=2; var limit=10;
    while( (a!=b || a!=c) && limit ){
      a=spi.send(0);
      b=spi.send(0);
      c=spi.send(0);
      limit--;
    }
    d=((a&1023) + 1023-326)%1023;
  }
  return d;
};

getButtonState = function() {
  return !digitalRead(pin_button);
};

//var on = false;
var inactiveCounter = 0;               // Keeps track of activity
var readModeHi = true;
function updatePossition()
{
  var updateInterval=READ_INTERVAL_HI;
  if(start){
    
    var preA=a;
    var preB=b;        
    // Only sample possition if connected
    if( NRF.getSecurityStatus().connected==true )
    {
      a = getRotation();
    }
    b = getButtonState();

    // Only send data if new information
    // else count up inactive counter
    if( a!=preA || b!=preB )
    {      
      inactiveCounter = 0;
      
      // Pack message
      StylusData[0] = a>>8;
      StylusData[1] = a - 256*StylusData[0];

      if(b) StylusData[0] = StylusData[0] + 128;

      NRF.updateServices({
        "90ad0000-662b-4504-b840-0ff1dd28d84e": {
          "90ad0001-662b-4504-b840-0ff1dd28d84e": {
            value: StylusData,
            notify: true
          }
        }
      });
    }
    else{
      inactiveCounter++;
      if( inactiveCounter > INACTIVE_THRESHOLD ){
        inactiveCounter = INACTIVE_THRESHOLD; // Avoid counter owerflow
        // Reduced read interval
        if( NRF.getSecurityStatus().connected==true ){
            updateInterval = READ_INTERVAL_LOW;
        }
        else{
            updateInterval = READ_INTERVAL_DEEP_SLEEP;
        }
      }
    }
  }
  setTimeout(updatePossition, updateInterval);
}

setTimeout(updatePossition, READ_INTERVAL_HI); // Initial call


// Start after 1 s to let other things start first....
setTimeout(function() {
  spi = new SPI(); // Software spi since we need nonstandard bit length (11)
  spi.setup({mosi:pin_dummy, miso:pin_green_to_sensor_data, sck:pin_white_to_sensor_clk, mode:2, bits:11});
},1000);

pinMode(pin_button, 'input_pullup');
var b2 = function(state){
  LED2.write(!state.state);
};

setWatch(b2, pin_button, {repeat:true, edge:"both", debounce:"10"});
setSleepIndicator(LED1);
//setDeepSleep(1);