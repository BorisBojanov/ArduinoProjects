/*
Make this Foil only for now
Focus on the esentials
Logic of off target and on target
Timing acuracy
LEDs turning on for set amount of time at the correct moment
Lock out after a certain amount of time
Correct power managment
Speed of baud rate
*/

#include <FastLED.h>
#include <Wire.h>
#include <Adafruit_ADS1X15.h>
//============
// #defines
//============

//ADC Defines
Adafruit_ADS1115 ads1115_default;  // ADS1115 with ADDR pin floating (default address 0x48)
Adafruit_ADS1115 ads1115_addr;     // ADS1115 with ADDR pin connected to GND

//FAST LED PINS
#define NUMPIXELS 64 // number of neopixels in strip

struct CRGB pixels[NUMPIXELS];
struct CRGB pixels222[NUMPIXELS];

//TODO: set up debug levels correctly
#define DEBUG 0

//#define TEST_LIGHTS       // turns on lights for a second on start up
//#define TEST_ADC_SPEED    // used to test sample rate of ADCs
//#define REPORT_TIMING     // prints timings over serial interface
#define BUZZERTIME  1000  // length of time the buzzer is kept on after a hit (ms)
#define LIGHTTIME   3000  // length of time the lights are kept on after a hit (ms)
#define BAUDRATE   115200  // baudrate of the serial debug interface
int delayval = 100; // timing delay in milliseconds


//============
// Pin Setup
// Gpios 6, 7, 8, 9, 10 and 11, 12 are a no go for ESP32 to use
// Gpio 0 is always high at 4096
// Gpio 35 is always high at 4096

//============
#define PIN       19	 // input pin Neopixel is attached to
#define PIN222    12   // input for the seconf Neopixel LED

// const uint8_t shortLEDA  =  ;    // Short Circuit A Light
// const uint8_t onTargetA  =  ;    // On Target A Light
// const uint8_t offTargetA = ;    // Off Target A Light
// const uint8_t offTargetB = ;    // Off Target B Light
// const uint8_t onTargetB  = ;    // On Target B Light
// const uint8_t shortLEDB  = ;    // Short Circuit A Light

// const uint8_t groundPinA = 32;    //Green Wire - Ground A pin - Analog
// const uint8_t lamePinA   = 34;    //Yellow wire -  A pin - Analog (Epee return path)
// const uint8_t weaponPinA = 33;    //Blue Wire - Weapon A pin - Analog
// const uint8_t weaponPinB = 2;     //Blue Wire - Weapon B pin - Analog
// const uint8_t lamePinB   = 13;     //Yellow wire - Lame   B pin - Analog (Epee return path)
// const uint8_t groundPinB = 15;    //Green Wire - Ground B pin - Analog

const uint8_t LEDPIN19 = 19;
const uint8_t LEDPIN12 = 12;
const uint8_t buzzerPin  =  5;    // buzzer pin
//==
// Interrrupt Bool
//==
volatile bool weaponAChanged = false;
volatile bool weaponBChanged = false;
//=========================
// values of analog reads
// Note: volatile is used to have the variable updated more frequently 
//       as the code is run,
//=========================
volatile int weaponA = 0;
volatile int weaponB = 0;
volatile int lameA   = 0;
volatile int lameB   = 0;
volatile int groundA = 0;
volatile int groundB = 0;

//=======================
// depress and timeouts
//=======================
long depressAtime = 0;
long depressBtime = 0;
bool lockedOut    = false;

//==========================
// Lockout & Depress Times
//==========================
// the lockout time between hits for foil is 300ms +/-25ms
// the minimum amount of time the tip needs to be depressed for foil 14ms +/-1ms
// the lockout time between hits for epee is 45ms +/-5ms (40ms -> 50ms)
// the minimum amount of time the tip needs to be depressed for epee 2ms
// the lockout time between hits for sabre is 120ms +/-10ms
// the minimum amount of time the tip needs to be depressed for sabre 0.1ms -> 1ms
// These values are stored as micro seconds for more accuracy
//                         foil    epee   sabre
const long lockout [] = {300000,  45000, 120000};  // the lockout time between hits
const long depress [] = { 14000,   2000,   1000};  // the minimum amount of time the tip needs to be depressed

//=========
// states
//=========
boolean depressedA  = false;
boolean depressedB  = false;
boolean hitOnTargA  = false;
boolean hitOffTargA = false;
boolean hitOnTargB  = false;
boolean hitOffTargB = false;

#ifdef TEST_ADC_SPEED
long now;
long loopCount = 0;
bool done = false;
#endif

