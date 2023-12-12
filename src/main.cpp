#include <Arduino.h>
#include <BleKeyboard.h>
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"

//Function Declarations
void SummonAssist(); 
void Adc_FSR(int R, int L);
void UpdatePrevious(int n);
void updateRunningAverage(int reading, float &runningAverage, int &numReadings);

BleKeyboard bleKeyboard("BLE Keyboard", "ESP32", 100);
#define FORCE_SENSOR_PIN_R 35 // ESP32 pin GPIO34 (ADC0): the FSR and 10K pulldown are connected
#define FORCE_SENSOR_PIN_L 34 // ESP32 pin GPIO35 (ADC0): the FSR and 10K pulldown are connected

int LongPressDuration = 2000;
int registerDelay = 500;

// Starting values
static int Threshold_R = 3600;
static int Threshold_L = 3600;

// Calibration parameters
float calibrationFactor = 1.2;
unsigned long calibrationTime = 10000; // 10 seconds
unsigned long calibrationStartTime;

// Current and previous input states
bool State_R = false;
bool State_L = false;
bool R_Prev = false;
bool L_Prev = false;

// Start and release time of inputs
unsigned long StartTime_R;
unsigned long StartTime_L;
unsigned long ReleaseTime_R;
unsigned long ReleaseTime_L;

// Variables to store the time of the first press and the state of the button
unsigned long firstPressTime_R = 0;
unsigned long firstPressTime_L = 0;
bool doublePressDetected_R = false;
bool doublePressDetected_L = false;

// Variables for running calibration
float runningAverage_R = 0;
float runningAverage_L = 0;
int numReadings_R = 0;
int numReadings_L = 0;

// Duration of input
unsigned long durationR;
unsigned long durationL;

// Weather a given input has been processed yet or not
bool Processed = true;

// Variables for discriminating between different input types
int pressCount_L = 0;
int pressCount_R = 0;
int longPressed_R = false;
int longPressed_L = false;

void setup() {
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); // Disables burnout detector while powering the ESP32 over USB
  Serial.begin(115200);                      // Begin serial
  Serial.println("Starting BLE work!");
  bleKeyboard.begin();                       // Begin bluetooth communication
  calibrationStartTime = millis();           // Start callibration time
}


