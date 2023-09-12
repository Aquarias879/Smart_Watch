#ifndef STEPDETECTOR_H
#define STEPDETECTOR_H

//#include <Arduino.h>
#include <Wire.h>
#include <MPU9250_asukiaaa.h>

class StepDetector {
public:
  StepDetector();
  void begin();
  void update();
  void Sleeping();
  int getStepCount();
  int getSleeping();
  int getlightSleep();
  int getdeepSleep();
  int gettotallightSleep();
  int gettotaldeepSleep();
  int getSleep();
  long getCollect();
  

private:
  void setMagMinMaxAndSetOffset(int seconds);

  MPU9250_asukiaaa mySensor; // Replace with the appropriate sensor library
  int stepThreshold;
  int stepCount;
  float vectorPrevious;
  float magXOffset;
  float magYOffset;
  float magZOffset;
  float vector;
  unsigned long timer = 0;
  long sleep_timer_start, sleep_timer_end, sleep_timer_end2;
  float x, y, z;
  int activate, interrupt, stage_sleep_time, interrupt_sleep_timer, interrupt_for_deep_sleep, total_sleep, total_deep_sleep, total_light_sleep, deep_sleep, light_sleep, interrupt_timer = 0;

};

#endif // STEPDETECTOR_H



