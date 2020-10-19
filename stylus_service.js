var a=1;
var b=0;
var start=true;
let StylusData = new Uint8Array(2);

NRF.setServices({
  "3e440001-f5bb-357d-719d-179272e4d4d9": {
    "3e440002-f5bb-357d-719d-179272e4d4d9": {
      notify: true,
      //description: "",
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
NRF.setAdvertising({"3e440001-f5bb-357d-719d-179272e4d4d9":"Stylus value"}, {name:"Stylus"});

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
      "3e440001-f5bb-357d-719d-179272e4d4d9": {
        "3e440002-f5bb-357d-719d-179272e4d4d9": {
          value: StylusData,
          notify: true
        }
      }
    });
  }
},20);