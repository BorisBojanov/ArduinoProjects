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
//============
// #defines
//============

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
#define PIN       19	 // input pin LED pannel is attached to
#define PIN222    12   // input for the second LED matrix

// const uint8_t shortLEDA  =  ;    // Short Circuit A Light
// const uint8_t onTargetA  =  ;    // On Target A Light
// const uint8_t offTargetA = ;    // Off Target A Light
// const uint8_t offTargetB = ;    // Off Target B Light
// const uint8_t onTargetB  = ;    // On Target B Light
// const uint8_t shortLEDB  = ;    // Short Circuit A Light

const uint8_t groundPinA = 32;    //Green Wire - Ground A pin - Analog
const uint8_t lamePinA   = 34;    //Yellow wire -  A pin - Analog (Epee return path)
const uint8_t weaponPinA = 33;    //Blue Wire - Weapon A pin - Analog
const uint8_t weaponPinB = 2;     //Blue Wire - Weapon B pin - Analog
const uint8_t lamePinB   = 13;     //Yellow wire - Lame   B pin - Analog (Epee return path)
const uint8_t groundPinB = 15;    //Green Wire - Ground B pin - Analog

const uint8_t LEDPIN19 = 19;
const uint8_t LEDPIN12 = 12;
const uint8_t buzzerPin  =  5;    // buzzer pin

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
volatile boolean depressedA  = false;
volatile boolean depressedB  = false;
volatile boolean hitOnTargA  = false;
volatile boolean hitOffTargA = false;
volatile boolean hitOnTargB  = false;
volatile boolean hitOffTargB = false;

#ifdef TEST_ADC_SPEED
long now;
long loopCount = 0;
bool done = false;
#endif


//================
// Configuration
//================
void setup() {
   Serial.begin(BAUDRATE);
  //  Serial.println("Foil Scoring Box");
  //  Serial.println("================");

  FastLED.addLeds<WS2812B, PIN, GRB>(pixels, NUMPIXELS);
  FastLED.addLeds<WS2812B, PIN222, GRB>(pixels222, NUMPIXELS);
  // set the light pins to outputs
  //  pinMode(offTargetA, OUTPUT);
  //  pinMode(offTargetB, OUTPUT);
  //  pinMode(onTargetA,  OUTPUT);
  //  pinMode(onTargetB,  OUTPUT);
  //  pinMode(shortLEDA,  OUTPUT);
  //  pinMode(shortLEDB,  OUTPUT);
  //  pinMode(buzzerPin,  OUTPUT);



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
   // use a while as a main loop as the loop() has too much overhead for fast analogReads
   // we get a 3-4% speed up on the loop this way

    // read analog pins
    // volatile means that the arduino will always be checking this to see if these have changed.
    // but it is kind of redunant because the value is inside the void loop() which is always running 
    weaponA = analogRead(weaponPinA);
    weaponB = analogRead(weaponPinB);
    lameA   = analogRead(lamePinA);
    lameB   = analogRead(lamePinB);
     
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
	  delay(5);
}


//===================
// Main foil method
//===================
void foil() {

   long now = micros();
   if (((hitOnTargA || hitOffTargA) && (depressAtime + lockout[0] < now)) || ((hitOnTargB || hitOffTargB) && (depressBtime + lockout[0] < now))) {
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
            depressedB   = 0;
         }
      }
   }
}


//==============
// Signal Hits
/*
This Function is now redundant as of OCT 27
This function sets boolean vaues to true when it is called
 to turn on LEDs by outputting a signal that can trigger a NPN transistor
 when trying to intigrate this function with the code from the NeoPixel Library 
 It was found that to turn on the LEDs would only work within the main Void loop directly.

*/
//==============
void signalHits() {
   // non time critical, this is run after a hit has been detected
   if (lockedOut) {
    resetValues();
      // digitalWrite(onTargetA,  hitOnTargA);
      // digitalWrite(offTargetA, hitOffTargA);
      // digitalWrite(offTargetB, hitOffTargB);
      // digitalWrite(onTargetB,  hitOnTargB);
      // digitalWrite(buzzerPin,  HIGH);

      // if (hitOnTargA) {
      //   // pixels.Color takes RGB values, from 0,0,0 up to 255,255,255
      //   pixels.fill(pixels.Color(150,0,0)); // Moderately bright green color.
      //   pixels.show(); // This sends the updated pixel color to the hardware.
      //   //delay(delayval); // Delay for a period of time (in milliseconds).
      // } else {
      //   pixels.fill(pixels.Color(0,0,0));
      //   pixels.show();
      //   //delay(delayval);
      // }
      // if (hitOffTargA) {
      //   // pixels.Color takes RGB values, from 0,0,0 up to 255,255,255
      //   pixels.fill(pixels.Color(150,150,0)); // Moderately bright green color.
      //   pixels.show(); // This sends the updated pixel color to the hardware.
      //   //delay(delayval); // Delay for a period of time (in milliseconds).
      // } else {
      //   pixels.fill(pixels.Color(0,0,0));
      //   pixels.show();
      //   //delay(delayval);
      // }
      // if (hitOnTargB) {
      //   // pixels.Color takes RGB values, from 0,0,0 up to 255,255,255
      //   pixels.fill(pixels.Color(0,150,0)); // Moderately bright green color.
      //   pixels.show(); // This sends the updated pixel color to the hardware.
      //   //delay(delayval); // Delay for a period of time (in milliseconds).
      // } else {
      //   pixels.fill(pixels.Color(0,0,0));
      //   pixels.show();
      //   //delay(delayval);
      // }
      // if (hitOffTargB){
      //   // pixels.Color takes RGB values, from 0,0,0 up to 255,255,255
      //   pixels.fill(pixels.Color(0,150,150)); // Moderately bright green color.
      //   pixels.show(); // This sends the updated pixel color to the hardware.
      //   //delay(delayval); // Delay for a period of time (in milliseconds).
      // } else {
      //   pixels.fill(pixels.Color(0,0,0));
      //   pixels.show();
      //   //delay(delayval);
      // }
      
   
// #ifdef DEBUG
// String serData = String("hitOnTargA  : ") + hitOnTargA  + "\n"
//                       + "hitOffTargA : "  + hitOffTargA + "\n"
//                       + "hitOffTargB : "  + hitOffTargB + "\n"
//                       + "hitOnTargB  : "  + hitOnTargB  + "\n"
//                       + "Locked Out  : "  + lockedOut   + "\n";
// Serial.println(serData);
// #endif
      
   }
}


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




















