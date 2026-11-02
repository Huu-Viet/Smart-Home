#include "dht11_sensor.h"

#include <math.h>

Dht11Sensor::Dht11Sensor(uint8_t dataPin) : sensor_(dataPin, DHT11) {}

void Dht11Sensor::begin() { sensor_.begin(); }

DhtReading Dht11Sensor::read() {
  const float humidity = sensor_.readHumidity();
  const float temperature = sensor_.readTemperature();
  const bool isValid = !isnan(humidity) && !isnan(temperature);

  return {temperature, humidity, isValid};
}