void loop() {
    // Ready analouge input values from the FSR
    int analogReading_R = analogRead(FORCE_SENSOR_PIN_R);
    int analogReading_L = analogRead(FORCE_SENSOR_PIN_L);
    
    Adc_FSR(analogReading_R, analogReading_L); //Convert analog reading to bool values based on dynamic thresholds

    // Update running averages and thresholds if a press is not detected
    if(!State_R) {
      updateRunningAverage(analogReading_R, runningAverage_R, numReadings_R);
      Threshold_R = max(int(runningAverage_R*calibrationFactor), 300);
    }
    if(!State_L) {
      updateRunningAverage(analogReading_L, runningAverage_L, numReadings_L);
      Threshold_L = max(int(runningAverage_L*calibrationFactor), 300);
    }

    // Print function
    /*Serial.print("  Analog Reading:");
    Serial.print(analogReading_L);
    Serial.print(",");
    Serial.print(analogReading_R); 
    Serial.print("  Adc Thresholds:");
    Serial.print(Threshold_L);
    Serial.print(", ");
    Serial.print(Threshold_R);
    Serial.print(" --> ");
    Serial.print(State_L);
    Serial.print(", ");
    Serial.println(State_R);
    */


  if(bleKeyboard.isConnected()) { //If bluetooth is connected

    // =================================== Skip button press detection during calibration period =================================
    if(millis() - calibrationStartTime < calibrationTime) {
      Serial.println("Callibrating");
      return;
    }

    // ==================================== Registers the begining of a press ======================================================
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



    // ============================================ Press and hold Detection ============================================================
    if (State_L && State_R){ // Both sensors held simultaniously
      unsigned long duration_R = millis() - StartTime_R;
      unsigned long duration_L = millis() - StartTime_L;

      if (duration_R >= LongPressDuration  && duration_L >= LongPressDuration){ //Press and Hold Both
        SummonAssist();
        Serial.println("Long Press Both!");
        UpdatePrevious(registerDelay);
        longPressed_L = longPressed_R = true;
        return;
      }
    } else { // If both sensors are triggered then the individual checks are skipped

      if (State_L){
        unsigned long duration = millis() - StartTime_L;

        if (duration >= LongPressDuration){  //Press and Hold Left
          bleKeyboard.write(KEY_MEDIA_VOLUME_DOWN);
          Serial.println("Long Press Left!");
          UpdatePrevious(registerDelay);
          longPressed_L = true;
          return;
        }
      }

      if(State_R){
        unsigned long duration = millis() - StartTime_R;

        if (duration >= LongPressDuration){  //Press and Hold Right
          bleKeyboard.write(KEY_MEDIA_VOLUME_UP);
          Serial.println("Long Press Right!");
          UpdatePrevious(registerDelay);
          longPressed_R = true;
          return;
        }
      }
    }
    


    // =============================================  Release detection: =========================================================
    if(!State_R && R_Prev){  // RIGHT:
      Serial.println("R Press");
      ReleaseTime_R = millis();
      durationR = millis() - StartTime_R;

      if (!longPressed_R) {   // Has a long press been registered for this input already?
        Processed = false;
        pressCount_R ++;      // increment input count
      } else {
        Processed = true;     // Unnessasary but just security for possibl input combinations.
        UpdatePrevious(0);    // Update previous input states to prevent this function from running twice on the same input
      }
      longPressed_R = false;  // Reset longpress varriable after the applicable check is passed.
    } 

    if(!State_L && L_Prev){ // LEFT:
      Serial.println("L Press");
      ReleaseTime_L = millis();
      durationL = millis() - StartTime_L;

      if (!longPressed_L) { 
        Processed = false;
        pressCount_L ++;  
      } else {
        Processed = true;
        UpdatePrevious(0);
      }
      longPressed_L = false;
    }


    // ======================== Long press protection  ======================== // 
    // Skip rest of function if a long press has been registered, reset applicable values.

    if((durationL > LongPressDuration || durationR > LongPressDuration) || (longPressed_L || longPressed_R)){
      Serial.println("Long press Protection");

      // Reset varriables and return ==> 
      longPressed_L = longPressed_R = false;
      UpdatePrevious(0);
      durationL = durationR = 0;
      return;
    }

    // Active press detection if an input is activly occuring do not continue
    if(State_L || State_R){
      return;
    }

    // ==================================== Double Click Detection ================================================== //

  // If input has yet to be processed
  if (!Processed){
    //Serial.println("Processing");

      if (firstPressTime_R == 0 && R_Prev) { // First press of double click
        firstPressTime_R = millis();
        Serial.println("FIRST PRESS R");

      } else if(!doublePressDetected_R && millis() - firstPressTime_R <= 500 && firstPressTime_R != 0 && pressCount_R >= 2) { // Second press of double click
        // This is the second press within 500ms, set the flag

        // Send bluetooth command
        doublePressDetected_R = true;
        Serial.println("Two sequential presses detected on Right!");
        bleKeyboard.write(KEY_MEDIA_NEXT_TRACK);

        // Reset varriables and return
        UpdatePrevious(registerDelay);
        Processed = true;
        pressCount_R = 0;
        return;
      }
      

      /* Debugging:
      Serial.print(!doublePressDetected_L);
      Serial.print(millis() - firstPressTime_L <= 500);
      Serial.print(!firstPressTime_L == 0);
      Serial.println(pressCount_L);
      */

      // Check if this is the first press
      if(firstPressTime_L == 0 && L_Prev) {
        firstPressTime_L = millis();
        Serial.println("FIRST PRESS L");
        
      } else if(!doublePressDetected_L && millis() - firstPressTime_L <= 500 && firstPressTime_L != 0 && pressCount_L >= 2) {
        // This is the second press within 500ms, set the flag
        doublePressDetected_L = true;
        Serial.println("Two sequential presses detected on Left!");
        bleKeyboard.write(KEY_MEDIA_PREVIOUS_TRACK);

        UpdatePrevious(registerDelay);
        Processed = true;
        pressCount_L = 0;
        return;
      }
    }


      // ========================================= Single press and Resetting Functions ===================================================== //

      // RIGHT:
      // Reset the press time and the flag if the button is released for more than 500ms
      if(!State_R && millis() - ReleaseTime_R >= 500 && firstPressTime_R != 0) {

        // If second input has not occured in half a second register single press
        if (pressCount_R == 1) {
          bleKeyboard.write(KEY_MEDIA_PLAY_PAUSE);
          Serial.println("Single Press");
        }

        // Reset applicable varribales
        Processed = true;
        firstPressTime_R = 0;
        doublePressDetected_R = false;
        Serial.println("resetting R");
        pressCount_R = 0;
      }

      // LEFT:
      if(!State_L && millis() - ReleaseTime_L >= 500 && firstPressTime_L != 0) {
        if (pressCount_L == 1) {
          bleKeyboard.write(KEY_MEDIA_PLAY_PAUSE);
          Serial.println("Single Press");
        }
        Processed = true; 
        firstPressTime_L = 0;
        doublePressDetected_L = false;
        Serial.println("resetting L");
        pressCount_L = 0;
      } 

      UpdatePrevious(0); // Reset input binary varriables
    } else {
    delay(100);
  }
}

// ============================================ Functions: ====================================================== //
// Converts analogue input to binary input value based on dynamic thresholding
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


// Summon Google assistant (Only Works with Android)
void SummonAssist(){ 
  bleKeyboard.press(KEY_LEFT_GUI);
  delay(2000);
  bleKeyboard.release(KEY_LEFT_GUI);
}


// Update varriables to detect begining and end of input + delay
void UpdatePrevious(int n){
  R_Prev = State_R;
  L_Prev = State_L;
  delay(n);
}


// Register the running average of all readings
void updateRunningAverage(int reading, float &runningAverage, int &numReadings) {
  runningAverage = ((runningAverage * numReadings) + reading) / (numReadings + 1);
  numReadings++;
}
