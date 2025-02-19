/*
All weapons
Uusing external 16bit ADCs - ADS1115
Use these ADC pins as Interrupts
Mode is eaily switched, does not slow down the system
Mode is changed through a button which is an interrupt
*/
#include <FastLED.h>
#include <Wire.h>
#include <Adafruit_ADS1X15.h>
/*
TODO: set up debug levels correctly
*/
//==========================
// TEST AND DEBUG
//==========================
  #define DEBUG 0
  #ifdef TEST_ADC_SPEED
  long now;
  long loopCount = 0;
  bool done = false;
  #endif
//==========================
// #defines
//==========================
  //#define TEST_LIGHTS       // turns on lights for a second on start up
  //#define TEST_ADC_SPEED    // used to test sample rate of ADCs
  //#define REPORT_TIMING     // prints timings over serial interface
  #define BUZZERTIME  1000  // length of time the buzzer is kept on after a hit (ms)
  #define LIGHTTIME   3000  // length of time the lights are kept on after a hit (ms)
  #define BAUDRATE   115200  // baudrate of the serial debug interface
  int delayval = 100; // timing delay in milliseconds
//=======================
// ADC Defines and info
//=======================
  // The ADC input range (or gain) can be changed via the following
  // functions, but be careful never to exceed VDD +0.3V max, or to
  // exceed the upper and lower limits if you adjust the input range!
  // Setting these values incorrectly may destroy your ADC!
  //                                                                ADS1015  ADS1115
  //                                                                -------  -------
  // ads.setGain(GAIN_TWOTHIRDS);  // 2/3x gain +/- 6.144V  1 bit = 3mV      0.1875mV (default)
  // ads.setGain(GAIN_ONE);        // 1x gain   +/- 4.096V  1 bit = 2mV      0.125mV
  // ads.setGain(GAIN_TWO);        // 2x gain   +/- 2.048V  1 bit = 1mV      0.0625mV
  // ads.setGain(GAIN_FOUR);       // 4x gain   +/- 1.024V  1 bit = 0.5mV    0.03125mV
  // ads.setGain(GAIN_EIGHT);      // 8x gain   +/- 0.512V  1 bit = 0.25mV   0.015625mV
  // ads.setGain(GAIN_SIXTEEN);    // 16x gain  +/- 0.256V  1 bit = 0.125mV  0.0078125mV
  Adafruit_ADS1115 ads1115_default;  // ADS1115 with ADDR pin floating (default address 0x48)
  Adafruit_ADS1115 ads1115_addr;     // ADS1115 with ADDR pin connected to GND
//=======================
// Fast LED Setup
//=======================
  #define NUMPIXELS 64 // number of neopixels in strip
  struct CRGB pixels[NUMPIXELS];
  struct CRGB pixels222[NUMPIXELS];
//=======================
// Pin Setup
//=======================
// Gpios 6, 7, 8, 9, 10 and 11, 12 are a no go for ESP32 to use
// Gpio 0 is always high at 4096
// Gpio 35 is always high at 4096
//=======================
  #define PIN       19	 // input pin Neopixel is attached to
  #define PIN222    12   // input for the seconf Neopixel LED

  // Short Circuit A Light
  // On Target A Light 
  // Off Target A Light
  // Off Target B Light
  // On Target B Light
  // Short Circuit A Light

  //Green Wire - Ground A pin - Analog
  //Yellow wire -  A pin - Analog (Epee return path)
  //Blue Wire - Weapon A pin - Analog
  //Blue Wire - Weapon B pin - Analog
  //Yellow wire - Lame   B pin - Analog (Epee return path)
  //Green Wire - Ground B pin - Analog

  const uint8_t LEDPIN19 = 19;
  const uint8_t LEDPIN12 = 12;
  const uint8_t buzzerPin  =  5;    // buzzer pin
  const int MODE_BUTTON_PIN    = 2; //Input
//=======================
// Interrrupt Bool
//=======================
  volatile bool weaponAChanged = false;
  volatile bool weaponBChanged = false;
