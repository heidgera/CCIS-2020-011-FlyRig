#include <Adafruit_MCP4728.h>
#include <TimerOne.h>
#include <MsTimer2.h>

#include "serialParse.h"
#include "timeOut.h"

// declare the terms for the parser commands
const int STIMULATE = 5;
const int SET_BL = 6;
const int SET_OUTPUTS = 7;
const int ALL_OFF = 10;
const int ERROR = 126;
const int READY = 127;

// declare the digital-to-analog converter and serial parser objects
Adafruit_MCP4728 dac;
serialParser parser(Serial);

//declare variables for stimulating LEDs
bool active = false;
int amp = 0;
int freq = 1;
int duty = 0;
unsigned long period = 1;
int pulseLength = 5;
bool lightState = 0;
int ctrlPins[4] = {0,0,0,0};

// declare vairables for backlight
int blIntensity = 0;
const int backlightPin = 5;    // pin for IR LED control

//declare variables for odor output valves
const int numOutputs = 8;
int outputs[numOutputs] = {13,12,11,10,9,8,7,6};

//function for pulsing the stimulating LEDs on and off at the commanded frequency
void pulseLights(){
  lightState = true;
  MsTimer2::stop();
  lightState = false;
  MsTimer2::start();
  //numberOfPulses++;
  //digitalWrite(13, !digitalRead(13));
}

void lightToggle(){
  if(!(lightState = !lightState)) MsTimer2::stop();;
}

void startSeq(){
  active = true;
  for(int i=0; i<4; i++) dac.setChannelValue(i,4095);
  MsTimer2::set(pulseLength,lightToggle);  //initialize the timer2
  MsTimer2::stop();
  Timer1.attachInterrupt(pulseLights, period);
}

void stopSeq(){
  active = false;
  Timer1.detachInterrupt();
  MsTimer2::stop();
  for(int i=0; i<4; i++) dac.setChannelValue(i,4095);
}

unsigned long loopTimer =0;
int loopCount = 0;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);

  parser.address = 1;

  pinMode(backlightPin, OUTPUT);
  analogWrite(backlightPin,0);

  for(int i=0; i<8; i++){
    pinMode(outputs[i],OUTPUT);
    digitalWrite(outputs[i],LOW);
  }

  Timer1.initialize();  //initialize the timer1
//  MsTimer2::set(pulseLength,lightToggle);  //initialize the timer2
//  MsTimer2::stop();

  if (!dac.begin()) {
    parser.sendPacket(REPORT,ERROR);
  }

  parser.on(READY, [](unsigned char * input, int size){
    parser.sendPacket(REPORT,READY);
  });

  parser.on(STIMULATE, [](unsigned char * input, int size){
    stopSeq();
    freq = input[2];
    amp = input[3];
    duty = constrain(map(amp, 0, 100, 3440, 1352), 0, 4096);
    period = 1000000/freq;
    pulseLength = input[4];
    for(int i=0; i<4; i++){
      ctrlPins[i] = bitRead(input[5],i)?1:0;
    }
    if(amp>0 && input[5]) startSeq();
    parser.sendPacket(REPORT,STIMULATE);
  });

  parser.on(SET_BL, [](unsigned char * input, int size){
    blIntensity = input[2]*2;
    analogWrite(backlightPin,blIntensity);
    parser.sendPacket(REPORT,SET_BL);
  });

  parser.on(SET_OUTPUTS, [](unsigned char * input, int size){
    for(int i=0; i<numOutputs; i++){
      digitalWrite(outputs[i],bitRead(input[2+i/7],i%7));
    }
  });

  parser.on(ALL_OFF, [](unsigned char * input, int size){
    digitalWrite(backlightPin,0);
    for(int i=0; i<numOutputs; i++){
      digitalWrite(outputs[i],0);
    }
    stopSeq();
  });


  parser.sendPacket(REPORT,READY);

//  freq = 5;
//  duty = 2000;
//  period = 1000000/freq;
//  pulseLength = 10;
//  ctrlPins[2] = ctrlPins[4] = 1;
//  startSeq();
}

bool lightsOn = false;

void loop() {
  parser.idle();

  if(lightState && active){
    lightsOn = true;
    for(int i=0; i<4; i++){
      dac.setChannelValue(i,(ctrlPins[i])?duty:4095);
    }
  } else if(lightsOn){
    lightsOn = false;
    for(int i=0; i<4; i++){
      dac.setChannelValue(i,4095);
    }
  }
}
