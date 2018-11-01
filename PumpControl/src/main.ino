#include <Arduino.h>
#include <NimbelinkSVZM20.h>
#include <PID_v1.h>
#include <SSD1306Ascii.h>
#include <SSD1306AsciiWire.h>
#include <Wire.h>
#include "main.h"

// 0 for STROKE/SEC, 1 for GAL/DAY
boolean units = 1;

double screenSetPoint = 0, screenCurrentPoint = 0, nextSetPoint;
double setPoint, currentPoint, outPoint, cc_stroke = 1;

volatile int tach_count = 0;
volatile boolean pumping = false, set_timeout = false, draw_current = true;
boolean network_registration;
boolean local_update = false;
volatile boolean enc_button = false;
// store two cursor positions. allows flicker-less OLED updates.
uint8_t x1, h1, x2, h2;
int enc_motion = 0;
int encLast = 0;

NimbelinkSVZM20 modem(MODEM_RX, MODEM_TX);
SSD1306AsciiWire oled;

double Kp = 300, Ki = 700, Kd = 200;
PID pumpPID(&currentPoint, &outPoint, &setPoint, Kp, Ki, Kd, DIRECT);
// 0 - setpoint, 1 - cc/stroke, 2 - currentPoint
int parameters[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
unsigned long nextTransmission;

#include "teensyCode.h"

void setup() {
  Serial.begin(9600);
  Serial.println("started");
  pinSetup();
  busSetup();
  digitalWrite(PUMP_PWM, LOW);

  oled.begin(&Adafruit128x64, OLED_I2C, OLED_RESET);
  drawWelcome();
  oled.clear();
#ifdef NO_CLOUD
  network_registration = 1;
#endif
#ifndef NO_CLOUD
  oled.println("Network\nRegistration...");
  modem.begin1();
  if (modem.turnOn()) {
    network_registration = 1;
  } else {
    network_registration = 0;
  }

  oled.clear();
  oled.setScroll(true);

  if (network_registration) {
    drawNoCloud();
  } else {
    oled.clear();
    oled.print("Registration\nOK");
    delay(700);
  }
#endif
  interruptSetup();

  // init pump control vars and PID
  currentPoint = 0;
  setPoint = 0;
  pumpPID.SetMode(AUTOMATIC);
  // setup to output directly to PWM
  pumpPID.SetOutputLimits(0, 4096);
  pumpPID.SetSampleTime(TACH_RATE);

  nextTransmission = millis() + TX_TIME;
  oled.clear();
  encPosn.setup();
  encPosn.start();

  watchDogSetup();
}

void loop() {
  enc_motion = encPosn.calcPosn() - encLast;

  /*
     make it a little harder to get into set mode.
    also prevents false zeroing from getting us into the menu
  */
  if (enc_motion < 2 && enc_motion > -2) {
    enc_motion = 0;
  }

  // local control
  if (enc_motion) {
    // draw menu
    drawSet();
    // reset timeout flag
    set_timeout = false;
    do {
      local_update = true;
      // update variable on screen and get to save.
      nextSetPoint = drawSetPoint(screenSetPoint);
      // delay next TX to 15 sec.
      nextTransmission = millis() + 15000;
      // update our interim variable
      screenSetPoint = nextSetPoint;
      // timeout menu so we don't miss the watchdog timer
      if (set_timeout) {
        break;
      }
    } while (!enc_button);
    // let user know we're saving the new value
    drawSave();
    // save the new value
    setSave(nextSetPoint);
    // gotta redraw the current screen
    draw_current = true;
  } else {
    // only draw the title when needed
    if (draw_current) drawCurrent();
    // current point is updated in the background
    drawCurrentPoint();
  }

  // Serial.println(units);

#ifdef PID_DEBUG
  Serial.print(currentPoint * 10);
  Serial.print("\t");
  Serial.println(setPoint * 10);
#endif

  // modem TX/RX logic:
  if (millis() > nextTransmission) {
    if (network_registration) {
      modem.turnOff();
      delay(1000);
      network_registration = modem.turnOn();
    }
    if (!network_registration) {
      drawBusy();
      boolean xmit_success = !talkToServer();
      drawXmit(xmit_success);
      if (xmit_success) {
        cc_stroke = parameters[1] / 1000.0;
        screenSetPoint = galDayToStrokeSec(cc_stroke, parameters[0] / 1000.0);
        setSave(screenSetPoint);
      }
    }
  }
  // button press handlers
  if (enc_button) {
    // handle rebooting on button hold.
    boolean try_reset;
    boolean draw_once = true;
    delay(100);
    if (digitalRead(ENCODER_SWITCH)) {
      try_reset = true;
    } else {
      try_reset = false;
    }

    // handle double click
    delay(100);
    if (enc_button && !try_reset) {
      units ^= true;
    }

    button_t1 = millis();
    while (try_reset) {
      boolean button = digitalRead(ENCODER_SWITCH);
      if (millis() - button_t1 > 7000 && button) {
        systemReboot();
      } else if (millis() - button_t1 > 200 && button) {
        if (draw_once) {
          oled.clear();
          oled.set1X();
          oled.println("Hold for reboot.");
          oled.println("Release before 7 secs\nto go back.");
          oled.set2X();
          draw_once = false;
        }
      } else if (!button) {
        break;
      }
    }

    oled.clear();
    draw_current = true;
  }
  kickDog();
}
