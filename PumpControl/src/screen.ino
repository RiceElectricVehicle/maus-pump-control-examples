/*
  This file includes all functions that draw to the screen
  or that compute something for the screen.
*/

// shorthand for storing cursor position
void setCoords(int n) {
  if (n == 1) {
    x1 = oled.col();
    h1 = oled.row();
  } else if (n == 2) {
    x2 = oled.col();
    h2 = oled.row();
  }
}

// draw Sensorfield
void drawWelcome() {
  oled.setFont(Callibri11);
  oled.clear();
  oled.set1X();
  oled.print("\n");
  oled.set2X();
  setCoords(1);
  String logo = "  Sensorfield";
  for (byte i = 0; i < 0xE; i++) {
    oled.print(logo[i]);
    delay(25);
  }
  delay(800);
  oled.setCursor(x1, h1);
  for (byte i = 0; i < 0xF; i++) {
    oled.print("  ");
    delay(10);
  }
  // oled.clear();
}

// Draw  "Set:" title
void drawSet() {
  oled.clear();
  oled.set1X();
  oled.print("\n  ");
  // first set of coords is used to wipe out entire "set" menu
  setCoords(1);
  oled.set2X();
  for (byte i = 0; i < 5; i++) {
    oled.print("Set: "[i]);
    delay(10);
  }
  // second set of coords is used to update the set value
  setCoords(2);
}

double drawSetPoint(float newSetPoint) {
  oled.setCursor(x2, h2);
  // screen var is what is drawn to screen.
  double screenVar;
  int encCurrent = encPosn.calcPosn();
  int rtEnc = encCurrent - encLast;
  if (rtEnc) {
    double enc_grain = units ? 0.001 : 0.01;
    newSetPoint += rtEnc * enc_grain;
    oled.print("                ");
    setTimer.update(3000000);
    setTimer.begin(set_timeout_isr, 3000000);
    oled.setCursor(x2, h2);
  }
  if (newSetPoint < 0) {
    newSetPoint = 0;
  }
  screenVar = units ? strokeSecToGalDay(cc_stroke, newSetPoint) : newSetPoint;
  oled.print(screenVar);
  encLast = encCurrent;
  return newSetPoint;
}

void drawSave() {
  oled.setCursor(x1, h1);
  for (byte i = 0; i < 12; i++) {
    oled.print("Set saved   "[i]);
    delay(30);
  }
  local_update = true;
  delay(500);
  for (byte i = 0; i < 10; i++) {
    oled.print(" ");
    delay(10);
  }
  oled.clear();
}

// save new setPoint from "set" mode and clear the "set" menu
void setSave(float newPoint) { setPoint = newPoint; }

// draw "Current:" menu
void drawCurrent() {
  oled.set2X();
  if (units) {
    oled.println("gal/day :");
  } else {
    oled.println("stroke/sec :");
  }
  setCoords(1);
  draw_current = false;
}

// draw current stroke rate
void drawCurrentPoint() {
  oled.setCursor(x1, h1);
  oled.print(String(screenCurrentPoint) + "     ");
}

void drawBusy() {
  oled.clear();
  oled.print("Talking to\nServer");
}

void systemReboot() {
  oled.clear();
  oled.print("Rebooting");
  for (int i = 0; i < 4; i++) {
    oled.print("...."[i]);
    delay(100);
  }
  delay(1000);
  tauntDog();
  while (1) {
  }
}

void drawNoCloud() {
  oled.clear();
  oled.print("Registration\nTimeout");
  delay(500);
  oled.clear();
  delay(100);
  oled.print("Registration\nTimeout");
  delay(500);
  oled.clear();
  delay(100);
  oled.print("Registration\nTimeout");
  delay(1000);
  oled.set1X();
  oled.clear();
  oled.print("Cloud capabilities disabled.\n");
  delay(5000);
  oled.set2X();
}

void drawXmit(boolean success) {
  if (success) {
    oled.clear();
    oled.print("Setting\n new params..");
    delay(1000);
    oled.clear();
  } else {
    oled.clear();
    oled.print("Connection\nfailed.");
    delay(2000);
    oled.clear();
  }
  draw_current = true;
}
