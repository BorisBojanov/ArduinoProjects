//Score Box thingy

//List of functions we need
/*
Remember, in Arduino, micros() rolls over (goes back to zero) approximately every 70 minutes,
so if your application runs longer than that, you'll need to handle the rollover case.
*/

#include <Wire.h>
#include <Adafruit_ADS1X15.h>
#include <Arduino.h>
#include <FastLED.h>
#include <iostream>
#define NUMPIXELS 64 // number of neopixels in strip
#define DATA_PIN 12  // input pin Neopixel is attached to
#define DATA_PIN2 19 // input for the seconf Neopixel LED
#define CHIPSET WS2812B
#define COLOR_ORDER GRB
#define BRIGHTNESS 20
#define MODE_BUTTON_PIN 23
#define BUZZER_PIN 5
struct CRGB pixels[NUMPIXELS];
struct CRGB pixels222[NUMPIXELS];
int lastButtonState = LOW; // The last known state of the button
int buttonState;
volatile bool ISR_StateChanged =false;
int32_t timerStartA;
int32_t timerStartB;

Adafruit_ADS1115 ads1115_weaponONTARGET_A_B;  // ADS1115 with ADDR pin floating (default address 0x48)
Adafruit_ADS1115 ads1115_weaponOFFTARGET_A_B;     // ADS1115 with ADDR pin connected to 3.3V (address 0x49)

//Variable declaration for the switch cases
int32_t timeHitWasMade; // This is the time the hit was made
//These function return a number 1-4
uint32_t weaponA_ADS_OnTarget; // WeaponA
uint32_t lameB_ADS_OnTarget; // LameB
uint32_t weaponB_ADS_OnTarget; // WeaponB
uint32_t lameA_ADS_OnTarget; // LameA
uint32_t weaponA_Diff_lameB_OnTarget; // WeaponA - LameB : for foil and Sabre only
uint32_t weaponB_Diff_lameA_OnTarget; // WeaponB - LameA : for foil and Sabre only

uint32_t weaponA_ADS_OffTarget;
uint32_t lameB_ADS_OffTarget;
uint32_t weaponB_ADS_OffTarget;
uint32_t lameA_ADS_OffTarget;
uint32_t weaponA_Diff_lameB_OffTarget; // WeaponA - LameB : for foil and Sabre only
uint32_t weaponB_Diff_lameA_OffTarget; // WeaponB - LameA : for foil and Sabre only



struct WeaponMode {
  long lockoutTime;
  long depressTime;
  void (*handleHit)();
  String Weapon_Name;
  int Mode_INT;
};
// Representing 300 microseconds
uint32_t Foil_LOCKOUT_TIME = 300000;
// Representing 45 microseconds
uint32_t Epee_LOCKOUT_TIME = 45000;
// Representing 170 microseconds
uint32_t Sabre_LOCKOUT_TIME = 170000;
// const int32_t Foil_LOCKOUT_TIME = 300000; // 300,000 microseconds = 300 milliseconds
// const int32_t Epee_LOCKOUT_TIME = 45000; // 45,000 microseconds = 45 milliseconds
// const int32_t Sabre_LOCKOUT_TIME = 170000; // 170,000 microseconds = 170 milliseconds
int32_t Foil_DEPRESS_TIME = 2000; // 2000 microseconds = 2 milliseconds
int32_t Epee_DEPRESS_TIME = 1000; // 1000 microseconds = 1 milliseconds
int32_t Sabre_DEPRESS_TIME = 0; // 10 microseconds = .01 milliseconds
// Global variable to store the current weapon mode

int32_t Foil_lockout = 300000;
int32_t Epee_lockout = 45000;
int32_t Sabre_lockout = 120000;

