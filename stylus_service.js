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

// Change the name that's advertised
NRF.setAdvertising({}, {name:"Stylus"});

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

setInterval(function() {
  if(start){
    // Need firmware 2v02. Might be good to only sample stylus pos while 
    // connected. And only update on changed value.+
    if( NRF.getSecurityStatus()=={connected:false} )
    {
      LED2.set();
    }
    else{
      LED2.reset();
    }
    
    
    
    a++;
    b=!b;
    if(a>1024) a=0;
    
    a = getRotation();
    b = getButtonState();
    
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
},20);

// Start after 1 s to let other things start first....
setTimeout(function() {
  spi = new SPI(); // Software spi since we need nonstandard bit length (11)
  spi.setup({mosi:pin_dummy, miso:pin_green_to_sensor_data, sck:pin_white_to_sensor_clk, mode:2, bits:11});
},1000);

pinMode(pin_button, 'input_pullup');
var b2 = function(state){
  //LED2.write(!state.state);
};
setWatch(b2, pin_button, {repeat:true, edge:"both", debounce:"10"});

// Blink for "alive"
var on = false;
setInterval(function() {
  on = !on;
  LED1.write(on);
}, 1000);
