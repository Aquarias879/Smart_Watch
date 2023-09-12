#include <Wire.h>
#include "MAX30105.h"
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <SPI.h>
#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "Fonts/FreeMono12pt7b.h"
#include "Fonts/FreeSerif18pt7b.h"
#include "Fonts/FreeMono9pt7b.h"
#include "tftIcons/bitmaps.h"
#include "tftIcons/bitmapsLarge.h"
#include "StepDetector.h"
#include "spo2_algorithm.h"

//ESP32 Wiring  DIN 23 CLK 18 BL 19 CS 15 RST 4 DC 2

#define TFT_CS   15
#define TFT_RST  4 // Or set to -1 and connect to Arduino RESET pin
#define TFT_DC   2
#define ADC_PIN 14
#define TFT_BL 19

Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);
//Battery18650Stats battery(ADC_PIN);
StepDetector stepDetector;
MAX30105 particleSensor;

uint32_t irBuffer[BUFFER_SIZE];
uint32_t redBuffer[BUFFER_SIZE];

float previousChargeLevel = 0.0;

int32_t spo2;
int8_t validSPO2;
int32_t heartRate;
int8_t validHeartRate;

int8_t validSpO2Count = 0;
int32_t totalSpO2 = 0;
int32_t totalHr = 0;
int32_t totalTemp = 0;
int stepCount = 0;

double targetTemp = 0; // Initialize with an appropriate value
int32_t averageSpO2 = 0;
int32_t averageHR = 0;
long irVal ;

const byte RATE_SIZE = 4;
byte rates[RATE_SIZE];
byte rateSpot = 0;
long lastBeat = 0;
float beatsPerMinute;
int beatAvg;
int deepsleep,lightsleep,totallight,totaldeep,totalsleep;

unsigned long lastMeasurementTime = 0;
const unsigned long measurementInterval = 0.5 * 60 * 1000; // 5 minutes in milliseconds

void SPO2_HR_handle() {
  for (int i = 0; i < BUFFER_SIZE; i++) {
    irBuffer[i] = particleSensor.getIR(); // Update with appropriate IR data collection
    redBuffer[i] = particleSensor.getRed(); // Update with appropriate Red data collection
    delay(10); // Adjust delay based on your desired sampling frequency
  }
  // Call the SPO2 calculation function
  maxim_heart_rate_and_oxygen_saturation(irBuffer, BUFFER_SIZE, redBuffer, &spo2, &validSPO2, &heartRate, &validHeartRate);
  //delay(500);
}

void setup() {
  Serial.begin(115200);
  tft.init(240, 280); // Initialize ST7789 240x240
  tft.fillScreen(0x0000);
  testdrawtext(0,140,"Initia izing...",0xFFFF);
  tft.fillScreen(0xFFFF);
  delay(1000);
  tft.drawRGBBitmap(0, 0, logo,240 , 224);
  delay(3000);
  
  if (!particleSensor.begin(Wire, I2C_SPEED_FAST)) {
    Serial.println("MAX30105 was not found. Please check wiring/power.");
    while (1);
  }
  Serial.println("Place your index finger on the sensor with steady pressure.");

  particleSensor.setup();
  particleSensor.setPulseAmplitudeRed(0x0A);
  particleSensor.setPulseAmplitudeGreen(0);
  tft.fillScreen(0xffff);
  tft.drawRGBBitmap(0, 0, bg,240 , 280);

  xTaskCreatePinnedToCore(
    SPO2_HR_handle,
    "SPO2_HR_handle",
    10000,
    NULL,
    1,
    NULL,
    1
  );

  xTaskCreatePinnedToCore(
    displayTask,
    "displayTask",
    10000,
    NULL,
    1,
    NULL,
    1
  );
  
  stepDetector.begin();
}

void loop() {

}

void SPO2_HR_handle(void *pvParameters) {
  (void) pvParameters;
  for (;;) {
    unsigned long elapsedTime = millis() - lastMeasurementTime;
    int walking = stepDetector.getStepCount();
    targetTemp = particleSensor.readTemperature();
    stepCount = stepDetector.getStepCount();
    totalsleep = stepDetector.getSleep();
    lightsleep = stepDetector.getlightSleep();
    deepsleep = stepDetector.getdeepSleep();
    totallight = stepDetector.gettotallightSleep();
    totaldeep = stepDetector.gettotaldeepSleep();
    long collect = stepDetector.getCollect();
    //float currentChargeLevel = battery.getBatteryChargeLevel();
   
    if ((elapsedTime >= measurementInterval)) {
      tft.enableSleep(false);
      float averageSpO2 = totalSpO2;
      float averageHR = totalHr;
      
      //Serial.println(collect);
      //Serial.println(lightsleep);
    
      // Create strings with values and units
      //String battery = String(currentChargeLevel,0) + "%";
      String walktext = String(walking) + " Steps";
      String temptext = String(targetTemp, 1) + " °C";  // Temperature with one decimal place and Celsius symbol
      String spo2text = String(averageSpO2, 1) + " %";   // SpO2 with one decimal place and percentage symbol
      String hrtext = String(averageHR,1) + " bpm";      // Heart rate with beats per minute unit
      String lighttxt = "Light Sleep " + String(lightsleep);
      String deeptxt = "Deep Sleep " + String(deepsleep);

      tft.drawRGBBitmap(0, 0, bg,240 , 280);
      //pt9drawtext(160,63,battery,0xFFFF);

      // Display results here if you have a compatible display library
      Serial.println(collect);
      Serial.println(lighttxt);
      Serial.println(deeptxt);
      testdrawtext(60,95,walktext,0xFFFF); //step
      testdrawtext(60,135,temptext,0xFFFF); // temp 
      testdrawtext(60,175,hrtext,0xFFFF); // heart rate
      testdrawtext(60,215,spo2text,0xFFFF); // spo2
      // Reset values
      lastMeasurementTime = millis();
      totalSpO2 = 0;
      totalHr = 0;
      validSpO2Count = 0;
      delay(3000);
      tft.enableSleep(true);
    }
    //vTaskDelay(pdMS_TO_TICKS(500)); // Delay between measurements
  }
}

void displayTask(void *pvParameters) {
  (void) pvParameters;
  
  for (;;) {
    SPO2_HR_handle();
    stepDetector.update();
    stepDetector.Sleeping();
    
    
    // Calculate other metrics
    if (validSPO2  == 1 and validHeartRate == 1) {
      totalSpO2 = spo2;
      totalHr = heartRate;
      validSpO2Count++;
    }
    else{
      spo2 = 1;
      heartRate = 1; 
      validSpO2Count = 1;
    }
    //vTaskDelay(pdMS_TO_TICKS(500)); // Delay between display updates
  }
}

void testdrawtext(int x, int y, String text, uint16_t color) {
  tft.setFont(&FreeMono12pt7b); // Set the font
  tft.setCursor(x, y);
  tft.setTextColor(color);
  tft.setTextWrap(true);
  tft.print(text);
}

void pt9drawtext(int x, int y, String text, uint16_t color) {
  tft.setFont(&FreeMono9pt7b); // Set the font
  tft.setCursor(x, y);
  tft.setTextColor(color);
  tft.setTextWrap(true);
  tft.print(text);
}

