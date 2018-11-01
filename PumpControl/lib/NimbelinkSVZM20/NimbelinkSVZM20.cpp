/*
  NimbelinkSVZm20.cpp - Library for Nimbelink Verizon M1 modem.
  Stephen Crowe
  All functions return 0 for no error, 1 for error.
*/

#include "NimbelinkSVZM20.h"

#define _modem Serial2
#define DEBUG_LEVEL 2

NimbelinkSVZM20::NimbelinkSVZM20(int RX_PIN, int TX_PIN) {
  _RX_PIN = RX_PIN;
  _TX_PIN = TX_PIN;
}

void NimbelinkSVZM20::begin1() {
  pinMode(_RX_PIN, INPUT);
  pinMode(_TX_PIN, OUTPUT);
  _modem.setTX(_TX_PIN);
  _modem.setRX(_RX_PIN);
  _modem.begin(19200);
  pinMode(MODEM_ONOFF, OUTPUT);
  digitalWrite(MODEM_ONOFF, HIGH);
  delay(1000);
  pinMode(MODEM_RESET, OUTPUT);
  digitalWrite(MODEM_RESET, HIGH);
}

void NimbelinkSVZM20::end1() {
  _modem.end();
  digitalWrite(MODEM_ONOFF, HIGH);
}

char NimbelinkSVZM20::read() {
  char c = _modem.read();
#if DEBUG_LEVEL >= 2
  if (c != -1) Serial.print(c);
#endif
  return c;
}
int NimbelinkSVZM20::available() { return _modem.available(); }
int NimbelinkSVZM20::write(const char *str) {
  _modem.print(str);
#if DEBUG_LEVEL >= 2
  Serial.print(str);
#endif
  return 0;
}
int NimbelinkSVZM20::write(char c) {
  _modem.write(c);
#if DEBUG
  Serial.print(c);
#endif
  return 0;
}
int NimbelinkSVZM20::flush() {
  while (_modem.read() != -1)
    ;
  return 0;
}

int NimbelinkSVZM20::turnOn() {
#if DEBUG_LEVEL >= 1
  Serial.println("Turning on modem.");
#endif

  unsigned long deadline;
  digitalWrite(MODEM_ONOFF, LOW);
  delay(1000);
  digitalWrite(MODEM_RESET, LOW);
  delay(1000);
  digitalWrite(MODEM_RESET, HIGH);
  pinMode(MODEM_RESET, INPUT_PULLUP);
  delay(3000);
  flush();
  // if (waitForResponse("+SYSSTART", 10000)) return 1;

  sendCommand("ATE0");
  if (waitForResponse("OK")) return 1;

  sendCommand("AT+CFUN=1");
  if (waitForResponse("OK")) return 1;

  sendCommand("AT+SQNAUTOINTERNET=1");
  if (waitForResponse("OK")) return 1;
#if DEBUG_LEVEL >= 1
  Serial.println("Registering on the network.");
#endif

  deadline = millis() + 120000;
  do {
    if (millis() >= deadline) {
#if DEBUG_LEVEL >= 1
      Serial.println("Registering on network timed out.");
#endif

      return 1;
    }
    delay(100);
    flush();
    sendCommand("AT+CEREG?");
  } while (waitForResponse("+CEREG: 2,1"));

  return 0;
}

int NimbelinkSVZM20::turnOff() {
#if DEBUG_LEVEL >= 1
  Serial.println("Turning off modem.");
#endif

  digitalWrite(MODEM_ONOFF, LOW);
  return 0;
}

int NimbelinkSVZM20::sendCommand(const char *command) {
  flush();
  write(command);
  write("\r\n");
  return 0;
}

int NimbelinkSVZM20::waitForResponse(const char *expectedResponse,
                                     int timeout) {
  unsigned long deadline = millis() + timeout;
  int length = strlen(expectedResponse);
  int index = 0;
  char buffer[length];

  while (millis() < deadline) {
    kickDog1();
    if (available()) {
      buffer[index++ % length] = read();

      char *p1 = (char *)expectedResponse;
      char *p2 = (char *)buffer;
      int i = 0;
      while (*p1 && *p2 && *p1 == *(p2 + (index + i) % length)) {
        p1++;
        i++;
      }
      if (i == length) return 0;
    }
  }
  return 1;
}

int NimbelinkSVZM20::sendToServer(char buffer[], int response[32]) {
#if DEBUG_LEVEL >= 1
  Serial.println("Sending data to server.");
#endif

  String str = "";
  char c;
  int index = 0;
  unsigned long deadline = millis() + 30000;

#if DEBUG_LEVEL >= 1
  Serial.println("Opening connection to server.");
#endif
  sendCommand("");
  delay(1000);
  flush();
  sendCommand("AT+SQNSD=3,0,80,\"http.sensorfield.com\"");
  if (waitForResponse("CONNECT", 10000)) return 1;

#if DEBUG_LEVEL >= 1
  Serial.println("Sending data.");
#endif

  flush();
  write("GET /twoway?");
  write(buffer);
  write("&response=1,2,3$ HTTP/1.1\r\n");
  write("Host: http.sensorfield.com\r\n\r\n");

  if (waitForResponse("\r\n\r\n", 30000)) {
    _closeSocket();
    return 1;
  }

  while (1) {
    // if timeed out, return error
    if (millis() >= deadline) {
#if DEBUG_LEVEL >= 1
      Serial.println("Response from server timed out.");
#endif

      _closeSocket();
      return 1;
    }

    // if nothing to read, restart teh loop
    if (!_modem.available()) continue;

    c = read();

    // if the character is a delimiter, convert the current string to a number
    // and restart the loop
    if (c == ',') {
      response[index++] = str.toInt();
      str = "";
      continue;
    }

    // if the character is the end of message character, convert the current
    // string to a number and exit the loop
    if (c == '$') {
      response[index++] = str.toInt();
      break;
    }

    // otherwise, the character is a digit so add it to the current string
    str += c;
  }

  return _closeSocket();
}

int NimbelinkSVZM20::_closeSocket() {
#if DEBUG_LEVEL >= 1
  Serial.println("Closing connection to server.");
#endif

  flush();
  write("+++");
  if (waitForResponse("OK", 5000)) return 1;
  flush();
  sendCommand("AT+SQNSH=3");
  return waitForResponse("OK", 5000);
}

void kickDog1() {
  noInterrupts();
  WDOG_REFRESH = 0xA602;
  WDOG_REFRESH = 0xB480;
  interrupts()
}
