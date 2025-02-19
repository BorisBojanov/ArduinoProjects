#include <FastLED.h>
#include <Wire.h>
#include <Adafruit_ADS1X15.h>
//TODO: set up debug levels correctly
#define DEBUG 0
//=======================
// Fast LED Setup
//=======================
#define NUMPIXELS 64 // number of neopixels in strip
#define DATA_PIN 12  // input pin Neopixel is attached to
#define DATA_PIN2 19 // input for the seconf Neopixel LED
#define CHIPSET WS2812B
#define COLOR_ORDER GRB
#define BRIGHTNESS 20
struct CRGB pixels[NUMPIXELS];
struct CRGB pixels222[NUMPIXELS];

// ADS1015 Register Addresses, These are pointers to the address we want to access
#define ADS_A 0x48 // ADDR Connected to GND
#define ADS_B 0x49 // ADDR Connected to VCC
#define ADS_C 0x4A // ADDR Connected to SDA
#define ADS_D 0x4B // ADDR Connected to SCL
const short ConvReg     = 0b00000000; // Conversion Register Stores the result of the last conversion
const short CfgReg      = 0b00000001; // Config Register contains settings for how the ADC operates
const short LoThreshReg = 0b00000010; // Low Threshold Register
const short HiThreshReg = 0b00000011; // High Threshold Register
// Assign Variables
uint16_t RawValue_ADS_A; // Raw value from the ADC 16bit
uint16_t RawValue_ADS_B; // Raw value from the ADC 16bit
uint16_t RawValue_ADS_C; // Raw value from the ADC 16bit
uint16_t RawValue_ADS_D; // Raw value from the ADC 16bit
uint16_t ConvertedValue; // Contains the End result of the ADS rawdata
byte error; // Contains the error code

//==============================MSB=======================================================
/* Select the Most Segnificant Byte, bits 8-15
    Bits are reas from left to right after the "0b"
Bit 15    0=no effect, 1=Begin Single Conversion (in power down mode)
Bit 14    Configure pins for (0) comparitor mode or (1) single end mode
Bit 13:12 if Bit 14 is 0 then 00=A0 compared to A1. 01=A0 >A3, 10=A1>A3, 11=A2>A3. A0>A1 || A0>A3 || A1>A3 || A2>A3
          if Bit 14 is 1 then 00=A0 > GND, 01=A1 > GND, 10=A2 > GND, 11=A3 > GND
Bit 11:9  Programable Gain 000=6.144v, 001=4.096v, 010=2.048v, 011=1.024v, 100=0.512v, 101=0.256v
bit 8     0=Continuous Conversion, 1=Power Down Single Shot
*/
                    //bits:15,14,13:12,11:9,8
const short MSBcfg1 = 0b01000010; // config for single-ended Continous conversion for 4.096v Readings on A0               
const short MSBcfg2 = 0b01010010; // config for single-ended Continous conversion for 4.096v Readings on A1
const short MSBcfg3 = 0b01100010; // config for single-ended Continous conversion for 4.096v Readings on A2
const short MSBcfg4 = 0b01110010; // config for single-ended Continous conversion for 4.096v Readings on A3
//=================================LSB===================================================
/* Least Segnificant Byte, bits 0-7
    Bits are reas from left to right after the "0b"
Bits 7:5    000 : 128 SPS
            001 : 250 SPS
            010 : 490 SPS
            011 : 920 SPS
            100 : 1600 SPS (default)
            101 : 2400 SPS
            110 : 3300 SPS
            111 : 3300 SPS
Bit  4    0=Traditional Comparator with hysteresis, 1=Window Comparator
Bit  3    Alert/Ready Pin 0= Goes Low when it enters active state or 1= goes High when it enters active state
Bit  2    Alert/Ready pin Latching 0=No (pulse the Pin after each conversion), 1=Yes (stays in current state until the data is read)
Bits 1:0  Number of conversions to complete before PIN is active, 00=1, 01=2, 10=4, 11=disable comparator and set to default
*/                       //bits:7:5,4,3,2,1:0
                                    // 100       0                 1                          0                                          10
//const short LSBcfg  = 0b00001010; // 128sps, Traditional Mode, High at the end of the conversion, pulse the Pin after each conversion, 4 conversions          
                      //bits:7:5,4,3,2,1:0
                                   // 011       0                  0                          0                                  00
//const short LSBcfg = 0b01100010; // 64x sps, Traditional Mode, Low when in active state, pulse the Pin after each conversion, 1 conversions
                                 // 111       1             0                           0                                 10
  const short LSBcfg = 0b10100011;// 3300x sps, Window mode, Low when in active state, pulse the Pin after each conversion, 4 conversions
                                 // goes ACTIVE after the fourth conversion, so we do 4 confirmations.
//=======================
// GPIOs
//=======================
/*
BAD PINS
GPIO 6 to GPIO 11 are exposed in some ESP32 development boards. However, chip and are not recommended for other uses. So, don’t use these pins in your projects:

GPIO 6 (SCK/CLK)
GPIO 7 (SDO/SD0)
GPIO 8 (SDI/SD1)
GPIO 9 (SHD/SD2)
GPIO 10 (SWP/SD3)
GPIO 11 (CSC/CMD)

Good
ADC1_CH0 (GPI 36) = OnTargetA_ALRT
ADC1_CH3 (GPI 39) = OnTargetB_ALRT
ADC1_CH6 (GPI 34) = OffTargetA_ALRT
ADC1_CH7 (GPI 35) = OffTargetB_ALRT
ADC1_CH4 (GPIO 32) = LED_DATA_1
ADC1_CH5 (GPIO 33) = LED_DATA_2
UART_0_TX(GPIO 1)
UART_0_RX(GPIO 3)
VSPI MISO(GPIO 19) = Mode_Change_Button
VSPI CLK (GPIO 18)
VSPI CSO (GPIO 5)
UART_2_TX(GPIO 17)
UART_2_RX(GPIO 16)
*/
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

Adafruit_ADS1015 ads1015_A;  // ADS1115 with ADDR pin floating (default address 0x48)
Adafruit_ADS1015 ads1015_B;     // ADS1115 with ADDR pin connected to 3.3V (address 0x49)
#define BAUDRATE   115200  // baudrate of the serial debug interface
//============
// Pin Setup
// Gpios 6, 7, 8, 9, 10 and 11, 12 are a no go for ESP32 to use
// Gpio 0 is always high at 4096
// Gpio 35 is always high at 4096
//============
const uint8_t MODE_BUTTON_PIN = 13; // The pin where your mode button is connected
const uint8_t buzzerPin= 5;    // buzzer pin




//============
// ANY BUTTON DEBOUNCE
//============
const int DEBOUNCE_DELAY = 50; // The debounce delay in milliseconds
unsigned long lastDebounceTime = 0; // The last time the button state changed  
int lastButtonState = LOW; // The last known state of the button
int buttonState;
volatile bool ISR_StateChanged =false;


//=========================
// values of analog reads
// Note: volatile is used to have the variable updated more frequently 
//       as the code is run,
//=========================
int16_t weaponA = 0;
int16_t weaponB = 0;
int16_t lameA   = 0;
int16_t lameB   = 0;
int16_t groundA = 0;
int16_t groundB = 0;

