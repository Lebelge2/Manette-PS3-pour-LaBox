

#ifndef __PS3_h
#define __PS3_h

//#include "EXComm.h"
#include <arduino.h>

//#ifdef ENABLE_PS3

void notify();
void onConnect();
void LEDCli(byte NumLED);
class PS3 {
  public:
    static bool begin();
    static bool loop();
};
/*
  class PS3: public  EXCommItem {
  public:                             // EXCommItem part
   // PS3();
    PS3(): EXCommItem("PS3") {};
    bool begin() override ;
    bool loop() override ;
  };
*/


//#endif
#endif
