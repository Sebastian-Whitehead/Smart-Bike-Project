#include <Arduino.h>
#include <BleKeyboard.h>
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"


void SummonAssist(); //Function Declaration

BleKeyboard bleKeyboard("BLE Keyboard", "ESP32", 100);

void setup() {
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); //Disables burnout detector while powering the ESP32 over USB
  Serial.begin(115200);
  Serial.println("Starting BLE work!");
  bleKeyboard.begin();
}

void loop() {
  if(bleKeyboard.isConnected()) { //If bluetooth is connected
    //bleKeyboard.print("Hello world");
    Serial.println("Sending Play/Pause media key...");
    bleKeyboard.write(KEY_MEDIA_PLAY_PAUSE);
    delay(1000);

  }

  Serial.println("Waiting 5 seconds...");
  delay(5000);
}


void SummonAssist(){ // Summon Google assistant (Only Works with Android)
  bleKeyboard.press(KEY_LEFT_GUI);
  delay(2000);
  bleKeyboard.release(KEY_LEFT_GUI);
}