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
const READ_INTERVAL_HI  = 20;    // ms
const INACTIVE_THRESHOLD= 1500;  // number of READ_INTERVAL_HI before going to low activity mode

let StylusData = new Uint8Array(2);
let spi = false;
var MODE_SLEEP=true;  // Initiate to true so that BLE will be initated at first update
var timeoutToken;

function activateBLE(){
  MODE_SLEEP=false;
  
  // Change the name that's adveprtised
  NRF.wake();
  NRF.setAdvertising({}, {name:"Polhem Stylus"});
  
  NRF.setServices({
    "90ad0000-662b-4504-b840-0ff1dd28d84e": {
      "90ad0001-662b-4504-b840-0ff1dd28d84e": {
        notify: true,
        value : StylusData,
        maxLen : 2,
        readable : true,
        writable : true,

        onWrite : function(evt) {
          //start=true;
        }
      }
    }
  }, { uart : true });
}

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

var btn=false;
function getButtonState(){
  return btn;
  //return !digitalRead(pin_button);
}

var buttonActivity = function(state){
  LED2.write(state.state);  
  btn=state.state;
  updatePossition();
};

// Watch button one
setWatch(buttonActivity, BTN1, {repeat:true, edge:"both", debounce:"10"});

// Watch button two
pinMode(pin_button, 'input_pullup');
setWatch(buttonActivity, pin_button, {repeat:true, edge:"both", debounce:"10"});

//var on = false;
var inactiveCounter = 0;               // Keeps track of activity
var readModeHi = true;

var updateInterval=READ_INTERVAL_HI;
var preRotation = 1;
var preButton = 0;
function updatePossition()
{
  if(MODE_SLEEP) activateBLE();

  var rotation=preRotation;
  var button=preButton;
  // Only sample possition if connected
  if( NRF.getSecurityStatus().connected==true )
  {
    rotation = getRotation();
  }
  button = getButtonState();

  // Only send data if new information
  // else count up inactive counter
  if( rotation!=preRotation || button!=preButton )
  {
    updateInterval = READ_INTERVAL_HI;
    preRotation=rotation;
    preButton=button;
    // Activity
    inactiveCounter = 0;

    // Pack message
    StylusData[0] = rotation>>8;
    StylusData[1] = rotation - 256*StylusData[0];
    if(button) StylusData[0] = StylusData[0] + 128;

    // Send data
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
    // No activity
    inactiveCounter++;
    if( inactiveCounter > INACTIVE_THRESHOLD ){
      inactiveCounter = INACTIVE_THRESHOLD; // Avoid counter owerflow
      // Reduced read interval
      if( NRF.getSecurityStatus().connected==true ){
          updateInterval = READ_INTERVAL_LOW;
      }
      else{
        // No activity and no connection, 'sleep' (inactivate BLE)
        // Should not need to do this, according to the below statement!
        //
        // nRF52 devices are used in newer Espruino products. On these, Espruino is able to enter low power
        // sleep modes automatically. Out of the box, power consumption is around 20uA (0.02mA) or over a 
        // year on a CR2032 battery.
          NRF.sleep();
          MODE_SLEEP=true;
      }
    }
  }
  if( !MODE_SLEEP ){
    clearTimeout(timeoutToken);  // Make sure we only have one active timeout
    timeoutToken = setTimeout(updatePossition, updateInterval);
  }
  //LED1.write(MODE_SLEEP);
}

timeoutToken = setTimeout(updatePossition, READ_INTERVAL_HI); // Initial call

//
// Is this the power hog? Should we disable SPI when not connected?
//
// Start after 1 s to let other things start first....
setTimeout(function() {
  spi = new SPI(); // Software spi since we need nonstandard bit length (11)
  spi.setup({mosi:pin_dummy, miso:pin_green_to_sensor_data, sck:pin_white_to_sensor_clk, mode:2, bits:11});
},1000);