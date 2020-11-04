/*
    Polhem high speed

    Support both standard encoder/setcurrent messages (default, command=0)
    and position/setforce message (command=2)

    Real device:                   Emulator:
      SSI (HW) is used for ENC4        Normal encoder
      Final axis (enc3) disabled       Normal encoder
*/

// -------- SETTINGS -----------------------------------------------
#define DIRECT_COMPUTE      // Polhem direct computations
//#define USE_DISPLAY       // PiOled (on emulator only currently)
//#define SERIAL_BRIDGE
constexpr int escon_max_milliamps = 6000;
constexpr bool AMPS_INFO_INSTEAD_OF_GIMBAL = false;
// -----------------------------------------------------------------



// -----------------------------------------------------------------
#include <TeensyThreads.h>
#include <Bounce2.h>
#include<SPI.h>

#ifdef DIRECT_COMPUTE
#include "haptikfabrikenapi.h"
using namespace haptikfabriken;
Kinematics kinematics(Kinematics::configuration::polhem_v3());
#endif
unsigned __exidx_start;
unsigned __exidx_end;

#ifdef USE_DISPLAY
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels
// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire1, OLED_RESET);
#endif



// -------- COMPUTER MESSAGE INTERFACE -------------
#pragma pack(push)  // Keep these structures unoptimized (packed as is, not padded)
#pragma pack(1)     // Since we are transferring them as is.
struct hid_to_pc_message { // 7*2 = 14 bytes
  short encoder_a;
  short encoder_b;
  short encoder_c;
  short encoder_d;
  short encoder_e;
  short encoder_f;
  short info;
  int toChars(char *c) {
    int n = sprintf(c, "%hd %hd %hd %hd %hd %hd %hd", encoder_a, encoder_b, encoder_c,
                   encoder_d, encoder_e, encoder_f, info);
    while(n<63) c[n++] = ' ';
    c[63]='\n';
    return 64;
  }
  void fromChars(const char *c) {
    sscanf(c, "%hd %hd %hd %hd %hd %hd %hd", &encoder_a, &encoder_b, &encoder_c,
           &encoder_d, &encoder_e, &encoder_f, &info);
  }
};

struct position_hid_to_pc_message {
    float x_mm;
    float y_mm;
    float z_mm;
    short a_ma;
    short b_ma;
    short c_ma;
    short info;
    float tA;
    float lambda;
    float tD;
    float tE;
    int toChars(char *c){
      // 012345678901234567890123456789012345678901234567890123456789123
      // -234.231 -234.121 -234.123 -1.2324 -1.1232 -1.1242 -1.2324 1
        int n = sprintf(c, "% 8.3f % 8.3f % 8.3f % 7.4f % 7.4f % 7.4f % 7.4f %hd", x_mm, y_mm, z_mm, 
           AMPS_INFO_INSTEAD_OF_GIMBAL ? a_ma*0.001 : tA, 
           AMPS_INFO_INSTEAD_OF_GIMBAL ? b_ma*0.001 : lambda, 
           AMPS_INFO_INSTEAD_OF_GIMBAL ? c_ma*0.001 : tD, 
           tE, info);
        while(n<63) c[n++] = ' ';
        c[62]='\n';
        c[63]='\0';
        return 63;        
    }
    void fromChars(const char *c){
        sscanf(c, "%f %f %f %f %f %f %f %hd", &x_mm, &y_mm, &z_mm, &tA, &lambda, &tD, &tE, &info);
    }
};