int32_t Foil_Drepess = 14000;
int32_t Epee_Depress = 0;
int32_t Sabre_Depress = 10;
int handleHitOnTargetA(int32_t weaponA_ADS_reading, int32_t lameB_ADS_reading, int32_t& timeHitWasMade) { 
    if(1000 > (ads1115_weaponONTARGET_A_B.readADC_Differential_0_1())){
        return 1;
    } else {
        return 0;
    }
}
int handleHitOnTargetB(int32_t weaponB_ADS_reading, int32_t lameA_ADS_reading, int32_t& timeHitWasMade) {
    if (1000 < (ads1115_weaponONTARGET_A_B.readADC_Differential_2_3())) {
        return 2;
    } else {
        return 0;
    }
}
int handleHitOffTargetA(int32_t weaponA_ADS_reading, int32_t& timeHitWasMade) {
    if (weaponA_ADS_reading > 26100) {
        return 3;
    } else {
        return 0;
    }
}
int handleHitOffTargetB(int32_t weaponB_ADS_reading, int32_t& timeHitWasMade) { 
    if (weaponB_ADS_reading > 26100) {
        return 4;
    } else {
        return 0;
    }
}
int handleEpeeHitA(){
    if (1000 > (ads1115_weaponONTARGET_A_B.readADC_SingleEnded(0) - ads1115_weaponONTARGET_A_B.readADC_SingleEnded(3))) {
        return 1;
    } else {
        return 0;
    }
}
int handleEpeeHitB(){
    if (1000 > (ads1115_weaponONTARGET_A_B.readADC_SingleEnded(2) - ads1115_weaponONTARGET_A_B.readADC_SingleEnded(1))) {
        return 2;
    } else {
        return 0;
    }
}
void handleFoilHit(){
    Serial.println("FOIL");
};
void handleEpeeHit(){
    Serial.println("FOIL");};
void handleSabreHit(){
    Serial.println("FOIL");};
WeaponMode foilMode = {Foil_lockout, Foil_Drepess, handleFoilHit, "FOIL", 1};
WeaponMode epeeMode = {Epee_lockout, Epee_Depress, handleEpeeHit, "EPEE", 2};
WeaponMode sabreMode = {Sabre_lockout, Sabre_Depress, handleSabreHit, "SABRE", 3};
WeaponMode* currentMode = &foilMode; // Default Mode and Initializing the cariable currentMode
WeaponMode* mode = &foilMode;



/*
If this function is called repeatedly while a button is being pressed down, the mode would indeed change rapidly.

If you want the mode to change only once per button press, you'll need to implement some form of button debouncing,
which ensures that multiple activations of the switch within a certain time period are counted as a single button press.
*/
void modeChange() { 
  buttonState = LOW;
  if (currentMode == &foilMode) {
    currentMode = &epeeMode;
  } else if (currentMode == &epeeMode) {
    currentMode = &sabreMode;
  } else {
    currentMode = &foilMode;
  }
}

/*
Function to check if the time value exceeds the mode's time limit, pass in the timeValue at the time of a hit ocouring
Each time you call this function, it checks the provided timeValue against the lockoutTime of the provided mode.
If timeValue is greater than or equal to lockoutTime, it returns true; otherwise, it returns false. 

The function doesn't have any side effects (it doesn't modify any global variables or the input parameters), 
so calling it multiple times with the same parameters will always produce the same result.
*/
bool checkForLockout(int32_t timeValue, WeaponMode* mode) {
    if (mode == nullptr) {
    return false; // Handle null pointer
    }
    int32_t timeLimit = 0;
    // Determine the time limit based on the weapon mode
    switch (mode->Mode_INT){
    case 1: //FOIL
        timeLimit = mode->lockoutTime;
        break; // exit the switch statement
    case 2: //EPEE
        timeLimit = mode->lockoutTime;
        break; // exit the switch statement
    case 3:
        timeLimit = mode->lockoutTime;
        break; // exit the switch statement
    default:
        return false; // Invalid mode, return false
        break; // exit the switch statement
    }
    // Compare the time value with the time limit
    if (timeValue >= timeLimit){
    return true; // Return true if the time value is greater or equal to the time limit
    }
    else {
        //return timeValue >= timeLimit;
        return false; // Return false if the time value is less than the time limit
    }
}


bool depressTimer(int32_t timeValue, WeaponMode* mode){
    if (mode == nullptr) {
    return false; // Handle null pointer
    }
    int32_t timeLimit = 0;
    // Determine the time limit based on the weapon mode
    switch (mode->Mode_INT){
    case 1: //FOIL
        timeLimit = mode->depressTime;
        break; // exit the switch statement
    case 2: //EPEE
        timeLimit = mode->depressTime;
        break; // exit the switch statement
    case 3:
        timeLimit = mode->depressTime;
        break; // exit the switch statement
    default:
        return false; // Invalid mode, return false
        break; // exit the switch statement
    }
    // Compare the time value with the time limit
    if (timeValue >= timeLimit){
    return true; // Return true if the time value is greater or equal to the time limit
    }
    else {
        //return timeValue >= timeLimit;
        return false; // Return false if the time value is less than the time limit
    }
}