//=======================
// values of analog reads
// Note: volatile is used to have the variable updated more frequently 
//       as the code is run,
//=======================
  volatile int weaponA = 0;
  volatile int weaponB = 0;
  volatile int lameA   = 0;
  volatile int lameB   = 0;
  volatile int groundA = 0;
  volatile int groundB = 0;
//=======================
// depress and lockedOut
//=======================
  long depressAtime = 0;
  long depressBtime = 0;
  bool lockedOut    = false;
//=======================
// States
//=======================
  boolean depressedA  = false;
  boolean depressedB  = false;
  boolean hitOnTargA  = false;
  boolean hitOffTargA = false;
  boolean hitOnTargB  = false;
  boolean hitOffTargB = false;
//=======================
// Weapon Modes
//=======================
  #define FOIL_MODE  0
  #define EPEE_MODE  1
  #define SABRE_MODE 2
  int currentMode = FOIL_MODE;
  volatile bool modeJustChangedFlag = false;
//=======================
// Lockout & Depress Times
//=======================
// the lockout time between hits for FOIL is 300ms +/-25ms
// the minimum amount of time the tip needs to be depressed for FOIL 14ms +/-1ms
// the lockout time between hits for EPEE is 45ms +/-5ms (40ms -> 50ms)
// the minimum amount of time the tip needs to be depressed for EPEE 2ms
// the lockout time between hits for SABRE is 120ms +/-10ms
// the minimum amount of time the tip needs to be depressed for SABRE 0.1ms -> 1ms
// These values are stored as micro seconds for more accuracy
//                         foil    epee   sabre
  const long lockout [] = {300000,  45000, 120000};  // the lockout time between hits
  const long depress [] = { 14000,   2000,   1000};  // the minimum amount of time the tip needs to be depressed

//=======================
// Change Mode
//=======================
void check_if_mode_changed(){
  if (modeJustChangedFlag) { // check if mode changed
    if(digitalRead(MODE_BUTTON_PIN)) {
      if (currentMode == SABRE_MODE) {
        currentMode = FOIL_MODE;
      } else {
        currentMode += 1;
      }
    }
    //set_mode_leds();
    Serial.print("Mode changed to:");
    Serial.println(currentMode);
  }
}
//=======================
//CLASSSSSSSSSSSSSESSSSS
//=======================


//=======================
//  ISR Functions
//=======================
/*
  =====In the void setup======
  attachInterrupt(digitalPinToInterrupt(ads1115_default.readADC_SingleEnded(0)), weaponA_ISR, CHANGE);

  =====In the void Loop======
  if (weaponAChanged) {
    weaponAChanged = false;
    handleWeaponA();
    }
*/
void weaponA_ISR() {
  weaponAChanged = true;
  if (17000 < weaponA && weaponA < 18000){
    /*
    Off target Weapon = 26500
    On target WeaponA & LameB = 8780
    no lights
    */
      handleWeaponA();
  }
}
void weaponB_ISR() {
  weaponBChanged = true;
  if (17000 < weaponB && weaponB < 18000){
      handleWeaponB();
  }
}
void change_mode_ISR() {
    modeJustChangedFlag = true;
    check_if_mode_changed();
}

