/*
  This file contains all teensy specific code.
  ISRs, Watchdog timer setup and refresh, Hardware quadrature decoding, etc.
*/

#ifndef teensy_h
#define teensy_h
#include "QuadDecode_def.h"

QuadDecode<1> encPosn;
volatile int32_t rtEnc = 0;
IntervalTimer setTimer;
IntervalTimer runPID;
unsigned long button_t1 = 0;
int num_reset = WDOG_RSTCNT;

void watchDogSetup() {
  noInterrupts();
  WDOG_UNLOCK = WDOG_UNLOCK_SEQ1;
  WDOG_UNLOCK = WDOG_UNLOCK_SEQ2;
  delayMicroseconds(1);
  // WDOGEN | IRQRSTEN | WAITEN
  WDOG_STCTRLH = 0x0001 | bit(2) | bit(7);
  // next 2 lines sets the time-out value.
  WDOG_TOVALL = 0x09FF;
  WDOG_TOVALH = 0;
  // This sets prescale clock so that the watchdog timer ticks at 1kHZ instead
  // of the default 1kHZ/4 = 200 HZ
  WDOG_PRESC = bit(10) | bit(9) | bit(8);
  interrupts();
}

// refresh the watchdog timer
void kickDog() {
  noInterrupts();
  WDOG_REFRESH = 0xA602;
  WDOG_REFRESH = 0xB480;
  interrupts()
}

// get reset by WDOG
void tauntDog() {
  noInterrupts();
  WDOG_REFRESH = 0x0000;
  WDOG_REFRESH = 0x0000;
  interrupts();
}

void button_isr() { enc_button ^= true; }

// ISR to service pump TACH data
void tach_isr() { tach_count++; }

void set_timeout_isr() { set_timeout = true; }

double strokeSecToGalDay(double cc_stroke, double stroke_sec) {
  double cc_sec = cc_stroke * stroke_sec;
  double gal_day = 22.8245;
  return gal_day * cc_sec;
}

double galDayToStrokeSec(double cc_stroke, double gal_day) {
  double cc_sec = 0.0438126364 * gal_day;
  return cc_sec * (1 / cc_stroke);
}

// this function is called every 200ms, stroke rate is easy to calculate if we
// know ticks per stroke
volatile double getStrokeRate() {
  volatile double strokeRate =
      (1000 / TACH_RATE) * (1 / TACH_TICKS_PER_STROKE) * tach_count;
  tach_count = 0;
  return strokeRate;
}

void run_pid_isr() {
  currentPoint = getStrokeRate();
  screenCurrentPoint =
      units ? strokeSecToGalDay(cc_stroke, currentPoint) : currentPoint;

  if (setPoint == 0) {
    analogWrite(PUMP_PWM, 0);
  } else {
    pumpPID.Compute();
    analogWrite(PUMP_PWM, outPoint);
  }
}

void interruptSetup() {
  // define and start the ISR that will time us out of the "Set" menu
  setTimer.begin(set_timeout_isr, 3000000);
  setTimer.priority(128);
  // define and start the ISR that will run the PID loop so we never lose
  // control of pump
  runPID.begin(run_pid_isr, TACH_RATE * 1000);
  runPID.priority(127);
  // now some good ol interrupt setup.
  noInterrupts();
  attachInterrupt(digitalPinToInterrupt(ENCODER_SWITCH), button_isr, CHANGE);
  attachInterrupt(PUMP_SENSE, tach_isr, RISING);
  // enable interrupts
  interrupts();
}

void pinSetup() {
  pinMode(ENCODER_SWITCH, INPUT_PULLDOWN);
  analogWriteResolution(12);
  pinMode(PUMP_SENSE, INPUT_PULLDOWN);
}

void busSetup() {
  Wire.begin();
  Wire.setClock(400000L);
}
#endif