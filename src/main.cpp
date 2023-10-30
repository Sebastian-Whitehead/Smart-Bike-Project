#include <Arduino.h>
#include <BleKeyboard.h>
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"

//Function Declarations
void SummonAssist(); 
void ADC_FSR();
void UpdatePrevious();

BleKeyboard bleKeyboard("BLE Keyboard", "ESP32", 100);
#define FORCE_SENSOR_PIN_R 34 // ESP32 pin GPIO34 (ADC0): the FSR and 10K pulldown are connected
#define FORCE_SENSOR_PIN_L 35 // ESP32 pin GPIO35 (ADC0): the FSR and 10K pulldown are connected

static int Threshold_R = 100;
static int Threshold_L = 100;

bool State_R = false;
bool State_L = false;
bool R_Prev = false;
bool L_Prev = false;

unsigned long StartTime_R;
unsigned long StartTime_L;
unsigned long ReleaseTime_R;
unsigned long ReleaseTime_L;

int LongPressDuration = 2000;
int registerDelay = 500;


void setup() {
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); //Disables burnout detector while powering the ESP32 over USB
  Serial.begin(115200);
  Serial.println("Starting BLE work!");
  bleKeyboard.begin();
}

void loop() {
  if(bleKeyboard.isConnected()) { //If bluetooth is connected
    //bleKeyboard.print("Hello world");
    //Serial.println("Sending Play/Pause media key...");
    //bleKeyboard.write(KEY_MEDIA_PLAY_PAUSE);
    //delay(1000);

    int analogReading_R = analogRead(FORCE_SENSOR_PIN_R);
    int analogReading_L = analogRead(FORCE_SENSOR_PIN_L);
    Serial.println(analogReading_R);
    Serial.println(analogReading_L);

    Adc_FSR(analogReading_R, analogReading_L); //Convert analog reading to bool values based on predefined thresholds
    // Registers the begining of a press
    if (State_R && !R_Prev) {
      StartTime_R = millis();
      UpdatePrevious(0);
      return;
    }

    if (State_L && !L_Prev) {
      StartTime_L = millis();
      UpdatePrevious(0);
      return;
    }


    //Press and hold Detection
    if (State_L && State_R){
      unsigned long duration = millis() - StartTime_R;
      if (duration >= LongPressDuration  && duration >= LongPressDuration){ //Press and Hold Both


        Serial.println("Long Press Both!");
        UpdatePrevious(registerDelay);
        return;
      }
    } else { // If both sensors are triggered then the individual checks are skipped

      if (State_L){
        unsigned long duration = millis() - StartTime_R;
        if (duration >= LongPressDuration){  //Press and Hold Left
          
          Serial.println("Long Press Left!");
          UpdatePrevious(registerDelay);
          return;
        }
      }

      if(State_R){
        unsigned long duration = millis() - StartTime_R;
        if (duration >= LongPressDuration){  //Press and Hold Right
          
          Serial.println("Long Press Right!");
          UpdatePrevious(registerDelay);
          return;
        }
      }
    }

    
    // Short Single Press check - 
    if(!State_R && R_Prev){
      ReleaseTime_R = millis();
      unsigned long duration = millis() - StartTime_R;

      if (duration < LongPressDuration && millis() - ReleaseTime_R >= 500){ // Once released check press duration
        //Single Press R
        bleKeyboard.write(KEY_MEDIA_PLAY_PAUSE);
        Serial.println("Press Left!");
        UpdatePrevious(registerDelay);
        return;
      }
      
    }

    
    if(!State_L && L_Prev){
      ReleaseTime_L = millis();
      unsigned long duration = millis() - StartTime_L;

      if (duration < LongPressDuration && millis() - ReleaseTime_L >= 500){ // Once released check press duration
        //Single Press L
        bleKeyboard.write(KEY_MEDIA_PLAY_PAUSE);
        Serial.println("Press Right!");
        UpdatePrevious(registerDelay);
        return;
      }

    }

    UpdatePrevious(0);
  }

  Serial.println("Waiting 5 seconds...");
  delay(5000);
}

void Adc_FSR(int R, int L){
  if(R >= Threshold_R){
    State_R = true;
  } else {
    State_R = false;
  }

  if(L >= Threshold_L){
    State_L = true;
  }else {
    State_L = false;
  }
}

void SummonAssist(){ // Summon Google assistant (Only Works with Android)
  bleKeyboard.press(KEY_LEFT_GUI);
  delay(2000);
  bleKeyboard.release(KEY_LEFT_GUI);
}

void UpdatePrevious(int n){
  R_Prev = State_R;
  L_Prev = State_L;
  delay(n);
}