//===================
// Main foil method
//===================
void foil() {
   long now = micros();
   if (((hitOnTargA || hitOffTargA) && (depressAtime + lockout[0] < now)) 
      || ((hitOnTargB || hitOffTargB) && (depressBtime + lockout[0] < now))) {
      lockedOut = true;
   }

   // weapon A
   if (hitOnTargA == false && hitOffTargA == false) { // ignore if A has already hit
      // off target
      if (3200 < weaponA && lameB < 100) {
         if (!depressedA) {
            depressAtime = micros();
            depressedA   = true;
         } else {
            if (depressAtime + depress[0] <= micros()) {
               hitOffTargA = true;
            }
         }
      } else {
      // on target
         if ((1000 < weaponA && weaponA < 3000) && (1000 < lameB && lameB < 3000)) {
            if (!depressedA) {
               depressAtime = micros();
               depressedA   = true;
            } else {
               if (depressAtime + depress[0] <= micros()) {
                  hitOnTargA = true;
               }
            }
         } else {
            // reset these values if the depress time is short.
            depressAtime = 0;
            depressedA   = 0;
         }
      }
   }

   // weapon B
   if (hitOnTargB == false && hitOffTargB == false) { // ignore if B has already hit
      // off target
      if (3200 < weaponB && lameA < 100) {
         if (!depressedB) {
            depressBtime = micros();
            depressedB   = true;
         } else {
            if (depressBtime + depress[0] <= micros()) {
               hitOffTargB = true;
            }
         }
      } else {
      // on target
         if ((1000 < weaponB && weaponB < 3000) && (1000 < lameA && lameA < 3000)) {
            if (!depressedB) {
               depressBtime = micros();
               depressedB   = true;
            } else {
               if (depressBtime + depress[0] <= micros()) {
                  hitOnTargB = true;
               }
            }
         } else {
            // reset these values if the depress time is short.
            depressBtime = 0;
            depressedB   = false;
         }
      }
   }
}
//===================
//  ISR Functions
//===================
/*
  =====In the void setup======
  attachInterrupt(digitalPinToInterrupt(weaponPinA), weaponA_ISR, CHANGE);

  =====In the void Loop======
  if (weaponAChanged) {
    weaponAChanged = false;
    handleWeaponA();
   }
*/
void weaponA_ISR() {
  weaponAChanged = true;
  if ((1000 < weaponA && weaponA < 3000) && (1000 < lameB && lameB < 3000)){
  handleWeaponA();
  }
}

void weaponB_ISR() {
  weaponBChanged = true;
  if ((1000 < weaponB && weaponB < 3000) && (1000 < lameA && lameA < 3000)){
    handleWeaponB();
  }
}

