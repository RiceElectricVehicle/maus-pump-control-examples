/*
  NimbelinkSVZm20.cpp - Library for Nimbelink Verizon M1 modem.
  Stephen Crowe
*/

/*
  TODO: teensy has WATCHDOG timer. maybe can be used to rescue MCU from some
  modem lockdown?
*/
#ifndef NimbelinkSVZM20_h
#define NimbelinkSVZM20_h
#include "Arduino.h"
#include "main.h"
#include "string.h"

class NimbelinkSVZM20 {
 public:
  NimbelinkSVZM20(int RX_PIN, int TX_PIN);
  void begin1();
  void end1();
  char read();
  int available();
  int write(char c);
  int write(const char *str);
  int flush();
  int turnOn();
  int turnOff();
  int sendCommand(const char *command);
  int waitForResponse(const char *expectedResponse, int timeout = 1000);
  int sendToServer(char buffer[], int response[32]);

 private:
  int _closeSocket();
  int _RX_PIN;
  int _TX_PIN;
};

void kickDog1();

#endif