//function to check if the hit is valid and calls HitType onTargetA, onTargetB, offTargetA, offTargetB
/*
ISR_FUNCTION
Four ISR functions that will be triggered by The
Comparitor on the ADC and sends a signol on the ARLT pin to a pin of the ESP32
the OnTargetDetected_ISR will call handleHitOnTargetA

Worry is that if the tip is continously depressed the ISR will be called so frequently that the CPU is stuck.
so i should make sure that the ISR function is only called if now - the last button press is > 1000 micros
*/

bool OnTargetDetected_ISR(){
    //return true if the ISR is called
    //if last call + now is greater than 1000 microseconds
    //return false to reset the ISR
    static bool firstCall = true;  // This variable retains its value between function calls because of static
    static int32_t timerStart; // This is the signal was recieved from ARLT pin

    if (firstCall) {
        firstCall = false; // Set to false after the first call
        timerStart = micros();

        return true;
    }
    if ( micros() - timerStart >= 1000){
            firstCall = true;
            return false;
        }
}

//trying to use switch case to call the correct function for the type of hit
// int ArrayInput[4] = {input1 , input2 , input3 , input4};
// int determineHitType(int combinedInput) {
//     switch (combinedInput) {
//         case /* value for hit type 1 */:
//             return /* hit type 1 */;
//         case /* value for hit type 2 */:
//             return /* hit type 2 */;
//         // More cases...
//         default:
//             return /* default hit type */;
//     }
// }


void Hadle_OnTargetDetected(int32_t &timeHitWasMade){
    //check if the depress timer is equal to or greater than mode specific depress time
    //if it is, return true, meaning the hit is valid
    //else return false, meaning the hit is not valid
    timeHitWasMade = micros();
    if (OnTargetDetected_ISR()/*true or false*/){
        handleHitOnTargetA(weaponA_ADS_OnTarget, lameB_ADS_OnTarget, timeHitWasMade);
        handleHitOnTargetB(weaponB_ADS_OnTarget, lameA_ADS_OnTarget, timeHitWasMade);
        handleEpeeHitA();
        handleEpeeHitB();
    }
}

void BUTTON_ISR() {
  ISR_StateChanged = true;
}

bool BUTTON_Debounce(int Some_Button_PIN){
  int reading = digitalRead(Some_Button_PIN);
  static int buttonState;
  static int lastButtonState = LOW;
  static unsigned long lastDebounceTime = 0;
  int DEBOUNCE_DELAY = 50; // Adjust this value based on your specific button and requirements

  // check to see if you just pressed the button
  // (i.e. the input went from LOW to HIGH), and you've waited long enough
  // since the last press to ignore any noise:

  // If the switch changed, due to noise or pressing:
  if (reading != lastButtonState) {
    // reset the debouncing timer
    lastDebounceTime = millis();
  }
  if ((millis() - lastDebounceTime) > DEBOUNCE_DELAY) {
    // whatever the reading is at, it's been there for longer than the debounce
    // delay, so take it as the actual current state:

    // if the button state has changed:
    if (reading != buttonState) {
      buttonState = reading;

      // only toggle the LED if the new button state is HIGH
      if (buttonState == HIGH) {
        Serial.println("changed mode was called");
        return true;
      }
    }
  }
  lastButtonState = reading;
  // Return false if no change or if not meeting the conditions
  return false;
}
// Reset all variables
unsigned long lastResetTime = 0;
void resetValues() {
  unsigned long currentMillis = millis(); // get the current time
  if (currentMillis - lastResetTime >= /*LIGHTTIME*/ 2000) { // check if delayTime has passed
    // wait before turning off the buzzer
    digitalWrite(BUZZER_PIN,  LOW);
    //Removed this from all Fill color commands and placed here to see if this command is being excicuted
    fill_solid(pixels, NUMPIXELS, CRGB::Black);
    fill_solid(pixels222, NUMPIXELS, CRGB::Black);
    FastLED.show();
    lastResetTime = currentMillis; // save the last time you reset the values
  }
}