struct pc_to_hid_message {  // 7*2 = 14 bytes + 1 inital byte always 0
  unsigned char reportid = 0;
  short current_motor_a_mA;   // or force x in milli-newtons
  short current_motor_b_mA;   // or force y in milli-newtons
  short current_motor_c_mA;   // or force z in milli-newtons
  short command; // e.g. reset encoders
  short command_attr0;
  short command_attr1;
  short command_attr2;
  int toChars(char *c) {
    int n = sprintf(c, "%hd %hd %hd %hd %hd %hd %hd", current_motor_a_mA, current_motor_b_mA,
                   current_motor_c_mA, command, command_attr0, command_attr1, command_attr2);
    while(n<63) c[n++] = ' ';
    c[62]='\n';
    c[63]='\0';
    return 63;
  }
  bool fromChars(const char *c) {
    return (7==sscanf(c, "%hd %hd %hd %hd %hd %hd %hd", &current_motor_a_mA, &current_motor_b_mA,
           &current_motor_c_mA, &command, &command_attr0, &command_attr1, &command_attr2));
  }
};
#pragma pack(pop)
// -------------------------------------------------



// -------------------------------------------------
// Pinout configuration & setup
// -------------------------------------------------
// Check if we are in emulator mode
constexpr int emulator_pin =  3; // GND on emulator, NC on Polhem
bool checkEmulatorMode(){
  pinMode(emulator_pin, INPUT_PULLUP);
  delay(100);
  return !digitalRead(emulator_pin);
}
const bool emulator_mode = checkEmulatorMode();

constexpr int ledPin = 13;
bool ledState=false;
constexpr int pwmPinA = 10;
constexpr int pwmPinB =  9;
constexpr int pwmPinC =  8;
constexpr int pwmResolutionBits = 13;
constexpr int pwmMax = pow(2, pwmResolutionBits) - 1;
constexpr int calib_pin =  6; // aka IX0
constexpr int enable_pin = 7;

constexpr int enc0_a_pin = 23;
constexpr int enc0_b_pin = 22;
constexpr int enc1_a_pin = 18;
constexpr int enc1_b_pin = 19;
constexpr int enc2_a_pin = 20;
constexpr int enc2_b_pin = 21;
constexpr int enc3_a_pin = 14; // Not used anymore, this is bluettooh now!
constexpr int enc3_b_pin = 15; // Not used anymore, this is bluetooth now!
const int enc4_a_pin = emulator_mode? 4 : 16; // Not used anymore, we use ssi
const int enc4_b_pin = emulator_mode? 5 : 17; // Not used anymore, we use ssi
constexpr int enc5_a_pin =  0; // This is actually 4th axis (was pin 12, moved with jumper)
constexpr int enc5_b_pin =  2; // This is actually 4th axis (was pin 11, moved with jumper)

constexpr int sda1 = 17; // Used for display in emulator mode
constexpr int scl1 = 16;

constexpr int enc4_ssi_miso = 12; // (This is hardware default)
constexpr int enc4_ssi_sck =  13; // (This is hardware default)
volatile int enc4_offset=0;

constexpr int debug_pin0 = 11;

