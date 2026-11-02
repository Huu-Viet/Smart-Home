#include "motion_alarm.h"

MotionAlarm::MotionAlarm(uint8_t pirPin, uint8_t buzzerPin,
                         bool buzzerActiveHigh, uint32_t warmupMs,
                         uint32_t holdMs)
    : pirPin_(pirPin),
      buzzerPin_(buzzerPin),
      buzzerActiveHigh_(buzzerActiveHigh),
      warmupMs_(warmupMs),
      holdMs_(holdMs) {}

void MotionAlarm::begin(uint32_t nowMs) {
  pinMode(pirPin_, INPUT);
  pinMode(buzzerPin_, OUTPUT);
  setBuzzer(false);

  startedAtMs_ = nowMs;
  lastMotionAtMs_ = nowMs;
}

void MotionAlarm::update(uint32_t nowMs) {
  if (!ready_) {
    if (nowMs - startedAtMs_ < warmupMs_) {
      return;
    }

    ready_ = true;
    wasMotionDetected_ = digitalRead(pirPin_) == HIGH;
    Serial.println(F("PIR is ready."));
    return;
  }

  motionDetected_ = digitalRead(pirPin_) == HIGH;
  if (motionDetected_) {
    lastMotionAtMs_ = nowMs;
    setBuzzer(true);

    if (!wasMotionDetected_) {
      Serial.println(F("Motion detected. Buzzer alarm active."));
    }
  } else if (buzzerEnabled_ && nowMs - lastMotionAtMs_ >= holdMs_) {
    setBuzzer(false);
    Serial.println(F("No motion for hold period. Buzzer alarm stopped."));
  }

  wasMotionDetected_ = motionDetected_;
}

bool MotionAlarm::isReady() const { return ready_; }

bool MotionAlarm::isMotionDetected() const { return ready_ && motionDetected_; }

bool MotionAlarm::isAlarmActive() const { return buzzerEnabled_; }

void MotionAlarm::setBuzzer(bool enabled) {
  buzzerEnabled_ = enabled;
  const bool outputHigh = enabled == buzzerActiveHigh_;
  digitalWrite(buzzerPin_, outputHigh ? HIGH : LOW);
}