void setup(void){
    Serial.begin(115000);
    Serial.println("Hello!");
    Serial.println("Getting single-ended readings from AIN0..3");
    Serial.println("ADC Range: +/- 6.144V (1 bit = 3mV)");
    ads1115_weaponONTARGET_A_B.begin(0x49);
    ads1115_weaponOFFTARGET_A_B.begin(0x48);
    ads1115_weaponONTARGET_A_B.setGain(GAIN_ONE);
    ads1115_weaponOFFTARGET_A_B.setGain(GAIN_ONE);

    currentMode = &foilMode; // This is the weapon mode
    /*
    If weapon value is greater than 13100 then it is on target

    If weapon value is greater than 26000 then it is off target
    ADS1 Checks for onTarget PIN; 0 = WeaponA, 1 = LameA, 2 = WeaponB, 3 = LameB
    ADS2 Checks for OffTarget PIN; 0 = WeaponA, 1 = LameA, 2 = WeaponB, 3 = LameB
    */
    // Setup 3V comparator on channel 1,3 for Lame A and B for the OnTarget checks
    // Setup 3V comparator on channel 0,2 for Weapon A and B for the OffTarget checks
    //void startComparator_SingleEnded(uint8_t channel, int16_t threshold);
    //this will trigger the interrupt pin when the value is greater than the threshold on the ALRT pin
    ads1115_weaponONTARGET_A_B.startComparator_SingleEnded(1, 13100);//WeaponA would hitOnTargetA = true pin 0
    ads1115_weaponONTARGET_A_B.startComparator_SingleEnded(3, 13100);//WeaponB would hitOnTargetB = true pin 3
    ads1115_weaponOFFTARGET_A_B.startComparator_SingleEnded(0, 26100);//WeaponA would hitOffTargetA = true pin 0
    ads1115_weaponOFFTARGET_A_B.startComparator_SingleEnded(2, 26100);//WeaponB would hitOffTargetB = true pin 3
    uint32_t weaponA_ADS_OnTarget = ads1115_weaponONTARGET_A_B.readADC_SingleEnded(0); // WeaponA
    uint32_t lameB_ADS_OnTarget = ads1115_weaponONTARGET_A_B.readADC_SingleEnded(1); // LameB
    uint32_t weaponB_ADS_OnTarget = ads1115_weaponONTARGET_A_B.readADC_SingleEnded(2); // WeaponB
    uint32_t lameA_ADS_OnTarget = ads1115_weaponONTARGET_A_B.readADC_SingleEnded(3); // LameA
    uint32_t weaponA_Diff_lameB_OnTarget =ads1115_weaponONTARGET_A_B.readADC_Differential_0_1(); // WeaponA - LameB : for foil and Sabre only
    uint32_t weaponB_Diff_lameA_OnTarget =ads1115_weaponONTARGET_A_B.readADC_Differential_2_3(); // WeaponB - LameA : for foil and Sabre only

    uint32_t weaponA_ADS_OffTarget = ads1115_weaponOFFTARGET_A_B.readADC_SingleEnded(0);
    uint32_t lameB_ADS_OffTarget = ads1115_weaponOFFTARGET_A_B.readADC_SingleEnded(1);
    uint32_t weaponB_ADS_OffTarget = ads1115_weaponOFFTARGET_A_B.readADC_SingleEnded(2);
    uint32_t lameA_ADS_OffTarget = ads1115_weaponOFFTARGET_A_B.readADC_SingleEnded(3);
    uint32_t weaponA_Diff_lameB_OffTarget =ads1115_weaponOFFTARGET_A_B.readADC_Differential_0_1(); // WeaponA - LameB : for foil and Sabre only
    uint32_t weaponB_Diff_lameA_OffTarget =ads1115_weaponOFFTARGET_A_B.readADC_Differential_2_3(); // WeaponB - LameA : for foil and Sabre only
    //FAST LED SETUP
    FastLED.addLeds<CHIPSET, DATA_PIN, COLOR_ORDER>(pixels, NUMPIXELS);
    FastLED.addLeds<CHIPSET, DATA_PIN2, COLOR_ORDER>(pixels222, NUMPIXELS);
    FastLED.setBrightness(BRIGHTNESS);
    attachInterrupt(digitalPinToInterrupt(MODE_BUTTON_PIN), BUTTON_ISR, FALLING);
    resetValues();
}

