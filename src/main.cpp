#include <Arduino.h>

#include "app/device_controller.h"

namespace {
DeviceController deviceController;
}

void setup() {
  Serial.begin(115200);
  delay(100);
  deviceController.begin();
}

void loop() {
  deviceController.update(millis());
}
