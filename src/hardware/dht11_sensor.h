#pragma once

#include <Arduino.h>
#include <DHT.h>

struct DhtReading {
  float temperatureC;
  float humidityPercent;
  bool valid;
};

class Dht11Sensor {
 public:
  explicit Dht11Sensor(uint8_t dataPin);

  void begin();
  DhtReading read();

 private:
  DHT sensor_;
};