void loop(void)
{
  if(BUTTON_Debounce(MODE_BUTTON_PIN)){
    modeChange();
  }

    Hadle_OnTargetDetected(timeHitWasMade);
    int hitResultA     = handleHitOnTargetA(weaponA_ADS_OnTarget, lameB_ADS_OnTarget, timeHitWasMade);
    int hitResultB     = handleHitOnTargetB(weaponB_ADS_OnTarget, lameA_ADS_OnTarget, timeHitWasMade);
    int offHitResultA  = handleHitOffTargetA(weaponA_ADS_OffTarget, timeHitWasMade);
    int offHitResultB  = handleHitOffTargetB(weaponB_ADS_OffTarget, timeHitWasMade);
    int EpeehitResultB = handleEpeeHitB();
    int EpeehitResultA = handleEpeeHitA();
    switch (mode->Mode_INT){
        case 1: //FOIL
        Serial.println("Foil Mode");
        // Check if 300,000 microseconds have elapsed
        if(!checkForLockout(timeHitWasMade, &foilMode)){break;}
            if (hitResultA == 1){    
            //onTargetA
                Serial.println("onTargetA");
                timeHitWasMade = micros();
                //turn on the LEDs
                // pixels.Color takes RGB values, from 0,0,0 up to 255,255,255
                fill_solid(pixels222, NUMPIXELS, CRGB::Green); // Moderately bright GREEN color.
                FastLED.show(); // This sends the updated pixel color to the hardware.
                digitalWrite(BUZZER_PIN, HIGH);
                depressTimer(timeHitWasMade, &foilMode);//start the depress timer
                //start the lockout timer
                if (checkForLockout(timeHitWasMade, &foilMode)){break;} // set the lockout timer and set Locked
                }
            if (hitResultB == 2){
            //onTargetB
                Serial.println("onTargetB");
                timeHitWasMade = micros();
                //turn on the LEDs
                // pixels.Color takes RGB values, from 0,0,0 up to 255,255,255
                fill_solid(pixels, NUMPIXELS, CRGB::Red); // Moderately bright RED color.
                FastLED.show(); // This sends the updated pixel color to the hardware.
                digitalWrite(BUZZER_PIN, HIGH);
                depressTimer(timeHitWasMade,  &foilMode); //start the depress timer
                //start the lockout timer
                if (checkForLockout(timeHitWasMade,  &foilMode)){break;} // set the lockout timer and set Locked
                }
            if (offHitResultA == 3){
            //offTargetA
                Serial.println("offTargetA");
                timeHitWasMade = micros();
                //turn on the LEDs
                // pixels.Color takes RGB values, from 0,0,0 up to 255,255,255
                fill_solid(pixels222, NUMPIXELS, CRGB::Yellow); // Yellow color.
                FastLED.show(); // This sends the updated pixel color to the hardware.
                digitalWrite(BUZZER_PIN, HIGH);
                depressTimer(timeHitWasMade,  &foilMode); //start the depress timer
                //start the lockout timer
                if (checkForLockout(timeHitWasMade,  &foilMode)){break;} // set the lockout timer and set Locked
                }
            if (offHitResultB == 4){
            //offTargetB
                Serial.println("offTargetB");
                timeHitWasMade = micros();
                //turn on the LEDs
                // pixels.Color takes RGB values, from 0,0,0 up to 255,255,255
                fill_solid(pixels, NUMPIXELS, CRGB::Blue); // Bright Blue color.
                FastLED.show(); // This sends the updated pixel color to the hardware.
                digitalWrite(BUZZER_PIN, HIGH);
                //start the depress timer
                depressTimer(timeHitWasMade,  &foilMode);
                //start the lockout timer
                if (!checkForLockout(timeHitWasMade,  &foilMode)){break;} // set the lockout timer and set Locked
            }
            if (hitResultA == 1 && hitResultB == 2){ //symoltanious hit ON Target
                //turn on the LEDs
                // pixels.Color takes RGB values, from 0,0,0 up to 255,255,255
                fill_solid(pixels, NUMPIXELS, CRGB::Red); // Bright RED color.
                fill_solid(pixels222, NUMPIXELS, CRGB::Green); // Moderately bright GREEN color.
                FastLED.show(); // This sends the updated pixel color to the hardware.
                digitalWrite(BUZZER_PIN, HIGH);
                //turn off the LEDs
            }
            if (offHitResultA == 3 && offHitResultB == 4){ //symoltanious hit OFF Target
                //turn on the LEDs
                // pixels.Color takes RGB values, from 0,0,0 up to 255,255,255
                fill_solid(pixels, NUMPIXELS, CRGB::Blue); // Bright Blue color.
                fill_solid(pixels222, NUMPIXELS, CRGB::Yellow); // Yellow color.
                FastLED.show(); // This sends the updated pixel color to the hardware.
                digitalWrite(BUZZER_PIN, HIGH);
                //turn off the LEDs
            }
        break;
        case 2: //EPEE
        Serial.println("Epee Mode");
        timeHitWasMade; // This is the time the hit was made
        // Call function to break out of the loop if the lockout time has been exceeded
        if(checkForLockout(timeHitWasMade, &epeeMode)){break;}
        // This is the hit type within the weapon mode
            if (EpeehitResultA == 1){    
            //onTargetA
                Serial.println("onTargetA");
                timeHitWasMade = micros();
                //turn on the LEDs
                // pixels.Color takes RGB values, from 0,0,0 up to 255,255,255
                fill_solid(pixels222, NUMPIXELS, CRGB::Green); // Moderately bright GREEN color.
                FastLED.show(); // This sends the updated pixel color to the hardware.
                digitalWrite(BUZZER_PIN, HIGH);
                depressTimer(timeHitWasMade, &epeeMode);//start the depress timer
                //start the lockout timer
                if (checkForLockout(timeHitWasMade, &epeeMode)){break;} // set the lockout timer and set Locked
                }
            if (EpeehitResultB == 2){
            //onTargetB
                Serial.println("onTargetB");
                timeHitWasMade = micros();
                //turn on the LEDs
                // pixels.Color takes RGB values, from 0,0,0 up to 255,255,255
                fill_solid(pixels, NUMPIXELS, CRGB::Red); // Moderately bright RED color.
                FastLED.show(); // This sends the updated pixel color to the hardware.
                digitalWrite(BUZZER_PIN, HIGH);
                //start the lockout timer
                if (checkForLockout(timeHitWasMade,  &epeeMode)){break;} // set the lockout timer and set Locked
                }
        break;
        case 3: //SABRE
        Serial.println("Sabre Mode");
        timeHitWasMade; // This is the time the hit was made
        // Call function to break out of the loop if the lockout time has been exceeded
        if(checkForLockout(timeHitWasMade, &sabreMode)){break;}
        // Call functions with values specific to the sabre mode
        if (hitResultA == 1){
        //onTargetA
            // This is the weapon mode
            timeHitWasMade = micros();
            //turn on the LEDs
            // pixels.Color takes RGB values, from 0,0,0 up to 255,255,255
            fill_solid(pixels222, NUMPIXELS, CRGB::Green); // Moderately bright GREEN color.
            FastLED.show(); // This sends the updated pixel color to the hardware.
            digitalWrite(BUZZER_PIN, HIGH);
            //start the depress timer
            depressTimer(timeHitWasMade, &sabreMode);
            //start the lockout timer
            if (checkForLockout(timeHitWasMade, &sabreMode)){break;} // set the lockout timer and set Locked
            }
        if (hitResultB == 2){ 
        //onTargetB
            timeHitWasMade = micros();
            //turn on the LEDs
            // pixels.Color takes RGB values, from 0,0,0 up to 255,255,255
            fill_solid(pixels, NUMPIXELS, CRGB::Red); // Moderately bright RED color.
            FastLED.show(); // This sends the updated pixel color to the hardware.
            digitalWrite(BUZZER_PIN, HIGH);
            //start the depress timer
            depressTimer(timeHitWasMade, &sabreMode);
            //start the lockout timer
            if (checkForLockout(timeHitWasMade, &sabreMode)){break;} // set the lockout timer and set Locked                                 
        }
        if (offHitResultA == 3){
        //offTargetA
            //turn on the LEDs
            // pixels.Color takes RGB values, from 0,0,0 up to 255,255,255
            fill_solid(pixels222, NUMPIXELS, CRGB::Yellow); // Yellow color.
            FastLED.show(); // This sends the updated pixel color to the hardware.
            digitalWrite(BUZZER_PIN, HIGH);
            //turn off the LEDs
        }
        if (offHitResultB == 4){
        //offTargetB
            //turn on the LEDs
            // pixels.Color takes RGB values, from 0,0,0 up to 255,255,255
            fill_solid(pixels, NUMPIXELS, CRGB::Blue); // Bright Blue color.
            FastLED.show(); // This sends the updated pixel color to the hardware.
            digitalWrite(BUZZER_PIN, HIGH);
            //turn off the LEDs
        }

        default: // Invalid weapon mode
            // Optional: handle any other cases
            break;
    }
}