void handleWeaponA() {
  //WeaponA is the analog read value of WeaponPinA
  // Here weaponA is true?
  // what kind of ture?
  // on target true
  // if ((1000 < weaponA && weaponA < 3000) && (1000 < lameB && lameB < 3000))
  // Your logic for handling a change in weapon A
  long now = micros();
  if (((hitOnTargA || hitOffTargA) && (depressAtime + lockout[0] < now)) 
      || 
     ((hitOnTargB || hitOffTargB) && (depressBtime + lockout[0] < now))) {
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
               if (depressAtime + depress[0] <= micros()) {
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
  // Your logic for handling a change in weapon B
  long now = micros();
  if(((hitOnTargA || hitOffTargA) && (depressAtime + lockout[0] < now))
    || 
    ((hitOnTargB || hitOffTargB) && (depressBtime + lockout[0] < now))) {
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
               if (depressBtime + depress[0] <= micros()) {
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

//======================
// Reset all variables
//======================
void resetValues() {
   delay(BUZZERTIME);             // wait before turning off the buzzer
   //digitalWrite(buzzerPin,  LOW);
   delay(LIGHTTIME-BUZZERTIME);   // wait before turning off the lights
  //  digitalWrite(onTargetA,  LOW);
  //  digitalWrite(offTargetA, LOW);
  //  digitalWrite(offTargetB, LOW);
  //  digitalWrite(onTargetB,  LOW);
  //  digitalWrite(shortLEDA,  LOW);
  //  digitalWrite(shortLEDB,  LOW);

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

//==============
// Test lights
//==============
void testLights() {
  //  digitalWrite(offTargetA, HIGH);
  //  digitalWrite(onTargetA,  HIGH);
  //  digitalWrite(offTargetB, HIGH);
  //  digitalWrite(onTargetB,  HIGH);
  //  digitalWrite(shortLEDA,  HIGH);
  //  digitalWrite(shortLEDB,  HIGH);
   delay(10);
   resetValues();
}

//================
// Configuration
//================
void setup() {
  Serial.begin(BAUDRATE);
  //  Serial.println("Foil Scoring Box");
  //  Serial.println("================");
  //ADC SETUP
  ads1115_default.begin(0x48);    // Initialize the first ADS1115 at default address 0x48 (ADDR is a Floating Pin)
  ads1115_addr.begin(0x49);       // Initialize the second ADS1115 at address 0x49 (ADDR connected to VDD aka power of ADC Pin)
  Serial.println("Getting single-ended readings from AIN0..3");
  Serial.println("ADC Range:(GAIN_ONE); // 1x gain   +/- 4.096V  1 bit = 0.125mV");
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

   // this optimises the ADC to make the sampling rate quicker
   //adcOpt();
   resetValues();
}

//============
// Main Loop
//============
void loop() {
  int16_t adc0, adc1, adc2, adc3;
  int16_t adc4, adc5, adc6, adc7;

  float volts0, volts1, volts2, volts3;
  float volts4, volts5, volts6, volts7;

  weaponA = ads1115_default.readADC_SingleEnded(0);    // Read from channel 0 of the first ADS1115
  lameA = ads1115_default.readADC_SingleEnded(1);    // Read from channel 1 of the first ADS1115
  //groundA = ads1115_default.readADC_SingleEnded(2);    // Read from channel 2 of the first ADS1115

  weaponB = ads1115_addr.readADC_SingleEnded(0);   // Read from channel 0 of the second ADS1115
  lameB = ads1115_addr.readADC_SingleEnded(1);   // Read from channel 1 of the second ADS1115
  //groundB = ads1115_addr.readADC_SingleEnded(2);   // Read from channel 2 of the second ADS1115
  
  
  // read analog pins
    
  // Handle other tasks

  if (weaponAChanged) {
    weaponAChanged = false;
    handleWeaponA();
  }

  if (weaponBChanged) {
    weaponBChanged = false;
    handleWeaponB();
  }
  // ... rest of loop code ...

    //SERIAL PRINTS REMOVED FOR TESTING SPEED
    // Serial.print("weaponA Value:");
    // Serial.print(weaponA);
    // Serial.println();
    // Serial.print("LameB Value:");
    // Serial.print(lameB);
    // Serial.println();
    // Serial.print("weaponB Value:");
    // Serial.print(weaponB);
    // Serial.println();
    // Serial.print("LameA Value:");
    // Serial.print(lameA);
    // Serial.println();


  foil();
  if (hitOnTargA) {
    // pixels.Color takes RGB values, from 0,0,0 up to 255,255,255
    fill_solid(pixels222, NUMPIXELS, CRGB::Green); // Moderately bright GREEN color.
    FastLED.show(); // This sends the updated pixel color to the hardware.
    //delay(delayval); // Delay for a period of time (in milliseconds).
    } else {
    fill_solid(pixels222, NUMPIXELS, CRGB::Black);
    FastLED.show();
    //delay(delayval);
    }
  if (hitOffTargA) {
    // pixels.Color takes RGB values, from 0,0,0 up to 255,255,255
    fill_solid(pixels222, NUMPIXELS, CRGB::Yellow); // Yellow color.
    FastLED.show(); // This sends the updated pixel color to the hardware.
    //delay(delayval); // Delay for a period of time (in milliseconds).
    } else {
    fill_solid(pixels222, NUMPIXELS, CRGB::Black);
    FastLED.show();
    //delay(delayval
    }
  if (hitOnTargB) {
    // pixels.Color takes RGB values, from 0,0,0 up to 255,255,255
    fill_solid(pixels, NUMPIXELS, CRGB::Red); // Moderately bright RED color.
    FastLED.show(); // This sends the updated pixel color to the hardware.
    //delay(delayval); // Delay for a period of time (in milliseconds).
    } else {
    fill_solid(pixels, NUMPIXELS, CRGB::Black);
    FastLED.show();        //delay(delayval);
    }
  if (hitOffTargB){
    // pixels.Color takes RGB values, from 0,0,0 up to 255,255,255
    fill_solid(pixels, NUMPIXELS, CRGB::Blue); // Bright Blue color.
    FastLED.show(); // This sends the updated pixel color to the hardware.
    //delay(delayval); // Delay for a period of time (in milliseconds).
    } else {
    fill_solid(pixels, NUMPIXELS, CRGB::Black);
    FastLED.show();
    //delay(delayval);
    }
  //  String serData = String("hitOnTargA  : ") + hitOnTargA  + "\n"
  //                         + "hitOffTargA : "  + hitOffTargA + "\n"
  //                         + "hitOffTargB : "  + hitOffTargB + "\n"
  //                         + "hitOnTargB  : "  + hitOnTargB  + "\n"
  //                         + "Locked Out  : "  + lockedOut   + "\n";
  //  Serial.println(serData);
  if (lockedOut){
    resetValues();
    }
  delay(1);
}