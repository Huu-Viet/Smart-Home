#pragma once

#include <Arduino.h>

class MotionAlarm {
 public:
  MotionAlarm(uint8_t pirPin, uint8_t buzzerPin, bool buzzerActiveHigh,
              uint32_t warmupMs, uint32_t holdMs);

  void begin(uint32_t nowMs);
  void update(uint32_t nowMs);
  bool isReady() const;
  bool isMotionDetected() const;
  bool isAlarmActive() const;

 private:
  void setBuzzer(bool enabled);

  uint8_t pirPin_;
  uint8_t buzzerPin_;
  bool buzzerActiveHigh_;
  uint32_t warmupMs_;
  uint32_t holdMs_;
  uint32_t startedAtMs_ = 0;
  uint32_t lastMotionAtMs_ = 0;
  bool ready_ = false;
  bool buzzerEnabled_ = false;
  bool motionDetected_ = false;
  bool wasMotionDetected_ = false;
};