// -------------------------------------------------
// Enconder logic
// -------------------------------------------------
volatile int prev_state[] = { -1, -1, -1, -1, -1, -1};
volatile bool encoder_raw[6][2] = {{false, false}, {false, false}, {false, false},
  {false, false}, {false, false}, {false, false}
};
//volatile int counter[] = {8327, -10926, 27140, 0, 30, 0}; // Default values on Polhem v3 2019
//volatile int counter[] = {8327, -10926, 19386, 0, 30, 0}; // Default values on Polhem v3 2020
volatile int counter[] = {8298, -10771, 19756, 0, 30, 0}; // Default values on Polhem v3+ 2020-10-27
constexpr int stable[16]   = {0, 1, -1, 0, -1, 0, 0, 1, 1, 0, 0, -1, 0, -1, 1, 0}; // Reverse
void encoder_callback(int _encoder, int AB, bool value) {
  encoder_raw[_encoder][AB] = value;
  int cur_state = encoder_raw[_encoder][0] << 1 | encoder_raw[_encoder][1];
  if (prev_state[_encoder] < 0) prev_state[_encoder] = cur_state;
  counter[_encoder] += stable[prev_state[_encoder] << 2 | cur_state];
  prev_state[_encoder] = cur_state;
}
// "callback stubs"
void callback_0_A_rise(void) {
  encoder_callback(0, 0, digitalRead(enc0_a_pin));
}
void callback_0_B_rise(void) {
  encoder_callback(0, 1, digitalRead(enc0_b_pin));
}
void callback_1_A_rise(void) {
  encoder_callback(1, 0, digitalRead(enc1_a_pin));
}
void callback_1_B_rise(void) {
  encoder_callback(1, 1, digitalRead(enc1_b_pin));
}
void callback_2_A_rise(void) {
  encoder_callback(2, 0, digitalRead(enc2_a_pin));
}
void callback_2_B_rise(void) {
  encoder_callback(2, 1, digitalRead(enc2_b_pin));
}
void callback_3_A_rise(void) {
  encoder_callback(3, 0, digitalRead(enc3_a_pin));
}
void callback_3_B_rise(void) {
  encoder_callback(3, 1, digitalRead(enc3_b_pin));
}
void callback_4_A_rise(void) {
  encoder_callback(4, 0, digitalRead(enc4_a_pin));
}
void callback_4_B_rise(void) {
  encoder_callback(4, 1, digitalRead(enc4_b_pin));
}
void callback_5_A_rise(void) {
  encoder_callback(5, 0, digitalRead(enc5_a_pin));
}
void callback_5_B_rise(void) {
  encoder_callback(5, 1, digitalRead(enc5_b_pin));
}
// -------------------------------------------------


// -------------------------------------------------
// Push button
// -------------------------------------------------
volatile unsigned long lastDebounceTime = 0; 
volatile bool ix0_button_flag = false;
Bounce pushbutton = Bounce(); 
void calib_button(void) {
  if (pushbutton.update()) {
    if (pushbutton.fell()) {
      ix0_button_flag = true;
    }
  }
}
IntervalTimer btnTimer;
// -------------------------------------------------


volatile short force_x_mN = 0; // Set by computer user
volatile short force_y_mN = 0;
volatile short force_z_mN = 0;
volatile bool compute_and_apply_forces = false; 

// -------------------------------------------------
// Message watchdog
// -------------------------------------------------
IntervalTimer wdTimer;
void wdCallback() {
  digitalWrite(enable_pin, 0);
  analogWrite(pwmPinA, pwmMax / 2);
  analogWrite(pwmPinB, pwmMax / 2);
  analogWrite(pwmPinC, pwmMax / 2);  
  force_x_mN = 0;
  force_y_mN = 0;
  force_z_mN = 0;
}
// -------------------------------------------------


// -------------------------------------------------
// Loop speed measurements
// -------------------------------------------------
volatile int lastEsconHz=0;
volatile int esconLoopCount=0;
volatile int lastMainHz=0;
volatile int mainLoopCount=0;
IntervalTimer updateRateTimer;
void esconLoopHzCallback(){
  lastEsconHz = esconLoopCount;
  esconLoopCount=0;
  lastMainHz = mainLoopCount;
  mainLoopCount=0;
}
// -------------------------------------------------



