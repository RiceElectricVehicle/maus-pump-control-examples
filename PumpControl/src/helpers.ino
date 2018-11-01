#include <Arduino.h>
boolean talkToServer() {
  boolean returnVal = 1;
  // let server know if there's a local update being pushed
  String message =
      "deviceID=5&localUpdate=" + String(local_update) + "&params=";
  // send up setPoint and currentPoint
  parameters[0] = (int)strokeSecToGalDay(cc_stroke, setPoint * 1000);
  parameters[1] = (int)(cc_stroke * 1000);
  parameters[2] = (int)strokeSecToGalDay(cc_stroke, currentPoint * 1000);

  char buffer[100];
  for (int i = 0; i < 10; i++) {
    if (i) message += ",";
    message += String(parameters[i]);
  }
  message.toCharArray(buffer, 100);

  returnVal = !!modem.sendToServer(buffer, parameters);

  Serial.print("New parameters: ");
  for (int i = 0; i < 10; i++) {
    if (i) Serial.print(", ");
    Serial.print(parameters[i]);
  }
  Serial.println("");

  // make sure we dont forget to send local changes (next time) if TXmit fails
  if (returnVal && local_update) {
    local_update = true;
  } else if (!returnVal && local_update) {
    local_update = false;
  }

  nextTransmission = millis() + TX_TIME;
  return returnVal;
}
