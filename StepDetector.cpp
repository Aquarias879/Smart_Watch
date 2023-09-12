#include "StepDetector.h"

#define CALIB_SEC 20

StepDetector::StepDetector() {
  stepThreshold = 10; // Adjust this value to set the step detection sensitivity
  //sleepThreshold = 2.0;
  stepCount = 0;
  vectorPrevious = 0;
}

void StepDetector::begin() {
#ifdef _ESP32_HAL_I2C_H_
  Wire.begin(); // SDA, SCL
#else
  Wire.begin();
#endif

  mySensor.setWire(&Wire);
  uint8_t sensorId;
  while (mySensor.readId(&sensorId) != 0) {
    Serial.println("Cannot find device to read sensorId");
    delay(2000);
  }
  mySensor.beginMag();

  Serial.println("Start scanning values of magnetometer to get offset values.");
  Serial.println("Rotate your device for " + String(CALIB_SEC) + " seconds.");
  setMagMinMaxAndSetOffset(CALIB_SEC);
  Serial.println("Finished setting offset values.");
}

void StepDetector::setMagMinMaxAndSetOffset(int seconds) {
  unsigned long calibStartAt = millis();
  float magX, magXMin, magXMax, magY, magYMin, magYMax, magZ, magZMin, magZMax;

  mySensor.magUpdate();
  magXMin = magXMax = mySensor.magX();
  magYMin = magYMax = mySensor.magY();
  magZMin = magZMax = mySensor.magZ();

  while (millis() - calibStartAt < (unsigned long)seconds * 1000) {
    delay(100);
    mySensor.magUpdate();
    magX = mySensor.magX();
    magY = mySensor.magY();
    magZ = mySensor.magZ();
    if (magX > magXMax) magXMax = magX;
    if (magY > magYMax) magYMax = magY;
    if (magZ > magZMax) magZMax = magZ;
    if (magX < magXMin) magXMin = magX;
    if (magY < magYMin) magYMin = magY;
    if (magZ < magZMin) magZMin = magZ;
  }

  mySensor.magXOffset = - (magXMax + magXMin) / 2;
  mySensor.magYOffset = - (magYMax + magYMin) / 2;
  mySensor.magZOffset = - (magZMax + magZMin) / 2;

  // Store the offset values for step detection
  magXOffset = mySensor.magXOffset;
  magYOffset = mySensor.magYOffset;
  magZOffset = mySensor.magZOffset;
}

void StepDetector::update() {
  mySensor.magUpdate();
  float mX = mySensor.magX();
  float mY = mySensor.magY();
  float mZ = mySensor.magZ();

  // Step detection algorithm
  vector = sqrt((mX * mX) + (mY * mY) + (mZ * mZ));
  float totalVector = vector - vectorPrevious;

  if (totalVector > stepThreshold) {
    stepCount++;
  }

  vectorPrevious = vector;
}

void StepDetector::Sleeping(){
  mySensor.magUpdate();

  if (millis() - timer > 1000) {
    x = mySensor.magX();
    y = mySensor.magY();
    z = mySensor.magZ();

    if (activate == 0) { // First sleep confirmation
      if ((x <= 20 || x >= -20) && (y <= 20 || y >= -20) && (z <= 20 || z >= -20)) {
        sleep_timer_start = millis() / 1000 - sleep_timer_end;
        if (sleep_timer_start == 300) {
          activate = 1;
        }
      }
      if ((x >= 20 || x <= -20) || (y >= 20 || y <= -20) || (z >= 20 || z <= -20)) {
        sleep_timer_end = (millis() / 1000);
      }
    }

    if (activate == 1) { // Sleeping mode
      light_sleep = (millis() / 1000) - sleep_timer_end;

      if (interrupt == 0) {
        if (light_sleep >= 4200) {
          if (interrupt_for_deep_sleep > 4200) {
            if (light_sleep - interrupt_sleep_timer >= 600) {
              deep_sleep = light_sleep - interrupt_for_deep_sleep;
            }
          }
        }
      }
      light_sleep = light_sleep - deep_sleep;

      if ((x >= 20 || x <= -20) || (y >= 20 || y <= -20) || (z >= 20 || z <= -20)) {
        interrupt_sleep_timer = (millis() / 1000) - sleep_timer_end;
        interrupt_for_deep_sleep = light_sleep;
        interrupt = interrupt + 1;
        delay(8000);
      }

      if ((millis() / 1000) - sleep_timer_end - interrupt_sleep_timer > 300) {
        interrupt = 0;
      }

      if ((millis() / 1000) - sleep_timer_end - interrupt_sleep_timer <= 300) {
        if (interrupt >= 5) {
          sleep_timer_end = (millis() / 1000);
          if (light_sleep >= 900) { // Second sleep confirmation
            total_light_sleep = total_light_sleep + light_sleep;
            total_deep_sleep = total_deep_sleep + deep_sleep;
            total_sleep = total_light_sleep + total_deep_sleep;
          }
          activate = 0;
          interrupt = 0;
          deep_sleep = 0;
          light_sleep = 0;
          interrupt_sleep_timer = 0;
          interrupt_for_deep_sleep = 0;
        }
      }
    }
    stage_sleep_time = light_sleep + deep_sleep;
    if (stage_sleep_time >= 5400) {
      sleep_timer_end = (millis() / 1000);
      total_light_sleep = total_light_sleep + light_sleep;
      total_deep_sleep = total_deep_sleep + deep_sleep;
      total_sleep = total_light_sleep + total_deep_sleep;
      activate = 0;
      interrupt = 0;
      deep_sleep = 0;
      light_sleep = 0;
      interrupt_sleep_timer = 0;
      interrupt_for_deep_sleep = 0;
    }
    timer = millis();
  }
}

int StepDetector::getStepCount() {
  return stepCount;
}

int StepDetector::getlightSleep() {
  int light = 0; // Initialize the variable

  if (light_sleep >= 900) {
    light = light_sleep / 60; // Update the variable if the condition is met
  }

  return light; // Return the variable
}

int StepDetector::getdeepSleep() {
  int deep = 0; // Initialize the variable

  if (deep_sleep >= 900) {
    deep = deep_sleep / 60; // Update the variable if the condition is met
  }

  return deep; // Return the variable
}


int StepDetector::gettotallightSleep() {
  return total_light_sleep / 60;
}

int StepDetector::gettotaldeepSleep() {
  return total_deep_sleep / 60;
}

int StepDetector::getSleep() {
  return total_sleep / 60;
}

long StepDetector::getCollect() {
  return sleep_timer_start;
}