// -------------------------------------------------
// Escon thread
// -------------------------------------------------
volatile double latest_position[]={0,0,0};
volatile double latest_angles[]={0,0,0,0}; // tA,lambda,tD,tE
volatile int milliamps[]={0,0,0};
void esconThread(){
   while (true){
      if(!compute_and_apply_forces) { delay(100); continue; }

#ifdef DIRECT_COMPUTE
      // Compute here, what we got is mNewtons in x,y,z
      const int base[] = {counter[0], counter[1], counter[2]};
      fsVec3d f(force_x_mN*0.001,
                force_y_mN*0.001,
                force_z_mN*0.001);
      fsVec3d amps = kinematics.computeMotorAmps(f, base);
      milliamps[0] = 1000*amps.m_x;
      milliamps[1] = 1000*amps.m_y;
      milliamps[2] = 1000*amps.m_z;
      
      // Verify range of milliamps.
      for(int i=0;i<3;++i)
        milliamps[i] = (milliamps[i]<=escon_max_milliamps && milliamps[i]>=-escon_max_milliamps)? milliamps[i] : 0;      
      
      int duty = int(pwmMax * (0.4 * (milliamps[0] / float(escon_max_milliamps)) + 0.5));
      analogWrite(pwmPinA, duty);
      
      duty = int(pwmMax * (0.4 * (milliamps[1] / float(escon_max_milliamps)) + 0.5));
      analogWrite(pwmPinB, duty);
      
      duty = int(pwmMax * (0.4 * (milliamps[2] / float(escon_max_milliamps)) + 0.5));
      analogWrite(pwmPinC, duty);

      digitalWrite(debug_pin0, milliamps[0]>10);

      fsVec3d p = kinematics.computePosition(counter[0], counter[1], counter[2]);
      latest_position[0]=p.m_x;
      latest_position[1]=p.m_y;
      latest_position[2]=p.m_z;

      const int rot[]={counter[5],counter[4],counter[3]};
      kinematics.computeRotation(base,rot, latest_angles);

      esconLoopCount++;
      #endif
      yield();
   }
}
// -------------------------------------------------


// -------------------------------------------------
// Display (on emulator)
// -------------------------------------------------
void displayThread() {
#ifdef USE_DISPLAY
  delay(1000);
  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3C for 128x32
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }
  char displaybuf[256];  
  while(true){

      display.clearDisplay();
      display.setTextSize(1);      // Normal 1:1 pixel scale
      display.setTextColor(SSD1306_WHITE); // Draw white text
      display.setCursor(0, 0);     // Start at top-left corner
      display.cp437(true);         // Use full 256 char 'Code Page 437' font
      
      memset(displaybuf,0,256);
      sprintf(displaybuf,"%d %d %d\n%.2f %.2f %.2f\n%d %d %d\n%d %d %d",counter[0],counter[1],counter[2],
                                                         // counter[3],counter[4],counter[5], 
                                                          1000*latest_position[0], 1000*latest_position[1],1000*latest_position[2],
                                                          milliamps[0],milliamps[1],milliamps[2], force_x_mN, lastMainHz, lastEsconHz);
    
      // Not all the characters will fit on the display. This is normal.
      // Library will draw what it can and the rest will be clipped.
      for(int16_t i=0; i<256; i++) {
        display.write(displaybuf[i]);
      }
    
      display.display();     
      delay(2000); // 10 fps
  }
#endif       
}
// -------------------------------------------------



