var a=1;
var b=0;
var start=true;
let StylusData = new Uint8Array(2);

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

setInterval(function() {
  if(start){
    // Need firmware 2v02. Might be good to only sample stylus pos while 
    // connected. And only update on changed value.+
    //if( NRF.getSecurityStatus()=={connected:false} )
    //{
    //  LED1.set();
    //}
    //else{
    //  LED1.reset();
    //}
    
    
    a++;
    b=!b;
    if(a>1024) a=0;
    
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