// for all X: weapon >= Values >= lame  
int16_t Allowable_Deviation = 10;
//Foil Values
int16_t weapon_OffTarget_Threshold = 1660;
int16_t lame_OffTarget_Threshold = 10;
int16_t ground_OffTarget_Threshold = 10;

int16_t weapon_OnTarget_Threshold = 800; //tested value for weapon is 833.  This should be adjusted if needed
int16_t lame_OnTarget_Threshold = 850; // Tested Value for lame is 833
int16_t ground_OnTarget_Threshold = 850;
//Epee Values
int16_t Epee_Weapon_OnTarget_Threshold = 800;
int16_t Epee_Lame_OnTarget_Threshold = 800;
int16_t Epee_Ground_OnTarget_Threshold = 800;
//Sabre Values
int16_t Sabre_Weapon_OffTarget_Threshold = 1660;
int16_t Sabre_Lame_OffTarget_Threshold = 10;
int16_t Sabre_Ground_OffTarget_Threshold = 10;

<<<<<<< Updated upstream:Score_BOXXX/Score_box_V4_BetterModeChange/Score_Box_V4_Faster TouchRecognition MACBOOK.ino
  //Epee Values
  int16_t Epee_Weapon_OnTarget_Threshold = 13100;
  int16_t Epee_Lame_OnTarget_Threshold = 13100;
  int16_t Epee_Ground_OnTarget_Threshold = 13100;

  int16_t Epee_weapon_OffTarget_Threshold = 26000;
  int16_t Epee_lame_OffTarget_Threshold = 10000000;
  int16_t Epee_Ground_OffTarget_Threshold = 10696969696;

  //Sabre Values
  int16_t Sabre_weapon_OffTarget_Threshold = 26000;
  int16_t Sabre_lame_OffTarget_Threshold = 10;
  int16_t Sabre_ground_OffTarget_Threshold = 10;

  int16_t Sabre_weapon_OnTarget_Threshold = 8500;
  int16_t Sabre_lame_OnTarget_Threshold = 8500;
  int16_t Sabre_ground_OnTarget_Threshold = 8500;
=======
int16_t Sabre_Weapon_OnTarget_Threshold = 500;
int16_t Sabre_Lame_OnTarget_Threshold = 500;
int16_t Sabre_Ground_OnTarget_Threshold = 500;
>>>>>>> Stashed changes:Score_BOXXX/Score_Box_V4_Faster_TouchRecognition_MACBOOK/Score_Box_V4_Faster_TouchRecognition_MACBOOK.ino
//=======================
// Lockouts
//=======================
uint16_t HIT_CHECK_INTERVAL = 1;
uint16_t UPDAE_INTERVAL = 20;
bool lockedOut    = false;
uint32_t lastResetTime = 0;
uint32_t TimeResetWasCalled = millis();
uint16_t BUZZERTIME = 500;  // length of time the buzzer is kept on after a hit (ms)
uint16_t LIGHTTIME  = 2000;  // length of time the lights are kept on after a hit (ms)
static int TimeOfLockout = 0; // This is the time that the hit was detected
// Initialize variables to track the time of each hit
static uint32_t hitTimeA = 0;
static uint32_t hitTimeB = 0;
static uint32_t last_update = 0;
//==========================
// states
//==========================
<<<<<<< Updated upstream:Score_BOXXX/Score_box_V4_BetterModeChange/Score_Box_V4_Faster TouchRecognition MACBOOK.ino
volatile boolean depressedA  = false;
volatile boolean depressedB  = false;
volatile boolean hitOnTargetA  = false;
volatile boolean hitOffTargetA = false;
volatile boolean hitOnTargetB  = false;
volatile boolean hitOffTargetB = false;
=======
bool depressedA  = false;
bool depressedB  = false;
bool hitOnTargetA  = false;
bool hitOffTargetA = false;
bool hitOnTargetB  = false;
bool hitOffTargetB = false;
// Assign each flag a unique power of 2
const int OnTargetA_Flag_Value = 2; // 2^1
const int OnTargetB_Flag_Value = 4; // 2^2
const int OffTargetA_Flag_Value = 8; // 2^3
const int OffTargetB_Flag_Value = 16; // 2^4
short int number = (hitOnTargetA * OnTargetA_Flag_Value) +
                   (hitOnTargetB * OnTargetB_Flag_Value) +
                   (hitOffTargetA * OffTargetA_Flag_Value) +
                   (hitOffTargetB * OffTargetB_Flag_Value);
>>>>>>> Stashed changes:Score_BOXXX/Score_Box_V4_Faster_TouchRecognition_MACBOOK/Score_Box_V4_Faster_TouchRecognition_MACBOOK.ino
//==========================
//Forward Declare The functions used by struct WeaponMode
//==========================
void handleFoilHit();
void handleEpeeHit();
void handleSabreHit();
//==========================
// WeaponMode Changer
//==========================
<<<<<<< Updated upstream:Score_BOXXX/Score_box_V4_BetterModeChange/Score_Box_V4_Faster TouchRecognition MACBOOK.ino
volatile int32_t Foil_lockout = 300000;
volatile int32_t Epee_lockout = 45000;
volatile int32_t Sabre_lockout = 120000;

volatile int32_t Foil_Drepess = 14000;
volatile int32_t Epee_Depress = 0;
volatile int32_t Sabre_Depress = 10;