// -------------------------------------------------
// Setup
// -------------------------------------------------
void setup() {
  // Enable led in emulator mode
  if(emulator_mode){
    pinMode(ledPin, OUTPUT);
    digitalWrite(ledPin, 0);
  }

  pinMode(debug_pin0, OUTPUT);
  digitalWrite(debug_pin0, 0);
  
  pinMode(pwmPinA, OUTPUT);
  pinMode(pwmPinB, OUTPUT);
  pinMode(pwmPinC, OUTPUT);
  analogWriteResolution(pwmResolutionBits); // 0-4095
  analogWriteFrequency(pwmPinA, 4000);
  analogWriteFrequency(pwmPinB, 4000);
  analogWriteFrequency(pwmPinC, 4000);

  analogWrite(pwmPinA, pwmMax / 2);
  analogWrite(pwmPinB, pwmMax / 2);
  analogWrite(pwmPinC, pwmMax / 2);

  pinMode(calib_pin, INPUT_PULLUP); // pull-up by default
  pinMode(enable_pin, OUTPUT);
  
  digitalWrite(enable_pin, 0);

  pinMode(enc0_a_pin, emulator_mode? INPUT_PULLUP : INPUT);
  pinMode(enc0_b_pin, emulator_mode? INPUT_PULLUP : INPUT);
  pinMode(enc1_a_pin, emulator_mode? INPUT_PULLUP : INPUT);
  pinMode(enc1_b_pin, emulator_mode? INPUT_PULLUP : INPUT);
  pinMode(enc2_a_pin, emulator_mode? INPUT_PULLUP : INPUT);
  pinMode(enc2_b_pin, emulator_mode? INPUT_PULLUP : INPUT);
  pinMode(enc3_a_pin, emulator_mode? INPUT_PULLUP : INPUT);
  pinMode(enc3_b_pin, emulator_mode? INPUT_PULLUP : INPUT);
  pinMode(enc4_a_pin, emulator_mode? INPUT_PULLUP : INPUT);
  pinMode(enc4_b_pin, emulator_mode? INPUT_PULLUP : INPUT);
  pinMode(enc5_a_pin, emulator_mode? INPUT_PULLUP : INPUT);
  pinMode(enc5_b_pin, emulator_mode? INPUT_PULLUP : INPUT);

  // First, second and third axis always incremental encoders
  attachInterrupt(digitalPinToInterrupt(enc0_a_pin), callback_0_A_rise, CHANGE);
  attachInterrupt(digitalPinToInterrupt(enc0_b_pin), callback_0_B_rise, CHANGE);
  attachInterrupt(digitalPinToInterrupt(enc1_a_pin), callback_1_A_rise, CHANGE);
  attachInterrupt(digitalPinToInterrupt(enc1_b_pin), callback_1_B_rise, CHANGE);
  attachInterrupt(digitalPinToInterrupt(enc2_a_pin), callback_2_A_rise, CHANGE);
  attachInterrupt(digitalPinToInterrupt(enc2_b_pin), callback_2_B_rise, CHANGE);

  // Forth axis (yes, hooked to "enc5") incremental encoder both real and emulator
  attachInterrupt(digitalPinToInterrupt(enc5_a_pin), callback_5_A_rise, CHANGE);
  attachInterrupt(digitalPinToInterrupt(enc5_b_pin), callback_5_B_rise, CHANGE);

  if(emulator_mode){ // Fifth axis on actual Polhem read with SSI HW, enc on emulator
    attachInterrupt(digitalPinToInterrupt(enc4_a_pin), callback_4_A_rise, CHANGE);
    attachInterrupt(digitalPinToInterrupt(enc4_b_pin), callback_4_B_rise, CHANGE);
  }

  if(emulator_mode){ // Sixth axis on actual Polhem goes over Bluetooh. Enc on emulator
    attachInterrupt(digitalPinToInterrupt(enc3_a_pin), callback_3_A_rise, CHANGE);
    attachInterrupt(digitalPinToInterrupt(enc3_b_pin), callback_3_B_rise, CHANGE);
  }

  Serial.begin(115200);
  Serial.flush();

#ifdef SERIAL_BRIDGE
  Serial4.begin(115200);
#endif

  delay(400);

  // Read SPI counter    
  if(!emulator_mode){
    SPI.begin();
    SPI.beginTransaction(SPISettings(100000, MSBFIRST, SPI_MODE2));     
    int v = SPI.transfer(0) << 8 |  SPI.transfer(0);
    SPI.endTransaction();
    delay(100);
    SPI.beginTransaction(SPISettings(100000, MSBFIRST, SPI_MODE2));     
    v = SPI.transfer(0) << 8 |  SPI.transfer(0);
    SPI.endTransaction();
    v = (v >> 5) & 1023; // 1A98 7654 3210 0000 -> A987654321
    //counter[4] = 30 + 1024-v; // Calibrate to initial value 30
    enc4_offset = 30 + 1024-v;
    delay(100);
  }
   
  // Set up reading of button with debounce
  pushbutton.interval(10);
  pushbutton.attach(calib_pin);
  btnTimer.begin(calib_button,2); // Check satus every 2ms   
  
  updateRateTimer.begin(esconLoopHzCallback,1000000); // 1s timeout
  
  // Start escon thread
  //threads.setDefaultStackSize(16384);
  //threads.setMicroTimer(10); // 10us
  //threads.addThread(esconThread);
  #ifdef USE_DISPLAY
  threads.addThread(displayThread);
  #endif
}

