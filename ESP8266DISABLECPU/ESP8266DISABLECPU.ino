#define RX_PIN 3   // GPIO3
#define TX_PIN 1   // GPIO1

void setup() {
  Serial1.begin(115200); 
  pinMode(RX_PIN, INPUT);    
  pinMode(TX_PIN, INPUT);   
  Serial1.println("Init...");

  // deepSleep time is defined in microseconds. Multiply
  // seconds by 1e6 
  // second param: WAKE_RF_DEFAULT, WAKE_RFCAL, WAKE_NO_RFCAL, WAKE_RF_DISABLED

  Serial1.println("Go to 10s deepsleep...");
  ESP.deepSleep(10 * 1000000); // 10 sec
//  ESP.deepSleep(0); // forever
  delay(1000);
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
