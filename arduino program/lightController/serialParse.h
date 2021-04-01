#include "arduino.h"

#define START_FLAG 128
#define STOP_FLAG 0
#define REPORT 126
#define BROADCAST 127

class serialParser {
public:

  int length;
  unsigned char  *input;
  int pntr;
  bool packStarted;
  int address;
  unsigned char endByte;
  void (*endCB)(unsigned char  *, int);
  void (*errCB)(unsigned char  *, int);
  void (*wrongAddrCB)(unsigned char  *, int);
  void (* commandCBs [128])(unsigned char  *, int);
  Stream * serial;

  serialParser(Stream &port){
    length = 128;
    address = -1;
    input = new unsigned char[length];
    for(int i=0; i < 128; i++){
      commandCBs[i] = NULL;
    }
    pntr = 0;
    packStarted = false;
    endByte = START_FLAG + STOP_FLAG;
    errCB = wrongAddrCB = endCB = NULL;
    serial = &port;
  }

  void setCallback(void (*CB)(unsigned char  *,int)){
    endCB = CB;
  }

  void setErrorCB(void (*CB)(unsigned char  *,int)){
    errCB = CB;
  }

  void on(unsigned char cmd, void (*CB)(unsigned char  *, int)){
    commandCBs[cmd] = CB;
  }

  void sendPacket(char addr, char cmd, byte data = 128, byte data2 = 128, byte data3 = 128, byte data4 = 128, byte data5 = 128, byte data6 = 128){
    char next = 0;
    
    int tot = (START_FLAG | addr) + cmd;
    serial->write(START_FLAG | addr);
    serial->write(cmd);
    
    /*va_list argList;
    va_start(argList, num);
    
    while(num-->0) {
      next = va_arg(argList, char);
      serial->write(next);
      tot += next;
    }  
    va_end(argList);*/

    if(data < 128) tot+=data, Serial.write(data);
    if(data2 < 128) tot+=data2, Serial.write(data2);
    if(data3 < 128) tot+=data3, Serial.write(data3);
    if(data4 < 128) tot+=data4, Serial.write(data4);
    if(data5 < 128) tot+=data5, Serial.write(data5);
    if(data6 < 128) tot+=data6, Serial.write(data6);
    
    serial->write(tot & 0b01111111);
    serial->write(START_FLAG + STOP_FLAG);
  }

  void startMessage(){
    serial->write(START_FLAG | REPORT);
  }

  void endMessage(){
    serial->write(START_FLAG + STOP_FLAG);
  }
  
  bool errCheck(){
    int tot = 0;
    for(int i = 0; i < pntr - 1; i++) {
      tot += input[i];
    }
    return ((tot & 0b01111111) == input[pntr - 1]);
  }

  bool checkAddress(){
    int recvAddr = input[0] & 0b01111111;
    return (recvAddr == address || recvAddr == 127);
  }


  void push(unsigned char inChar){

    if(inChar > endByte){
      packStarted = true;
      pntr = 1;
      input[0] = inChar;
    } else if (inChar == endByte){
      if(pntr>0 && packStarted){
        if(errCheck()){
          if(checkAddress()){
            if(commandCBs[input[1]]) commandCBs[input[1]](input, pntr);
            else if(endCB) endCB(input,pntr);
          } else {
            if(wrongAddrCB) wrongAddrCB(input,pntr);
          }
        } else if(errCB) errCB(input,pntr);
      }
      packStarted = false;
      pntr = 0;
    } else if(pntr < length && packStarted) input[pntr++] = inChar;
    else if(pntr >= length) pntr = 0;

  }

  void idle(){
    while(serial->available()) push(serial->read());
  }
};
