#include <Arduino.h>

extern "C" {
#include "user_interface.h"
}

//#define USE_DEEPSLEEP

uint32_t counter;
uint32_t salt = 12345;

#define RX_PIN 3   // GPIO3
#define TX_PIN 1   // GPIO1

void setup() {
  Serial1.begin(74880); //74880 
//  pinMode(RX_PIN, INPUT);    
//  pinMode(TX_PIN, INPUT);   
  while(!Serial1) { };
  Serial1.println("Init...");
  Serial1.println("ms from start=" + String(millis()));

  uint32_t check = 0;
  ESP.rtcUserMemoryRead(0, &check, sizeof(uint32_t));
  if (check != salt) {
    ESP.rtcUserMemoryWrite(0, &salt, sizeof(uint32_t));
    counter = 0;
    Serial1.println("Error RTC memory. Counter cleared.");
  } else {
    ESP.rtcUserMemoryRead(4, &counter, sizeof(uint32_t));
    Serial1.println("RTC counter=" + String(counter));
  }
  counter ++;
  ESP.rtcUserMemoryWrite(4, &counter, sizeof(uint32_t));

  // deepSleep time is defined in microseconds. Multiply
  // seconds by 1e6 
  // second param: WAKE_RF_DEFAULT, WAKE_RFCAL, WAKE_NO_RFCAL, WAKE_RF_DISABLED

#ifdef USE_DEEPSLEEP
  Serial1.println("Go to 10s deepsleep...");
  ESP.deepSleep(10 * 1000000, WAKE_RF_DEFAULT); // 10 sec, max ~71 minutes.
//  ESP.deepSleep(0); // forever
  delay(1000);
#else
  Serial1.println("Go to 10s lightsleep...");
  wifi_set_sleep_type(LIGHT_SLEEP_T);
  delay(10 * 1000); // 10 sec
  wifi_set_sleep_type(NONE_SLEEP_T);
  delay(200);
  ESP.reset();
  delay(200);
#endif
  Serial1.println("I cant be here!!!");

  //ESP.rtcUserMemoryWrite(offset, &data, sizeof(data)) and ESP.rtcUserMemoryRead(offset, &data, sizeof(data)) 
  //allow data to be stored in and retrieved from the RTC user memory of the chip respectively. Total size of 
  //RTC user memory is 512 bytes, so offset + sizeof(data) shouldn't exceed 512. Data should be 4-byte aligned. 
  //The stored data can be retained between deep sleep cycles. However, the data might be lost after power cycling the chip.

  //http://www.espressif.com/sites/default/files/9b-esp8266-low_power_solutions_en_0.pdf

  // http://homecircuits.eu/blog/esp8266-temperature-iot-logger/
  // Required for LIGHT_SLEEP_T delay mode
  //extern "C" {
  //#include "user_interface.h"
  //}
  //wifi_set_sleep_type(LIGHT_SLEEP_T);
  //delay(60000*3-800); // loop every 3 minutes

  // deep sleep wo sleep) https://hackaday.io/project/12866-esp8266-power-latch

  // not so deep sleep (at the bottom) https://github.com/chaeplin/esp8266_and_arduino/blob/master/_48-door-alarm-deepsleep/_48-door-alarm-deepsleep.ino
}

void loop() {

  delay(1000);
}