volatile struct WeaponMode {
=======
int ModeLED;
struct WeaponMode {
>>>>>>> Stashed changes:Score_BOXXX/Score_Box_V4_Faster_TouchRecognition_MACBOOK/Score_Box_V4_Faster_TouchRecognition_MACBOOK.ino
  long lockoutTime;
  void (*handleHit)();
  int ModeLED;
};
//==========================
// Weapon Specifiv Code
//==========================
// Lockout & Depress Times
//==========================
// the lockout time between hits for FOIL is 300ms +/-25ms
// the minimum amount of time the tip needs to be depressed for FOIL 14ms +/-1ms
// the lockout time between hits for EPEE is 45ms +/-5ms (40ms -> 50ms)
// the minimum amount of time the tip needs to be depressed for EPEE 2ms
// the lockout time between hits for SABRE is 120ms +/-10ms
// the minimum amount of time the tip needs to be depressed for SABRE 0.1ms -> 1ms
// These values are stored as micro seconds for more accuracy
<<<<<<< Updated upstream:Score_BOXXX/Score_box_V4_BetterModeChange/Score_Box_V4_Faster TouchRecognition MACBOOK.ino

WeaponMode foilMode = {Foil_lockout, Foil_Drepess, handleFoilHit,
                       Foil_weapon_OnTarget_Threshold, Foil_lame_OnTarget_Threshold, Foil_ground_OnTarget_Threshold, 
                       Foil_weapon_OffTarget_Threshold, Foil_lame_OffTarget_Threshold, Foil_ground_OffTarget_Threshold};
WeaponMode epeeMode = {Epee_lockout, Epee_Depress, handleEpeeHit, 
                       Epee_Weapon_OnTarget_Threshold, Epee_Lame_OnTarget_Threshold, Epee_Ground_OnTarget_Threshold, 
                       Epee_weapon_OffTarget_Threshold, Epee_lame_OffTarget_Threshold, Epee_Ground_OffTarget_Threshold};
WeaponMode sabreMode = {Sabre_lockout, Sabre_Depress, handleSabreHit,
                       weapon_OnTarget_Threshold, lame_OnTarget_Threshold, ground_OnTarget_Threshold, 
                       weapon_OffTarget_Threshold, lame_OffTarget_Threshold, ground_OffTarget_Threshold};
WeaponMode* currentMode = &foilMode; // Default Mode

//Returns lockedOut = true if locked out
bool CheckForLockout(boolean& hitOnTargetA, boolean& hitOffTargetA, 
                     boolean& hitOnTargetB, boolean& hitOffTargetB, 
                     long& depressAtime, long& depressBtime, 
                     WeaponMode* currentMode) {
unsigned long now = micros();
//_____Check for lockout___
if(currentMode == &foilMode){
  if (
    ((hitOnTargetA || hitOffTargetA) && (depressAtime + currentMode->lockoutTime < now)) 
                                     ||
    ((hitOnTargetB || hitOffTargetB) && (depressBtime + currentMode->lockoutTime < now))) {
    lockedOut = true;
    return true;
  }
} else {
  return false;
  lockedOut = false;
  }
}


void handleHitWeapon(int16_t weaponA, int16_t lameA, int16_t weaponB, int16_t lameB, int32_t depressTime, 
  boolean& hitOnTarget, boolean& hitOffTarget, long& depressTimeVar, 
  WeaponMode* currentMode) {
  int16_t Allowable_Deviation = 500;
  unsigned long now = micros();
  if (!hitOnTargetA && !hitOffTargetA) {
    // Off target for WeaponA`
    if ((weaponA > (currentMode -> weapon_OffTarget_Threshold) && lameB < (currentMode -> lame_OffTarget_Threshold))
                                              ||
        (weaponB > weapon_OffTarget_Threshold && lameA < lame_OffTarget_Threshold)) {
      if (!depressedA) {
        unsigned long depressTimeVar = micros();
        depressedA = true;
      } else if (depressTimeVar + currentMode->depressTime <= now) {
        hitOffTargetA = true;
      }
    } else {
      // On target for WeaponA
      if ((weaponA > (currentMode -> weapon_OnTarget_Threshold - Allowable_Deviation) 
                                        && 
          weaponA < (currentMode -> weapon_OnTarget_Threshold + Allowable_Deviation)) 
                                                                        &&
                (lameB > (lame_OnTarget_Threshold - Allowable_Deviation) && lameB < (lame_OnTarget_Threshold + Allowable_Deviation))) {
        if (!depressedA) {
          unsigned long depressTimeVar = micros();
          depressedA = true;
        } else if (depressTimeVar + currentMode->depressTime <= now) {
          hitOnTargetA = true;
        }
      } else {
        depressTimeVar = 0;
        depressedA = false;
      }
    }
  }
}

void handleFoilHit() {
  // Serial.println("Inside: Handle Foil Function");
  volatile bool hitOnTargetA;
  volatile bool hitOffTargetA;
  volatile bool hitOnTargetB;
  volatile bool hitOffTargetB;
  volatile unsigned long depressAtime;
  volatile unsigned long depressBtime;
  volatile bool lockedOut;

  // read analog pins
  int16_t weaponA, lameA, groundA, adc3;
  int16_t weaponB, lameB, groundB, adc7;
  weaponA = ads1115_A.readADC_SingleEnded(0);    // Read from channel 0 of the first ADS1115
  lameA = ads1115_A.readADC_SingleEnded(1);    // Read from channel 1 of the first ADS1115
  groundA = ads1115_A.readADC_SingleEnded(2);    // Read from channel 2 of the first ADS1115

  weaponB = ads1115_B.readADC_SingleEnded(0);   // Read from channel 0 of the second ADS1115
  lameB = ads1115_B.readADC_SingleEnded(1);   // Read from channel 1 of the second ADS1115
  groundB = ads1115_B.readADC_SingleEnded(2);   // Read from channel 2 of the second ADS1115
  // Serial.print("Default ADS1115 (0x48) weaponA 0: "); Serial.println(weaponA);
  // Serial.print("Default ADS1115 (0x48) lameA 1: "); Serial.println(lameA);
  // Serial.print("Default ADS1115 (0x48) groundA 2: "); Serial.println(groundA);
  
  // Serial.print("ADDR ADS1115 (0x49) weaponB 4: "); Serial.println(weaponB);
  // Serial.print("ADDR ADS1115 (0x49) lameB 5: "); Serial.println(lameB);
  // Serial.print("ADDR ADS1115 (0x49) groundB 6: "); Serial.println(groundB);

  if(CheckForLockout(hitOnTargetA, hitOffTargetA, hitOnTargetB, hitOffTargetB, depressAtime, depressBtime, currentMode)) {
    handleHitWeapon();
    return; //returns to the main loop if locked out
  }

  
  unsigned long now = micros(); // Arduino uses millis() to get the number of milliseconds since the board started running.
                              //It's similar to the Python monotonic_ns() function but gives time in ms not ns.
  // ___Weapon B___
  if (!hitOnTargetB && !hitOffTargetB) {
      // Off target
      if (weaponB > weapon_OffTarget_Threshold && lameA < lame_OffTarget_Threshold) {
          if (!depressedB) {
              long depressBtime = micros();
              depressedB = true;
          } else if (depressBtime + foilMode.depressTime <= now) {
              hitOffTargetB = true;
          }
      } else {
          // On target
          if (weaponB > (weapon_OnTarget_Threshold - Allowable_Deviation) && weaponB < (weapon_OnTarget_Threshold + Allowable_Deviation)
                                                                          && 
                  lameA > (lame_OnTarget_Threshold - Allowable_Deviation) && lameA < (lame_OnTarget_Threshold + Allowable_Deviation)) {
              if (!depressedB) {
                  long depressBtime = micros();
                  depressedB = true;
              } else if (depressBtime + foilMode.depressTime <= now) {
                  hitOnTargetB = true;
              }
          } else {
              depressBtime = 0;
              depressedB = false;
          }
      }
=======
WeaponMode foilMode  = {300, handleFoilHit, 0};
WeaponMode epeeMode  = {45,  handleEpeeHit, 1};
WeaponMode sabreMode = {170, handleSabreHit, 2};
WeaponMode* currentMode = &foilMode; // Default Mode

void handleFoilHit() {
  //Serial.println("Inside: Handle Foil Function");
    // int16_t weaponA, lameA, groundA, adc3;
    // int16_t weaponB, lameB, groundB, adc7;
    // weaponA = ads1015_A.readADC_SingleEnded(0);    // Read from channel 0 of the first ADS1115
    // lameA = ads1015_A.readADC_SingleEnded(1);    // Read from channel 1 of the first ADS1115
    // groundA = ads1015_A.readADC_SingleEnded(2);    // Read from channel 2 of the first ADS1115

    // weaponB = ads1015_B.readADC_SingleEnded(0);   // Read from channel 0 of the second ADS1115
    // lameB = ads1015_B.readADC_SingleEnded(1);   // Read from channel 1 of the second ADS1115
    // groundB = ads1015_B.readADC_SingleEnded(2);   // Read from channel 2 of the second ADS1115
    // Serial.print("Default ADS1115 (0x48) weaponA 0: "); Serial.println(weaponA);
    // Serial.print("Default ADS1115 (0x48) lameA 1: "); Serial.println(lameA);
    // Serial.print("Default ADS1115 (0x48) groundA 2: "); Serial.println(groundA);
    
    // Serial.print("ADDR ADS1115 (0x49) weaponB 4: "); Serial.println(weaponB);
    // Serial.print("ADDR ADS1115 (0x49) lameB 5: "); Serial.println(lameB);
    // Serial.print("ADDR ADS1115 (0x49) groundB 6: "); Serial.println(groundB);
    

    long now = millis(); // Arduino uses millis() to get the number of milliseconds since the board started running.
    // Serial.println(lockedOut);
    //_____Check for lockout___
    if (((hitOnTargetA || hitOffTargetA) && (TimeOfLockout < now)) 
                                         ||
        ((hitOnTargetB || hitOffTargetB) && (TimeOfLockout < now))) {
      lockedOut = true;
      //TimeResetWasCalled = millis();
      return; // exit the function if we are locked out
  }

  // ___Weapon A___
  if (!hitOnTargetA && !hitOffTargetA) {
    // Off target              // 1660                      // 10
    if ((weaponA > weapon_OffTarget_Threshold ) ) { //&& (lameB < lame_OffTarget_Threshold)
    hitOffTargetA = true; // hitOffTargetA
    TimeOfLockout = millis() + foilMode.lockoutTime;
    hitTimeA = now;  // Record the time of the hit
    }
    // On target
    else if ((weaponA > weapon_OnTarget_Threshold && weaponA < 900 ) && (lameB < lame_OnTarget_Threshold && lameB > 800)) { // Tested Value for lameB is 833
      hitOnTargetA = true; // hitOnTargetA
      TimeOfLockout = millis() + foilMode.lockoutTime;
      hitTimeA = now;  // Record the time of the hit
    }
  }       
  // ___Weapon B___
  if (!hitOnTargetB && !hitOffTargetB) {
    // Off target
    if ((weaponB > weapon_OffTarget_Threshold ) ) { //&& (lameA < lame_OffTarget_Threshold )
      hitOffTargetB = true; // hitOffTargetB
      TimeOfLockout = millis() + foilMode.lockoutTime;
      hitTimeB = now;  // Record the time of the hit
    // On target
    } else if ((weaponB > weapon_OnTarget_Threshold && weaponB <900) && (lameA < lame_OnTarget_Threshold && lameA > 800)) { // Tested Value for lameA is 833
      hitOnTargetB = true; // hitOnTargetB
      TimeOfLockout = millis() + foilMode.lockoutTime;
      hitTimeB = now;  // Record the time of the hit  
      //digitalWrite(buzzerPin, HIGH);
    } 
>>>>>>> Stashed changes:Score_BOXXX/Score_Box_V4_Faster_TouchRecognition_MACBOOK/Score_Box_V4_Faster_TouchRecognition_MACBOOK.ino
  }
}

void handleEpeeHit() {
  //Serial.println("Inside: Handle EPEE Function");
  // Implement the specific behavior for a hit in Epee mode
  // read analog pins
<<<<<<< Updated upstream:Score_BOXXX/Score_box_V4_BetterModeChange/Score_Box_V4_Faster TouchRecognition MACBOOK.ino
    int16_t weaponA, lameA, groundA, adc3;
    int16_t weaponB, lameB, groundB, adc7;
    weaponA = ads1115_A.readADC_SingleEnded(0);    // Read from channel 0 of the first ADS1115
    lameA = ads1115_A.readADC_SingleEnded(1);    // Read from channel 1 of the first ADS1115
    groundA = ads1115_A.readADC_SingleEnded(2);    // Read from channel 2 of the first ADS1115

    weaponB = ads1115_B.readADC_SingleEnded(0);   // Read from channel 0 of the second ADS1115
    lameB = ads1115_B.readADC_SingleEnded(1);   // Read from channel 1 of the second ADS1115
    groundB = ads1115_B.readADC_SingleEnded(2);   // Read from channel 2 of the second ADS1115
=======
    // int16_t weaponA, lameA, groundA, adc3;
    // int16_t weaponB, lameB, groundB, adc7;
    // weaponA = ads1015_A.readADC_SingleEnded(0);    // Read from channel 0 of the first ADS1115
    // lameA = ads1015_A.readADC_SingleEnded(1);    // Read from channel 1 of the first ADS1115
    // groundA = ads1015_A.readADC_SingleEnded(2);    // Read from channel 2 of the first ADS1115

    // weaponB = ads1015_B.readADC_SingleEnded(0);   // Read from channel 0 of the second ADS1115
    // lameB = ads1015_B.readADC_SingleEnded(1);   // Read from channel 1 of the second ADS1115
    // groundB = ads1015_B.readADC_SingleEnded(2);   // Read from channel 2 of the second ADS1115
    // Serial.println(lockedOut);
    // Serial.println(hitOnTargetA);
    // Serial.println(hitOnTargetB);
>>>>>>> Stashed changes:Score_BOXXX/Score_Box_V4_Faster_TouchRecognition_MACBOOK/Score_Box_V4_Faster_TouchRecognition_MACBOOK.ino
    // Serial.print("Default ADS1115 (0x48) weaponA 0: "); Serial.println(weaponA);
    // Serial.print("Default ADS1115 (0x48) lameA 1: "); Serial.println(lameA);
    // Serial.print("Default ADS1115 (0x48) groundA 2: "); Serial.println(groundA);
    
    // Serial.print("ADDR ADS1115 (0x49) weaponB 4: "); Serial.println(weaponB);
    // Serial.print("ADDR ADS1115 (0x49) lameB 5: "); Serial.println(lameB);
    // Serial.print("ADDR ADS1115 (0x49) groundB 6: "); Serial.println(groundB);
<<<<<<< Updated upstream:Score_BOXXX/Score_box_V4_BetterModeChange/Score_Box_V4_Faster TouchRecognition MACBOOK.ino

    //_____Check for lockout___
    long now = micros();
    if ((hitOnTargetA && (depressAtime + epeeMode.lockoutTime < now)) 
                      ||
        (hitOnTargetB && (depressBtime + epeeMode.lockoutTime < now))) {
      lockedOut = true;
    }

    // weapon A
    //  no hit for A yet    && weapon depress    && opponent lame touched
    if (hitOnTargetA == false) {
      if (weaponA > (Epee_Weapon_OnTarget_Threshold - Allowable_Deviation) && weaponA < (Epee_Weapon_OnTarget_Threshold + Allowable_Deviation)
                                                                      && 
              lameA > (Epee_Lame_OnTarget_Threshold - Allowable_Deviation) && lameA < (Epee_Lame_OnTarget_Threshold + Allowable_Deviation)) {
          if (!depressedA) {
            long depressAtime = micros();
            depressedA   = true;
          } else {
            if (depressAtime + epeeMode.depressTime <= micros()) {
                hitOnTargetA = true;
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
    if (hitOnTargetB == false) {
      if (weaponB > (Epee_Weapon_OnTarget_Threshold - Allowable_Deviation) && weaponB < (Epee_Weapon_OnTarget_Threshold + Allowable_Deviation)
                                                                      && 
              lameB > (Epee_Lame_OnTarget_Threshold - Allowable_Deviation) && lameB < (Epee_Lame_OnTarget_Threshold + Allowable_Deviation)) {
          if (!depressedB) {
            long depressBtime = micros();
            depressedB   = true;
          } else {
            if (depressBtime + epeeMode.depressTime <= micros()) {
                hitOnTargetB = true;
            }
          }
      } else {
          // reset these values if the depress time is short.
          if (depressedB == !true) {
            depressBtime = 0;
            depressedB   = 0;
          }
      }
    }
=======
    //_____Check for lockout___
    // Serial.println(lockedOut);
  long now = millis();
    // Check for lockout
  if (lockedOut || (hitOnTargetA && (now - hitTimeA) > epeeMode.lockoutTime) || (hitOnTargetB && (now - hitTimeB) > epeeMode.lockoutTime)) {
    lockedOut = true;
    return;
  }
  // if ((hitOnTargetA || hitOnTargetB && (TimeOfLockout < now))) {
  //   lockedOut = true;
  //   //TimeResetWasCalled = millis();
  //   return; // exit the function if we are locked out
  // }

  // weapon A
  //  no hit for A yet    && weapon depress    && opponent lame touched
  if (!hitOnTargetA) {
    if ((weaponA > Epee_Weapon_OnTarget_Threshold) && (lameA > Epee_Lame_OnTarget_Threshold)) {
      hitOnTargetA = true; // hitOnTargetA
      //TimeOfLockout = millis() + epeeMode.lockoutTime;
      hitTimeA = now;  // Record the time of the hit
    }
  }
  // weapon B
  //  no hit for B yet    && weapon depress    && opponent lame touched
  if (!hitOnTargetB) {
    if ((weaponB > Epee_Weapon_OnTarget_Threshold) && (lameB > Epee_Lame_OnTarget_Threshold )) {
      hitOnTargetB = true; // hitOnTargetB
      //TimeOfLockout = millis() + epeeMode.lockoutTime;
      hitTimeB = now;  // Record the time of the hit     
    }
  }
>>>>>>> Stashed changes:Score_BOXXX/Score_Box_V4_Faster_TouchRecognition_MACBOOK/Score_Box_V4_Faster_TouchRecognition_MACBOOK.ino
}

void handleSabreHit() {
  //Serial.println("Inside: Handle Sabre Function");
  // Implement the specific behavior for a hit in Sabre mode
  // read analog pins
<<<<<<< Updated upstream:Score_BOXXX/Score_box_V4_BetterModeChange/Score_Box_V4_Faster TouchRecognition MACBOOK.ino
    int16_t weaponA, lameA, groundA, adc3;
    int16_t weaponB, lameB, groundB, adc7;
    weaponA = ads1115_A.readADC_SingleEnded(0);    // Read from channel 0 of the first ADS1115
    lameA = ads1115_A.readADC_SingleEnded(1);    // Read from channel 1 of the first ADS1115
    groundA = ads1115_A.readADC_SingleEnded(2);    // Read from channel 2 of the first ADS1115

    weaponB = ads1115_B.readADC_SingleEnded(0);   // Read from channel 0 of the second ADS1115
    lameB = ads1115_B.readADC_SingleEnded(1);   // Read from channel 1 of the second ADS1115
    groundB = ads1115_B.readADC_SingleEnded(2);   // Read from channel 2 of the second ADS1115
  //   Serial.print("Default ADS1115 (0x48) weaponA 0: "); Serial.println(weaponA);
  //   Serial.print("Default ADS1115 (0x48) lameA 1: "); Serial.println(lameA);
  //   Serial.print("Default ADS1115 (0x48) groundA 2: "); Serial.println(groundA);
    
  //   Serial.print("ADDR ADS1115 (0x49) weaponB 4: "); Serial.println(weaponB);
  //   Serial.print("ADDR ADS1115 (0x49) lameB 5: "); Serial.println(lameB);
  //   Serial.print("ADDR ADS1115 (0x49) groundB 6: "); Serial.println(groundB);
  // //_____Check for lockout___
   long now = micros();
   if (((hitOnTargetA) && (depressAtime + sabreMode.lockoutTime < now)) 
                                        || 
       ((hitOnTargetB) && (depressBtime + sabreMode.lockoutTime < now))) {
      lockedOut = true;
   }

  // weapon A
 if (!hitOnTargetA && !hitOffTargetA) {
      // Off target
      if (weaponA > weapon_OffTarget_Threshold && lameB < lame_OffTarget_Threshold) {
          if (!depressedA) {
              long depressAtime = micros();
              depressedA = true;
          } else if (depressAtime + foilMode.depressTime <= now) {
              hitOffTargetA = true;
          }
      } else {
      // on target
      if (weaponA > (lame_OnTarget_Threshold - Allowable_Deviation) && weaponA < (lame_OnTarget_Threshold + Allowable_Deviation) 
                                                                    && 
            lameB > (lame_OnTarget_Threshold - Allowable_Deviation) && lameB < (lame_OnTarget_Threshold + Allowable_Deviation)) {
         if (!depressedA) {
            long depressAtime = micros();
            depressedA   = true;
         } else {
            if (depressAtime + sabreMode.depressTime <= micros()) {
                hitOnTargetA = true;
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
  if (!hitOnTargetB && !hitOffTargetB) {
      // Off target
      if (weaponB > weapon_OffTarget_Threshold && lameA < lame_OffTarget_Threshold) {
          if (!depressedB) {
              long depressBtime = micros();
              depressedB = true;
          } else if (depressBtime + foilMode.depressTime <= now) {
              hitOffTargetB = true;
          }
      } else {
      // on target
      if (weaponB > (lame_OnTarget_Threshold - Allowable_Deviation) && weaponB < (lame_OnTarget_Threshold + Allowable_Deviation) 
                                                                    &&
            lameA > (lame_OnTarget_Threshold - Allowable_Deviation) && lameA < (lame_OnTarget_Threshold + Allowable_Deviation)) {
         if (!depressedB) {
            long depressBtime = micros();
            depressedB   = true;
         } else {
            if (depressBtime + sabreMode.depressTime <= micros()) {
                hitOnTargetB = true;
            }
         }
      } else {
         // reset these values if the depress time is short.
         depressBtime = 0;
         depressedB   = 0;
      }
    }
=======
    // int16_t weaponA, lameA, groundA, adc3;
    // int16_t weaponB, lameB, groundB, adc7;
    // weaponA = ads1015_A.readADC_SingleEnded(0);    // Read from channel 0 of the first ADS1115
    // lameA = ads1015_A.readADC_SingleEnded(1);    // Read from channel 1 of the first ADS1115
    // groundA = ads1015_A.readADC_SingleEnded(2);    // Read from channel 2 of the first ADS1115

    // weaponB = ads1015_B.readADC_SingleEnded(0);   // Read from channel 0 of the second ADS1115
    // lameB = ads1015_B.readADC_SingleEnded(1);   // Read from channel 1 of the second ADS1115
    // groundB = ads1015_B.readADC_SingleEnded(2);   // Read from channel 2 of the second ADS1115
    // Serial.print("Default ADS1115 (0x48) weaponA 0: "); Serial.println(weaponA);
    // Serial.print("Default ADS1115 (0x48) lameA 1: "); Serial.println(lameA);
    // Serial.print("Default ADS1115 (0x48) groundA 2: "); Serial.println(groundA);
    
    // Serial.print("ADDR ADS1115 (0x49) weaponB 4: "); Serial.println(weaponB);
    // Serial.print("ADDR ADS1115 (0x49) lameB 5: "); Serial.println(lameB);
    // Serial.print("ADDR ADS1115 (0x49) groundB 6: "); Serial.println(groundB);
    //_____Check for lockout___
    long now = millis();
    // Check for lockout
  if (lockedOut || (hitOnTargetA && (now - hitTimeA) > sabreMode.lockoutTime) || (hitOnTargetB && (now - hitTimeB) > TimeOfLockout)) {
    lockedOut = true;
    return;
>>>>>>> Stashed changes:Score_BOXXX/Score_Box_V4_Faster_TouchRecognition_MACBOOK/Score_Box_V4_Faster_TouchRecognition_MACBOOK.ino
  }
  // if (((hitOnTargetA) && (TimeOfLockout < now)) 
  //                                       ||
  //     ((hitOnTargetB) && (TimeOfLockout < now))) {
  //   lockedOut = true;
  //   return; // exit the function if we are locked out
  // }
  // ___Weapon A___
  if (!hitOnTargetA || !hitOffTargetA) { // We dont need to check if we have already got a hit off target herre since the off target will not need to trigger a lockout
    // Off target
    if ((weaponA > weapon_OffTarget_Threshold) && (lameB < lame_OffTarget_Threshold)) {
      hitOffTargetA = true; // hitOffTargetA
    // On target     
    }
    else if ((weaponA > Sabre_Weapon_OnTarget_Threshold && weaponA < 700) && (lameB > Sabre_Lame_OnTarget_Threshold )) {
      hitOnTargetA =  true; // hitOnTargetA
      // TimeOfLockout = millis() + sabreMode.lockoutTime;
      hitTimeB = now;
    }
  }
  // ___Weapon B___
  if (!hitOnTargetB || !hitOffTargetB) {
    // Off target
    if ((weaponB > weapon_OffTarget_Threshold ) && (lameA < lame_OffTarget_Threshold )) {
      hitOffTargetB = true; // hitOffTargetB
    }
    // On target          // 500                                                 // 500
     else if ((weaponB > Sabre_Weapon_OnTarget_Threshold && weaponB < 700) && (lameA > Sabre_Lame_OnTarget_Threshold)) {
      hitOnTargetB = true; // hitOnTargetB
      // TimeOfLockout = millis() + sabreMode.lockoutTime;
      hitTimeB = now;  
    }
  } 
}

void handleHit() {
<<<<<<< Updated upstream:Score_BOXXX/Score_box_V4_BetterModeChange/Score_Box_V4_Faster TouchRecognition MACBOOK.ino
    currentMode->handleHit(); // will point to a Mode instance in struct Mode
=======
    currentMode->handleHit(); // will pint to a Mode instance in struct Mode
    
>>>>>>> Stashed changes:Score_BOXXX/Score_Box_V4_Faster_TouchRecognition_MACBOOK/Score_Box_V4_Faster_TouchRecognition_MACBOOK.ino
}

void modeChange() { 
  buttonState = LOW;
  if (currentMode == &foilMode) {
    //set an LED to show Mode

    pixels[0] = CRGB::Black;
    pixels[1] = CRGB::Black;
    pixels[2] = CRGB::Black;
    HardReset();
    pixels[1] = CRGB(255, 0, 255);  // Purple is a mix of full red and blue with no green

    FastLED.show();             // Display the newly-written colors
    Serial.println("changed mode was called: to Epee");
    currentMode = &epeeMode;
  } else if (currentMode == &epeeMode) {
    //set an LED to show Mode
    for (int i = 0; i <= 2; i++) {
      pixels[i] = CRGB::Black;
    }
    HardReset();
    pixels[2] = CRGB(255, 0, 255);  // Purple is a mix of full red and blue with no green
    FastLED.show();             // Display the newly-written colors
    Serial.println("changed mode was called: to Sabre");
    currentMode = &sabreMode;
  } else {
    //set an LED to show Mode
    for (int i = 0; i <= 2; i++) {
      pixels[i] = CRGB::Black;
    }
    HardReset();
    pixels[0] = CRGB(255, 0, 255);  // Purple is a mix of full red and blue with no green
    FastLED.show();             // Display the newly-written colors
    Serial.println("changed mode was called: to Foil");
    currentMode = &foilMode;
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
        return true;
      }
    }
  }
  lastButtonState = reading;
  // Return false if no change or if not meeting the conditions
  return false;
}

//======================
// Reset all variables
// The resetValues will mainly occour when the lockout time has been reached
// HardReset will be used for mode changes and its main difference is that it does not case about light time.
//======================
void HardReset(){
  digitalWrite(buzzerPin,  LOW);
  //Removed this from all Fill color commands and placed here to see if this command is being excicuted
  fill_solid(pixels, NUMPIXELS, CRGB::Black);
  fill_solid(pixels222, NUMPIXELS, CRGB::Black);
  FastLED.show();
  number = 0;
  lockedOut    = false;
  hitOnTargetA  = false;
  hitOffTargetA = false;
  hitOnTargetB  = false;
  hitOffTargetB = false;
}

void resetValues() { //might need to pass in a value when this function is called of the time that the function is called
  static unsigned long buzzerOFFTime = 0;
  static unsigned long lightsOFFTime = 0;
  static bool isBuzzerOFF = false;
  if (TimeResetWasCalled - lastResetTime >= LIGHTTIME){  // This makes sure that the buzzer is turned off
    if(!isBuzzerOFF && millis() - lastResetTime >= BUZZERTIME){
      digitalWrite(buzzerPin,  LOW);
      buzzerOFFTime = millis();
      isBuzzerOFF = true;
    }
      //Before the if statement the LEDS are ON and the Buzzer is OFF
    if (isBuzzerOFF && millis() - buzzerOFFTime >= LIGHTTIME - BUZZERTIME){
      fill_solid(pixels, NUMPIXELS, CRGB::Black);
      fill_solid(pixels222, NUMPIXELS, CRGB::Black);
      FastLED.show();
      number = 0;
      lockedOut     = false;
      hitOnTargetA  = false;
      hitOffTargetA = false;
      hitOnTargetB  = false;
      hitOffTargetB = false;
      lastResetTime = TimeResetWasCalled;
      for (int i = 0; i <= 2; i++) {
          pixels[i] = CRGB::Black;
      }
      pixels[currentMode->ModeLED] = CRGB(255, 0, 255); // Turn on the light for the current mode
      FastLED.show();
      isBuzzerOFF = false;
    }
  }
}

void setup() {
  Serial.begin(BAUDRATE);
  Wire.setClock(400000UL); // Increase I2C clock speed to 400kHz UL is to tell the compiler that it is an unsigned long
  Serial.println("Score_box_V4_BetterModeChange");
  Serial.println("Getting single-ended readings from AIN0..3");
  Serial.println("ADC GAIN_ONE Range: +/- 4.096V (1 bit = 0.125mV/ADS1115)");
  pinMode(MODE_BUTTON_PIN, INPUT_PULLDOWN);
  pinMode(buzzerPin, OUTPUT);
  modeChange(); // Epee on start up
  modeChange(); //Sabre on start up
  modeChange(); // set to foil on start up
  int LO = 0b100000000000; // Vin 3.3 R1 = 1000 R2 = 1000
  int HI = 0b11111111111; // Vout should be 1.65V
  Wire.begin();
  ads1015_A.begin(0x48);
  ads1015_B.begin(0x49);
  //  Serial.println("Three Weapon Scoring Box");
  //  Serial.println("================");
  //=========
  //ADC SETUP
  //=========
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
  ads1015_A.setGain(GAIN_ONE); // 1x gain   +/- 4.096V  1 bit = 2mV      0.125mV
  ads1015_B.setGain(GAIN_ONE); // 1x gain   +/- 4.096V  1 bit = 2mV      0.125mV
  ads1015_A.setDataRate(RATE_ADS1015_3300SPS); // 3300 samples per second 
  ads1015_B.setDataRate(RATE_ADS1015_3300SPS); // 3300 samples per second

  Serial.println(ads1015_A.getDataRate()); 
  Serial.println(ads1015_B.getDataRate());

  //FAST LED SETUP
  FastLED.addLeds<CHIPSET, DATA_PIN, COLOR_ORDER>(pixels, NUMPIXELS);
  FastLED.addLeds<CHIPSET, DATA_PIN2, COLOR_ORDER>(pixels222, NUMPIXELS);
  FastLED.setBrightness(BRIGHTNESS);
  //attatch the interrupts we need to check
  //attachInterrupt(digitalPinToInterrupt(ads1115_A.readADC_SingleEnded(0)), handleHit, CHANGE);
  //attachInterrupt(digitalPinToInterrupt(ads1115_B.readADC_SingleEnded(0)), handleHit, CHANGE); //SEt a different interrupt PIN
  attachInterrupt(digitalPinToInterrupt(MODE_BUTTON_PIN), BUTTON_ISR, FALLING);
  resetValues();
}

void loop() {
<<<<<<< Updated upstream:Score_BOXXX/Score_box_V4_BetterModeChange/Score_Box_V4_Faster TouchRecognition MACBOOK.ino
  volatile bool hitOnTargetA;
  volatile bool hitOffTargetA;
  volatile bool hitOnTargetB;
  volatile bool hitOffTargetB;
  volatile unsigned long depressAtime;
  volatile unsigned long depressBtime;
  volatile bool lockedOut;
  //main code here, to run repeatedly:
=======
  unsigned long now = millis();

  weaponA = ads1015_A.readADC_SingleEnded(0);    // Read from channel 0 of the first ADS1115
  lameA = ads1015_A.readADC_SingleEnded(1);    // Read from channel 1 of the first ADS1115
  groundA = ads1015_A.readADC_SingleEnded(2);    // Read from channel 2 of the first ADS1115

  weaponB = ads1015_B.readADC_SingleEnded(0);   // Read from channel 0 of the second ADS1115
  lameB = ads1015_B.readADC_SingleEnded(1);   // Read from channel 1 of the second ADS1115
  groundB = ads1015_B.readADC_SingleEnded(2);   // Read from channel 2 of the second ADS1115

  // Wire.begin();
  // // ADS_A Single Ended Readings AIN0
  // Wire.beginTransmission(ADS_A); // Start I2C Transmission to the ADS1115
  // Wire.write(CfgReg); // write the register address to point to the configuration register
  // Wire.write(MSBcfg1); // Write the MSB of the configuration register
  // Wire.write(LSBcfg); // Write the LSB of the configuration register
  // Wire.endTransmission();
  // Wire.beginTransmission(ADS_A); // Start I2C Transmission to the ADS1115
  // Wire.write(ConvReg);
  // Wire.endTransmission();
  // Wire.requestFrom(ADS_A, 2);
  // weaponA = Wire.read() << 4 | Wire.read();
  // Serial.print("Raw Value ADS_A weaponA: "); 
  // Serial.println(weaponA);

  // Wire.beginTransmission(ADS_A); // Start I2C Transmission to the ADS1115
  // Wire.write(CfgReg); // write the register address to point to the configuration register
  // Wire.write(MSBcfg2); // Write the MSB of the configuration register
  // Wire.write(LSBcfg); // Write the LSB of the configuration register
  // Wire.endTransmission();
  // Wire.beginTransmission(ADS_A); // Start I2C Transmission to the ADS1115
  // Wire.write(ConvReg);
  // Wire.endTransmission();
  // Wire.requestFrom(ADS_A, 2);
  // lameA = Wire.read() << 4 | Wire.read();
  // Serial.print("Raw Value ADS_A lameA: "); Serial.println(lameA);

  // Wire.beginTransmission(ADS_B); // Start I2C Transmission to the ADS1115
  // Wire.write(CfgReg); // write the register address to point to the configuration register
  // Wire.write(MSBcfg1); // Write the MSB of the configuration register
  // Wire.write(LSBcfg); // Write the LSB of the configuration register
  // Wire.endTransmission();
  // Wire.beginTransmission(ADS_B); // Start I2C Transmission to the ADS1115
  // Wire.write(ConvReg);
  // Wire.endTransmission();
  // Wire.requestFrom(ADS_B, 2);
  // lameB = Wire.read() << 4 | Wire.read();
  // Serial.print("Raw Value ADS_A weaponB: "); Serial.println(weaponB);

  // Wire.beginTransmission(ADS_B); // Start I2C Transmission to the ADS1115
  // Wire.write(CfgReg); // write the register address to point to the configuration register
  // Wire.write(MSBcfg2); // Write the MSB of the configuration register
  // Wire.write(LSBcfg); // Write the LSB of the configuration register
  // Wire.endTransmission();
  // Wire.beginTransmission(ADS_B); // Start I2C Transmission to the ADS1115
  // Wire.write(ConvReg);
  // Wire.endTransmission();
  // Wire.requestFrom(ADS_B, 2);
  // lameB = Wire.read() << 4 | Wire.read();
  // Serial.print("Raw Value ADS_A lameB: "); Serial.println(lameB);


>>>>>>> Stashed changes:Score_BOXXX/Score_Box_V4_Faster_TouchRecognition_MACBOOK/Score_Box_V4_Faster_TouchRecognition_MACBOOK.ino
  if(BUTTON_Debounce(MODE_BUTTON_PIN)){
    modeChange();
  }

<<<<<<< Updated upstream:Score_BOXXX/Score_box_V4_BetterModeChange/Score_Box_V4_Faster TouchRecognition MACBOOK.ino
if(!lockedOut) {
handleHit(); // will point to a Mode instance in struct Mode Foil,Epee, or Sabre
}
 
CheckForLockout(hitOnTargetA, hitOffTargetA, hitOnTargetB, hitOffTargetB, depressAtime, depressBtime, currentMode); // This is either true or False

//maybe want to make a reset function for each mode
if ( lockedOut|| (currentMode == &sabreMode && (hitOffTargetB || hitOffTargetA))){
  resetValues();
  }

  if (hitOnTargetA) {
    // pixels.Color takes RGB values, from 0,0,0 up to 255,255,255
    fill_solid(pixels222, NUMPIXELS, CRGB::Green); // Moderately bright GREEN color.
    FastLED.show(); // This sends the updated pixel color to the hardware.
    digitalWrite(buzzerPin, HIGH);
    }
  if (hitOffTargetA) {
    // pixels.Color takes RGB values, from 0,0,0 up to 255,255,255
    fill_solid(pixels222, NUMPIXELS, CRGB::Yellow); // Yellow color.
    FastLED.show(); // This sends the updated pixel color to the hardware.
    digitalWrite(buzzerPin, HIGH);
    }
  if (hitOnTargetB) {
    // pixels.Color takes RGB values, from 0,0,0 up to 255,255,255
    fill_solid(pixels, NUMPIXELS, CRGB::Red); // Moderately bright RED color.
    FastLED.show(); // This sends the updated pixel color to the hardware.
    digitalWrite(buzzerPin, HIGH);
    }
  if (hitOffTargetB){
    // pixels.Color takes RGB values, from 0,0,0 up to 255,255,255
    fill_solid(pixels, NUMPIXELS, CRGB::Blue); // Bright Blue color.
    FastLED.show(); // This sends the updated pixel color to the hardware.
    digitalWrite(buzzerPin, HIGH);
    }

  // String serData = String("hitOnTargetA  : ") + hitOnTargetA  + "\n"
  //                       + "hitOffTargetA : "  + hitOffTargetA + "\n"
  //                       + "hitOffTargetB : "  + hitOffTargetB + "\n"
  //                       + "hitOnTargetB  : "  + hitOnTargetB  + "\n"
  //                       + "Locked Out  : "  + lockedOut   + "\n";
  //   Serial.println(serData);
=======
  // Check for hits
  handleHit();



// if (number != 0){
//   Serial.println(number);
// }
if (now - last_update > UPDAE_INTERVAL) {
  last_update = now;
  number = (hitOnTargetA * OnTargetA_Flag_Value) +
                    (hitOnTargetB * OnTargetB_Flag_Value) +
                    (hitOffTargetA * OffTargetA_Flag_Value) +
                    (hitOffTargetB * OffTargetB_Flag_Value);
  switch (number){
    case OnTargetA_Flag_Value:
        // Serial.println("ON TARGET A");
        fill_solid(pixels222, NUMPIXELS, CRGB::Green); // Moderately bright GREEN color.
        FastLED.show(); // This sends the updated pixel color to the hardware.
        digitalWrite(buzzerPin, HIGH);
    break;
    case OnTargetB_Flag_Value:
        // Serial.println("ON TARGET B");
        fill_solid(pixels, NUMPIXELS, CRGB::Red); // Moderately bright RED color.
        FastLED.show(); // This sends the updated pixel color to the hardware.
        digitalWrite(buzzerPin, HIGH);
    break;
    case OffTargetA_Flag_Value:
        // Serial.println("OFF TARGET A");
        fill_solid(pixels222, NUMPIXELS, CRGB::Yellow); // Yellow color.
        FastLED.show(); // This sends the updated pixel color to the hardware.
        digitalWrite(buzzerPin, HIGH);
        if (currentMode == &sabreMode){
          fill_solid(pixels222, NUMPIXELS, CRGB::Black); // turn back off.
          FastLED.show(); // This sends the updated pixel color to the hardware.
          hitOffTargetA = false;
        }
    break;
    case OffTargetB_Flag_Value:
        // Serial.println("OFF TARGET B");
        fill_solid(pixels, NUMPIXELS, CRGB::Yellow); // Bright Blue color.
        FastLED.show(); // This sends the updated pixel color to the hardware.
        digitalWrite(buzzerPin, HIGH);
        if (currentMode == &sabreMode){
          fill_solid(pixels, NUMPIXELS, CRGB::Black); // turn back off.
          FastLED.show(); // This sends the updated pixel color to the hardware.
          hitOffTargetB = false;
        }
    break;
    case OnTargetA_Flag_Value+OnTargetB_Flag_Value:
        fill_solid(pixels222, NUMPIXELS, CRGB::Green); // Moderately bright GREEN color.
        fill_solid(pixels, NUMPIXELS, CRGB::Red); // Moderately bright RED color.
        FastLED.show(); // This sends the updated pixel color to the hardware.
        digitalWrite(buzzerPin, HIGH);
    break;
    case OnTargetA_Flag_Value+OffTargetB_Flag_Value:
        fill_solid(pixels222, NUMPIXELS, CRGB::Green); // Moderately bright GREEN color.
        fill_solid(pixels, NUMPIXELS, CRGB::Yellow); // Bright Blue color.
        FastLED.show(); // This sends the updated pixel color to the hardware.
        digitalWrite(buzzerPin, HIGH);
    break;
    case OnTargetA_Flag_Value+OffTargetA_Flag_Value:
        fill_solid(pixels222, NUMPIXELS, CRGB::Green); // Moderately bright GREEN color.
        fill_solid(pixels222, NUMPIXELS, CRGB::Yellow); // Yellow color.
        FastLED.show(); // This sends the updated pixel color to the hardware.
        digitalWrite(buzzerPin, HIGH);
    break;
    case OnTargetB_Flag_Value+OffTargetA_Flag_Value:
        fill_solid(pixels, NUMPIXELS, CRGB::Red); // Moderately bright RED color.
        fill_solid(pixels222, NUMPIXELS, CRGB::Yellow); // Yellow color.
        FastLED.show(); // This sends the updated pixel color to the hardware.
        digitalWrite(buzzerPin, HIGH);
    break;
    case OnTargetB_Flag_Value+OffTargetB_Flag_Value:
        fill_solid(pixels, NUMPIXELS, CRGB::Red); // Moderately bright RED color.
        fill_solid(pixels, NUMPIXELS, CRGB::Yellow); // Bright Blue color.
        FastLED.show(); // This sends the updated pixel color to the hardware.
        digitalWrite(buzzerPin, HIGH);
    break;
    case OffTargetA_Flag_Value + OffTargetB_Flag_Value:
        // Serial.println("OFF TTARGET A AND OFF TARGET B");
        fill_solid(pixels, NUMPIXELS, CRGB::Yellow); // Bright Blue color.
        fill_solid(pixels222, NUMPIXELS, CRGB::Yellow); // Yellow color.
        FastLED.show(); // This sends the updated pixel color to the hardware.
        digitalWrite(buzzerPin, HIGH);
        if (currentMode == &sabreMode){
          fill_solid(pixels, NUMPIXELS, CRGB::Black); // turn back off.
          fill_solid(pixels222, NUMPIXELS, CRGB::Black); // turn back off.
          FastLED.show(); // This sends the updated pixel color to the hardware.
          hitOffTargetA = false;
          hitOffTargetB = false;
        }
    break;
    default:
    break;
  } // end of switch statement
}
  
  // String serData = String("hitOnTargetA  : ") + hitOffTargetA +"\n"
  //                       + "hitOffTargetA : "  + hitOffTargetA + "\n"
  //                       + "hitOffTargetB : "  + hitOffTargetB + "\n"
  //                       + "hitOnTargetB  : "  + hitOnTargetB  + "\n"
  //                       + "Number        : "  + number  + "\n"
  //                       + "Locked Out    : "  + lockedOut   + "\n";
  // Serial.println(serData);
  //|| (currentMode == &sabreMode && (hitOffTargetB || hitOffTargetA))
  // if ((lockedOut == true)){ // i dont want this if statement in the main loop, i might have to suck it up
  //   resetValues();
  //   TimeResetWasCalled = millis();
  // }

  // Check if it's time to reset after LIGHTTIME has elapsed since the last hit

  if (lockedOut && ((now - max(hitTimeA, hitTimeB) > LIGHTTIME) || (hitOnTargetA && now - hitTimeA > LIGHTTIME) || (hitOnTargetB && now - hitTimeB > LIGHTTIME))) {
      resetValues();  // This will turn off LEDs and reset hit flags
      TimeResetWasCalled = millis();
  }
>>>>>>> Stashed changes:Score_BOXXX/Score_Box_V4_Faster_TouchRecognition_MACBOOK/Score_Box_V4_Faster_TouchRecognition_MACBOOK.ino
}
 