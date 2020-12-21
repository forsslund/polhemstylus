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

const READ_INTERVAL_LOW = 1000;  // ms
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
var updatePossitionIntervalID = null;  // Handle to intervall callback
var readModeHi = true;
function updatePossition()
 {
  if(start){
//    // Need firmware 2v02. Might be good to only sample stylus pos while 
//    // connected. And only update on changed value.+
//    if( NRF.getSecurityStatus()=={connected:false} )
//    {
//      LED1.write(false);
//    }
//    else{
//      // Blink for "alive"
//      on = !on;
//      LED1.write(on);
//    }
  
    var preA=a;
    var preB=b;
    a = getRotation();
    b = getButtonState();
    
    // Only send data if new information
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
      
      // Ensure readModeHi = true
      if( !readModeHi && updatePossitionIntervalID != null )
      {        
        clearInterval(updatePossitionIntervalID);
        updatePossitionIntervalID = setInterval(updatePossition, READ_INTERVAL_HI);     
        readModeHi = true;        
      } 
    }
    else{
      inactiveCounter++;
      if( inactiveCounter > INACTIVE_THRESHOLD ){
        inactiveCounter = INACTIVE_THRESHOLD; // Avoid counter owerflow
        // Reduce read interval (readModeHi = false)
        if( readModeHi && updatePossitionIntervalID != null )
        {          
          clearInterval(updatePossitionIntervalID);
          updatePossitionIntervalID = setInterval(updatePossition, READ_INTERVAL_LOW);     
          readModeHi = false;           
        }        
      }      
    }
  }
}

updatePossitionIntervalID = setInterval(updatePossition, READ_INTERVAL_HI);


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
