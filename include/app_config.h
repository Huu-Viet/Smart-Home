#pragma once

#include <Arduino.h>

namespace AppConfig {

constexpr uint8_t kDhtDataPin = 4;
constexpr uint8_t kPirPin = 27;
constexpr uint8_t kBuzzerPin = 26;

constexpr uint32_t kDhtReadIntervalMs = 2000;
constexpr uint32_t kPirWarmupMs = 60000;
constexpr uint32_t kBuzzerHoldMs = 3000;

// Most active buzzers sound when their input is HIGH. Set false for active-low
// hardware.
constexpr bool kBuzzerActiveHigh = true;

}  // namespace AppConfig