void handleWeaponA() {
  //WeaponA is the analog read value of ads1115_default.readADC_SingleEnded(0)
  // Here weaponA is true?
  // what kind of ture?
  // on target true
  // if ((1000 < weaponA && weaponA < 3000) && (1000 < lameB && lameB < 3000))
  // logic for handling a change in weapon A
  long now = micros();
  if ((hitOnTargA || hitOffTargA) && (depressAtime + lockout[FOIL_MODE] < now)) {
    lockedOut = true;
      }
     // weapon A
   if (hitOnTargA == false && hitOffTargA == false) { // ignore if A has already hit
      // on target
         if ((1000 < weaponA && weaponA < 3000) && (1000 < lameB && lameB < 3000)) {
            if (!depressedA) {
               depressAtime = micros();
               depressedA   = true;
            } else {
               if (depressAtime + depress[FOIL_MODE.depresstime] <= micros()) {
                  hitOnTargA = true;
               }
            }
         } else {
            // reset these values if the depress time is short.
            depressAtime = 0;
            depressedA   = 0;
         
      }
   }
} // End of Interrupt called function
void handleWeaponB() {
  // logic for handling a change in weapon B
  long now = micros();
  if((hitOnTargB || hitOffTargB) && (depressBtime + lockout[FOIL_MODE] < now)) {
    lockedOut = true;
      }
  // weapon B
   if (hitOnTargB == false && hitOffTargB == false) { // ignore if B has already hit
      // on target
         if ((1000 < weaponB && weaponB < 3000) && (1000 < lameA && lameA < 3000)) {
            if (!depressedB) {
               depressBtime = micros();
               depressedB   = true;
            } else {
               if (depressBtime + depress[FOIL_MODE] <= micros()) {
                  hitOnTargB = true;
               }
            }
         } else {
            // reset these values if the depress time is short.
            depressBtime = 0;
            depressedB   = false;
      }
   }
}  // End of Interrupt called function
//===================
// Main foil method
//===================
void foil(){
  unsigned long now = millis(); // Arduino uses millis() to get the number of milliseconds since the board started running.
                              //It's similar to the Python monotonic_ns() function but gives time in ms not ns.
  //_____Check for lockout___
  if (((hitOnTargetA || hitOffTargetA) && (depressAtime + LOCKOUT_TIME[FOIL_MODE] < now)) ||
          ((hitOnTargetB || hitOffTargetB) && (depressBtime + LOCKOUT_TIME[FOIL_MODE] < now))) {
      lockedOut = true;
  }

  // ___Weapon A___
  if (!hitOnTargetA && !hitOffTargetA) {
      // Off target
      if (weaponAValue > 900 && lameBValue < 100) {
          if (!depressedA) {
              depressAtime = now;
              depressedA = true;
          } else if (depressAtime + DEPRESS_TIME[FOIL_MODE] <= now) {
              hitOffTargetA = true;
              Left = true;


          }
      } else {
          // On target
          if (weaponAValue > 400 && weaponAValue < 600 && lameBValue > 400 && lameBValue < 600) {
              if (!depressedA) {
                  depressAtime = now;
                  depressedA = true;
              } else if (depressAtime + DEPRESS_TIME[FOIL_MODE] <= now) {
                  hitOnTargetA = true;
              }
          } else {
              depressAtime = 0;
              depressedA = false;
          }
      }
  }

  // ___Weapon B___
  if (!hitOnTargetB && !hitOffTargetB) {
      // Off target
      if (weaponBValue > 900 && lameAValue < 100) {
          if (!depressedB) {
              depressBtime = now;
              depressedB = true;
          } else if (depressBtime + DEPRESS_TIME[FOIL_MODE] <= now) {
              hitOffTargetB = true;
              Left = true;
          }
      } else {
          // On target
          if (weaponBValue > 400 && weaponBValue < 600 && lameAValue > 400 && lameAValue < 600) {
              if (!depressedB) {
                  depressBtime = now;
                  depressedB = true;
              } else if (depressBtime + DEPRESS_TIME[FOIL_MODE] <= now) {
                  hitOnTargetB = true;
              }
          } else {
              depressBtime = 0;
              depressedB = false;
          }
      }
  }
}
//===================
// Main Epee method
//===================
void epee() {
   long now = micros();
   if ((hitOnTargA && (depressAtime + lockout[EPEE_MODE] < now)) 
      ||
      (hitOnTargB && (depressBtime + lockout[EPEE_MODE] < now))) {
      lockedOut = true;
   }

   // weapon A
   //  no hit for A yet    && weapon depress    && opponent lame touched
   if (hitOnTargA == false) {
      if (400 < weaponA && weaponA < 600 && 400 < lameA && lameA < 600) {
         if (!depressedA) {
            depressAtime = micros();
            depressedA   = true;
         } else {
            if (depressAtime + depress[1] <= micros()) {
               hitOnTargA = true;
            }
         }
      } else {
         // reset these values if the depress time is short.
         if (depressedA == true) {
            depressAtime = 0;
            depressedA   = 0;
         }
      }
   }

   // weapon B
   //  no hit for B yet    && weapon depress    && opponent lame touched
   if (hitOnTargB == false) {
      if (400 < weaponB && weaponB < 600 && 400 < lameB && lameB < 600) {
         if (!depressedB) {
            depressBtime = micros();
            depressedB   = true;
         } else {
            if (depressBtime + depress[1] <= micros()) {
               hitOnTargB = true;
            }
         }
      } else {
         // reset these values if the depress time is short.
         if (depressedB == true) {
            depressBtime = 0;
            depressedB   = 0;
         }
      }
   }
}
//===================
// Main Sabre method
//===================
void sabre() {
   long now = micros();
   if (((hitOnTargA || hitOffTargA) && (depressAtime + lockout[SABRE_MODE] < now)) || 
       ((hitOnTargB || hitOffTargB) && (depressBtime + lockout[SABRE_MODE] < now))) {
      lockedOut = true;
   }

   // weapon A
   if (hitOnTargA == false && hitOffTargA == false) { // ignore if A has already hit
      // on target
      if (400 < weaponA && weaponA < 600 && 400 < lameB && lameB < 600) {
         if (!depressedA) {
            depressAtime = micros();
            depressedA   = true;
         } else {
            if (depressAtime + depress[2] <= micros()) {
               hitOnTargA = true;
            }
         }
      } else {
         // reset these values if the depress time is short.
         depressAtime = 0;
         depressedA   = 0;
      }
   }

   // weapon B
   if (hitOnTargB == false && hitOffTargB == false) { // ignore if B has already hit
      // on target
      if (400 < weaponB && weaponB < 600 && 400 < lameA && lameA < 600) {
         if (!depressedB) {
            depressBtime = micros();
            depressedB   = true;
         } else {
            if (depressBtime + depress[2] <= micros()) {
               hitOnTargB = true;
            }
         }
      } else {
         // reset these values if the depress time is short.
         depressBtime = 0;
         depressedB   = 0;
      }
   }
}
//======================
// Reset all variables
//======================
void resetValues() {
   delay(BUZZERTIME);             // wait before turning off the buzzer
   //digitalWrite(buzzerPin,  LOW);
   delay(LIGHTTIME-BUZZERTIME);   // wait before turning off the lights
  //Removed this from all Fill color commands and placed here to see if this command is being excicuted
  fill_solid(pixels, NUMPIXELS, CRGB::Black);
  fill_solid(pixels222, NUMPIXELS, CRGB::Black);
  FastLED.show();
  lockedOut    = false;
  depressAtime = 0;
  depressedA   = false;
  depressBtime = 0;
  depressedB   = false;
  hitOnTargA  = false;
  hitOffTargA = false;
  hitOnTargB  = false;
  hitOffTargB = false;

   delay(5);
}
void setup() {
  Serial.begin(BAUDRATE);
  //  Serial.println("Foil Scoring Box");
  //  Serial.println("================");
  //ADC SETUP
  ads1115_default.begin(0x48);    // Initialize the first ADS1115 at default address 0x48 (ADDR is a Floating Pin)
  ads1115_addr.begin(0x49);       // Initialize the second ADS1115 at address 0x49 (ADDR connected to VDD aka power of ADC Pin)
  //====
  while (!Serial); // Wait for the serial port to connect

  if (!ads1115_default.begin(0x48)) {
    Serial.println("Failed to initialize ADS1115 at default (0x48!");
  }
  if (!ads1115_addr.begin(0x49)) {
    Serial.println("Failed to initialize ADS1115 at address 0x49!");
  }
  //====
  Serial.println("Getting single-ended readings from AIN0..3");
  Serial.println("ADC Range: +/- 2.048V (1 bit = 1mV/ADS1015, 0.0625mV/ADS1115)");
  // The ADC input range (or gain) can be changed via the following
  // functions, but be careful never to exceed VDD +0.3V max, or to
  // exceed the upper and lower limits if you adjust the input range!
  // Setting these values incorrectly may destroy your ADC!
  //                                                                ADS1015  ADS1115
  //                                                                -------  -------
  // ads.setGain(GAIN_TWOTHIRDS);  // 2/3x gain +/- 6.144V  1 bit = 3mV      0.1875mV (default)
  // ads.setGain(GAIN_ONE);        // 1x gain   +/- 4.096V  1 bit = 2mV      0.125mV
  // ads.setGain(GAIN_TWO);        // 2x gain   +/- 2.048V  1 bit = 1mV      0.0625mV
  // ads.setGain(GAIN_FOUR);       // 4x gain   +/- 1.024V  1 bit = 0.5mV    0.03125mV
  // ads.setGain(GAIN_EIGHT);      // 8x gain   +/- 0.512V  1 bit = 0.25mV   0.015625mV
  // ads.setGain(GAIN_SIXTEEN);    // 16x gain  +/- 0.256V  1 bit = 0.125mV  0.0078125mV
  ads1115_default.setGain(GAIN_ONE);
  ads1115_addr.setGain(GAIN_ONE);

  //FAST LED SETUP
  FastLED.addLeds<WS2812B, PIN, GRB>(pixels, NUMPIXELS);
  FastLED.addLeds<WS2812B, PIN222, GRB>(pixels222, NUMPIXELS);


  //attatch the interrupts we need to check
  attachInterrupt(digitalPinToInterrupt(ads1115_default.readADC_SingleEnded(0)), weaponA_ISR, CHANGE);
  attachInterrupt(digitalPinToInterrupt(ads1115_addr.readADC_SingleEnded(0)), weaponB_ISR, CHANGE); //SEt a different interrupt PIN
  attachInterrupt(digitalPinToInterrupt(MODE_BUTTON_PIN), change_mode_ISR, CHANGE);


  // set the light pins to outputs
  //  pinMode(offTargetA, OUTPUT);
  //  pinMode(offTargetB, OUTPUT);
  //  pinMode(onTargetA,  OUTPUT);
  //  pinMode(onTargetB,  OUTPUT);
  //  pinMode(shortLEDA,  OUTPUT);
  //  pinMode(shortLEDB,  OUTPUT);
  //  pinMode(buzzerPin,  OUTPUT);

  //====
  while (!Serial); // Wait for the serial port to connect
    if (!ads1115_default.begin(0x48)) {
    Serial.println("Failed to initialize ADS1115 at default address!");
   }
    if (!ads1115_addr.begin(0x49)) {
    Serial.println("Failed to initialize ADS1115 at address 0x49!");
   }
  //====

  #ifdef TEST_LIGHTS
   testLights();
  #endif
   resetValues();
}
void loop() {
  while (currentMode == FOIL_MODE) {
    // we want this to be our main loop
    // if we detect that the mode needs to be changed, break out of this loop
    // and go into a special function for changing modes
    // read analog pins
    int16_t adc0, adc1, adc2, adc3;
    int16_t adc4, adc5, adc6, adc7;

    float volts0, volts1, volts2, volts3;
    float volts4, volts5, volts6, volts7;

    weaponA = ads1115_default.readADC_SingleEnded(0);    // Read from channel 0 of the first ADS1115
    lameA = ads1115_default.readADC_SingleEnded(1);    // Read from channel 1 of the first ADS1115
    groundA = ads1115_default.readADC_SingleEnded(2);    // Read from channel 2 of the first ADS1115

    weaponB = ads1115_addr.readADC_SingleEnded(0);   // Read from channel 0 of the second ADS1115
    lameB = ads1115_addr.readADC_SingleEnded(1);   // Read from channel 1 of the second ADS1115
    groundB = ads1115_addr.readADC_SingleEnded(2);   // Read from channel 2 of the second ADS1115
    foil();
    if (hitOnTargA) {
      // pixels.Color takes RGB values, from 0,0,0 up to 255,255,255
      fill_solid(pixels222, NUMPIXELS, CRGB::Green); // Moderately bright GREEN color.
      FastLED.show(); // This sends the updated pixel color to the hardware.
      //delay(delayval); // Delay for a period of time (in milliseconds).
      } else {
      // fill_solid(pixels222, NUMPIXELS, CRGB::Black);
      // FastLED.show();
      //delay(delayval);
      }
    if (hitOffTargA) {
      // pixels.Color takes RGB values, from 0,0,0 up to 255,255,255
      fill_solid(pixels222, NUMPIXELS, CRGB::Yellow); // Yellow color.
      FastLED.show(); // This sends the updated pixel color to the hardware.
      //delay(delayval); // Delay for a period of time (in milliseconds).
      } else {
      // fill_solid(pixels222, NUMPIXELS, CRGB::Black);
      // FastLED.show();
      //delay(delayval
      }
    if (hitOnTargB) {
      // pixels.Color takes RGB values, from 0,0,0 up to 255,255,255
      fill_solid(pixels, NUMPIXELS, CRGB::Red); // Moderately bright RED color.
      FastLED.show(); // This sends the updated pixel color to the hardware.
      //delay(delayval); // Delay for a period of time (in milliseconds).
      } else {
      // fill_solid(pixels, NUMPIXELS, CRGB::Black);
      // FastLED.show();
      // delay(delayval);
      }
    if (hitOffTargB){
      // pixels.Color takes RGB values, from 0,0,0 up to 255,255,255
      fill_solid(pixels, NUMPIXELS, CRGB::Blue); // Bright Blue color.
      FastLED.show(); // This sends the updated pixel color to the hardware.
      //delay(delayval); // Delay for a period of time (in milliseconds).
      } else {
      // fill_solid(pixels, NUMPIXELS, CRGB::Black);
      // FastLED.show();
      // delay(delayval);
      }
    if(lockedOut){
      resetValues();
      }
    if(modeJustChangedFlag){
      modeJustChangedFlag = false;
      break;
      }
    delay(1);
  }
  while (currentMode == EPEE_MODE) {
    /*
    Epee has no Off-target Hits
    Ontarget for Epee is from Weapon A to Lame A becasue of the way Epees work
    */
    int16_t adc0, adc1, adc2, adc3;
    int16_t adc4, adc5, adc6, adc7;

    float volts0, volts1, volts2, volts3;
    float volts4, volts5, volts6, volts7;

    weaponA = ads1115_default.readADC_SingleEnded(0);    // Read from channel 0 of the first ADS1115
    lameA = ads1115_default.readADC_SingleEnded(1);    // Read from channel 1 of the first ADS1115
    groundA = ads1115_default.readADC_SingleEnded(2);    // Read from channel 2 of the first ADS1115

    weaponB = ads1115_addr.readADC_SingleEnded(0);   // Read from channel 0 of the second ADS1115
    lameB = ads1115_addr.readADC_SingleEnded(1);   // Read from channel 1 of the second ADS1115
    groundB = ads1115_addr.readADC_SingleEnded(2);   // Read from channel 2 of the second ADS1115
    epee();

    if (hitOnTargA) {
      // pixels.Color takes RGB values, from 0,0,0 up to 255,255,255
      fill_solid(pixels222, NUMPIXELS, CRGB::Green); // Moderately bright GREEN color.
      FastLED.show(); // This sends the updated pixel color to the hardware.
      //delay(delayval); // Delay for a period of time (in milliseconds).
      } else {
      // fill_solid(pixels222, NUMPIXELS, CRGB::Black);
      // FastLED.show();
      //delay(delayval);
      }

    if (hitOnTargB) {
      // pixels.Color takes RGB values, from 0,0,0 up to 255,255,255
      fill_solid(pixels, NUMPIXELS, CRGB::Red); // Moderately bright RED color.
      FastLED.show(); // This sends the updated pixel color to the hardware.
      //delay(delayval); // Delay for a period of time (in milliseconds).
      } else {
      // fill_solid(pixels, NUMPIXELS, CRGB::Black);
      // FastLED.show();
      // delay(delayval);
      }

    if(lockedOut){
      resetValues();
      }
    if(modeJustChangedFlag){
      modeJustChangedFlag = false;
      break;
      }
    delay(1);
  }
  while (currentMode == SABRE_MODE) {
    /*
    Sabre does not lock out for Off-Target Hits
    Sabre does not stop time for Off-Target Hits
    */
    int16_t adc0, adc1, adc2, adc3;
    int16_t adc4, adc5, adc6, adc7;

    float volts0, volts1, volts2, volts3;
    float volts4, volts5, volts6, volts7;

    weaponA = ads1115_default.readADC_SingleEnded(0);    // Read from channel 0 of the first ADS1115
    lameA = ads1115_default.readADC_SingleEnded(1);    // Read from channel 1 of the first ADS1115
    groundA = ads1115_default.readADC_SingleEnded(2);    // Read from channel 2 of the first ADS1115

    weaponB = ads1115_addr.readADC_SingleEnded(0);   // Read from channel 0 of the second ADS1115
    lameB = ads1115_addr.readADC_SingleEnded(1);   // Read from channel 1 of the second ADS1115
    groundB = ads1115_addr.readADC_SingleEnded(2);   // Read from channel 2 of the second ADS1115
    sabre();
    if (hitOnTargA) {
      // pixels.Color takes RGB values, from 0,0,0 up to 255,255,255
      fill_solid(pixels222, NUMPIXELS, CRGB::Green); // Moderately bright GREEN color.
      FastLED.show(); // This sends the updated pixel color to the hardware.
      //delay(delayval); // Delay for a period of time (in milliseconds).
      } else {
      // fill_solid(pixels222, NUMPIXELS, CRGB::Black);
      // FastLED.show();
      //delay(delayval);
      }
    if (hitOffTargA) {
      // pixels.Color takes RGB values, from 0,0,0 up to 255,255,255
      fill_solid(pixels222, NUMPIXELS, CRGB::Yellow); // Yellow color.
      FastLED.show(); // This sends the updated pixel color to the hardware.
      //delay(delayval); // Delay for a period of time (in milliseconds).
      } else {
      // fill_solid(pixels222, NUMPIXELS, CRGB::Black);
      // FastLED.show();
      //delay(delayval
      }
    if (hitOnTargB) {
      // pixels.Color takes RGB values, from 0,0,0 up to 255,255,255
      fill_solid(pixels, NUMPIXELS, CRGB::Red); // Moderately bright RED color.
      FastLED.show(); // This sends the updated pixel color to the hardware.
      //delay(delayval); // Delay for a period of time (in milliseconds).
      } else {
      // fill_solid(pixels, NUMPIXELS, CRGB::Black);
      // FastLED.show();
      // delay(delayval);
      }
    if (hitOffTargB){
      // pixels.Color takes RGB values, from 0,0,0 up to 255,255,255
      fill_solid(pixels, NUMPIXELS, CRGB::Blue); // Bright Blue color.
      FastLED.show(); // This sends the updated pixel color to the hardware.
      //delay(delayval); // Delay for a period of time (in milliseconds).
      } else {
      // fill_solid(pixels, NUMPIXELS, CRGB::Black);
      // FastLED.show();
      // delay(delayval);
      }
    if(lockedOut){
      resetValues();
      }
    if(modeJustChangedFlag){
      modeJustChangedFlag = false;
      break;
      }
    delay(1);
    
  }
}