const size_t buf_len = 64;
char buffer[buf_len];
char out_buf[buf_len];

void bitstr(char *o, int v, int offset=0){
  for(int b=31;b>=0;b--)      
    o[offset+31-b] = '0' + ((v & (1<<b))? 1 : 0);    
}

void loop() {
  mainLoopCount++;  
  memset(buffer,0,buf_len);
  memset(out_buf,0,buf_len);

  for (size_t buf_pos = 0; buf_pos < buf_len; buf_pos++) {
    while (Serial.available() <= 0){
      //yield();

#ifdef DIRECT_COMPUTE
        const int base[] = {counter[0], counter[1], counter[2]};
        fsVec3d f(force_x_mN*0.001,
                  force_y_mN*0.001,
                  force_z_mN*0.001);
        fsVec3d amps = kinematics.computeMotorAmps(f, base);       
        if(emulator_mode){
          if(amps.m_x>10) digitalWrite(ledPin,ledState=!ledState);        
        }
#endif
    }

    // got message
    //if(emulator_mode)
    //  digitalWrite(ledPin,ledState=!ledState);
    
    int c = Serial.read();
    buffer[buf_pos] = c;
    if (c == '\n') {

      
      // Read SPI counter
      int v=0;
      if(!emulator_mode){
        SPI.beginTransaction(SPISettings(100000, MSBFIRST, SPI_MODE2));     
        v = SPI.transfer16(0);
        SPI.endTransaction();
        v = (v >> 5) & 1023; // 1A98 7654 3210 0000 -> A987654321
      }
      counter[4] = (v+enc4_offset)%1024;

      milliamps[0] = 0;
      milliamps[1] = 0;
      milliamps[2] = 0;
      

      // Check message if its okay
      pc_to_hid_message pc_to_hid;
      if(pc_to_hid.fromChars(buffer)){
        // Read sucessfully 7 data 

        // Callibrate command
        if (pc_to_hid.command == 1) {
          counter[0] = pc_to_hid.command_attr0;
          counter[1] = pc_to_hid.command_attr1;
          counter[2] = pc_to_hid.command_attr2;
          counter[3] = pc_to_hid.current_motor_a_mA;
          enc4_offset = pc_to_hid.current_motor_b_mA + 1024-v;
          counter[4] = (v+enc4_offset)%1024; // Because counter is absolute and loops at 1024
          counter[5] = pc_to_hid.current_motor_c_mA;
          ix0_button_flag = false;
        } else if(pc_to_hid.command == 2){ // Force command
          force_x_mN = pc_to_hid.current_motor_a_mA;
          force_y_mN = pc_to_hid.current_motor_b_mA;
          force_z_mN = pc_to_hid.current_motor_c_mA;
          compute_and_apply_forces = true;        

          // Compute milliamps
          const int base[] = {counter[0], counter[1], counter[2]};
          fsVec3d f(force_x_mN*0.001,
                    force_y_mN*0.001,
                    force_z_mN*0.001);
          fsVec3d amps = kinematics.computeMotorAmps(f, base);     

          for(int i=0;i<3;++i)
            milliamps[i] = 1000*amps[i];
          // Check if any motor saturated and dial down all if so
          /*
          double gainFactor=1.0;
          fsVec3d test_amps = amps;
          const double escon_max = escon_max_milliamps/1000;
          for(int i=0;i<3;++i){
            test_amps = amps*gainFactor;
            if(test_amps[i]>=escon_max || test_amps[i]<= -escon_max){
              gainFactor = min(abs(escon_max/amps[i]) -0.01, gainFactor);
              i=0; // go through all again
            }
          }
          amps = amps*gainFactor;
          */      
          
          digitalWrite(enable_pin, 1);
        } else {
          compute_and_apply_forces = false;
          milliamps[0] = pc_to_hid.current_motor_a_mA;
          milliamps[1] = pc_to_hid.current_motor_b_mA;
          milliamps[2] = pc_to_hid.current_motor_c_mA;            
          digitalWrite(enable_pin, 1);
        }

        wdTimer.begin(wdCallback,100000); // 100ms timeout for next message
      }

      // Set forces (0 by default)      
      constexpr int chan[] = {pwmPinA, pwmPinB, pwmPinC};
      for(int i=0;i<3;++i){
        milliamps[i] = (milliamps[i]<=escon_max_milliamps && milliamps[i]>=-escon_max_milliamps)? milliamps[i] : 0;  // extra safety check
        analogWrite(chan[i], int(pwmMax * (0.4 * (milliamps[i] / float(escon_max_milliamps)) + 0.5)));
      }        
        
      //digitalWrite(debug_pin0, amps.m_x>0);       
      int len=1;
      out_buf[0]='\n'; // Just so we always send something
      
      if(compute_and_apply_forces){
        fsVec3d p = kinematics.computePosition(counter[0], counter[1], counter[2]);
        latest_position[0]=p.m_x;
        latest_position[1]=p.m_y;
        latest_position[2]=p.m_z;
  
        const int base[] = {counter[0], counter[1], counter[2]};
        const int rot[]={counter[5],counter[4],counter[3]};        
        kinematics.computeRotation(base,rot, latest_angles);        
        
        position_hid_to_pc_message pmsg;
        pmsg.x_mm = 1000*latest_position[0];
        pmsg.y_mm = 1000*latest_position[1];
        pmsg.z_mm = 1000*latest_position[2];
        pmsg.a_ma = milliamps[0];
        pmsg.b_ma = milliamps[1];
        pmsg.c_ma = milliamps[2];
        pmsg.tA = latest_angles[0];
        pmsg.lambda = latest_angles[1];
        pmsg.tD = latest_angles[2];//lastMainHz;
        pmsg.tE = latest_angles[3];//lastEsconHz;
        pmsg.info = ix0_button_flag;
        len = pmsg.toChars(out_buf);
      } else {

        hid_to_pc_message hid_to_pc;        
        hid_to_pc.encoder_a = counter[0];
        hid_to_pc.encoder_b = counter[1];
        hid_to_pc.encoder_c = counter[2];
        hid_to_pc.encoder_d = counter[3];
        hid_to_pc.encoder_e = counter[4];
        hid_to_pc.encoder_f = counter[5];
        
        hid_to_pc.info = ix0_button_flag; // flagged until cleared (calibrated)
      
        len = hid_to_pc.toChars(out_buf);
                
        digitalWrite(debug_pin0, milliamps[0]>0);
      }

#ifdef SERIAL_BRIDGE
      int rd = Serial4.available();
      //Serial.print("Rd: \n");
      //Serial.print(rd);
      //Serial.print("\n");
      memset(out_buf,0,64);      
      out_buf[63]='\n';  
      if(rd>=63){
        int read_n_bytes = Serial4.readBytes((char *)out_buf, 63);
        //Serial.print("Read bytes: ");
        //Serial.print(read_n_bytes);
        //Serial.print("\n");
        
        len=64;
        Serial4.write(buffer,63); // Force command from computer
      }
#endif

      Serial.write(out_buf, len);
      Serial.send_now();

      //if(emulator_mode) { Serial.write("Emulator "); Serial.send_now();}
      //else              { Serial.write("Real device\n"); Serial.send_now();}

      break;
    }
  }
}
