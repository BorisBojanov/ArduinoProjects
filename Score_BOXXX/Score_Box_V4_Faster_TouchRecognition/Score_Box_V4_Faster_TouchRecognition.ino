#include <FastLED.h>
#include <Wire.h>
#include <Adafruit_ADS1X15.h>
#include "esp32-hal-cpu.h" // for setting CPU speed
//TODO: set up debug levels correctly
#define DEBUG 0
//=======================
// Fast LED Setup
//=======================
#define NUMPIXELS 64  // number of neopixels in strip
#define DATA_PIN 12   // input pin Neopixel is attached to
#define DATA_PIN2 19  // input for the seconf Neopixel LED
#define CHIPSET WS2812B
#define COLOR_ORDER GRB
#define BRIGHTNESS 20
struct CRGB pixels[NUMPIXELS];
struct CRGB pixels222[NUMPIXELS];
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
Adafruit_ADS1015 ads1015_B;  // ADS1115 with ADDR pin connected to 3.3V (address 0x49)
#define BAUDRATE 115200      // baudrate of the serial debug interface
//============
// Pin Setup
// Gpios 6, 7, 8, 9, 10 and 11, 12 are a no go for ESP32 to use
// Gpio 0 is always high at 4096
// Gpio 35 is always high at 4096
//============
const uint8_t Mode_Button_PIN = 13;  // The pin where your mode button is connected
const uint8_t buzzerPin = 5;         // buzzer pin




//============
// ANY BUTTON DEBOUNCE
//============
const int DEBOUNCE_DELAY = 50;       // The debounce delay in milliseconds
unsigned long lastDebounceTime = 0;  // The last time the button state changed
int lastButtonState = LOW;           // The last known state of the button
int buttonState;
volatile bool ISR_StateChanged = false;


//=========================
// values of analog reads
// Note: volatile is used to have the variable updated more frequently
//       as the code is run,
//=========================
int16_t weaponA = 0;
int16_t weaponB = 0;
int16_t lameA = 0;
int16_t lameB = 0;
int16_t groundA = 0;
int16_t groundB = 0;

// for all X: weapon >= Values >= lame
int16_t Allowable_Deviation = 10;
//Foil Values
int16_t weapon_OffTarget_Threshold = 1000;
int16_t lame_OffTarget_Threshold = 10;
int16_t ground_OffTarget_Threshold = 10;

int16_t weapon_OnTarget_Threshold = 600;  //tested value for weapon is 833.  This should be adjusted if needed
int16_t lame_OnTarget_Threshold = 1000;    // Tested Value for lame is 833
int16_t ground_OnTarget_Threshold = 1000;
//Epee Values
int16_t Epee_Weapon_OnTarget_Threshold = 800;
int16_t Epee_Lame_OnTarget_Threshold = 800;
int16_t Epee_Ground_OnTarget_Threshold = 800;
//Sabre Values
int16_t Sabre_Weapon_OffTarget_Threshold = 1000;
int16_t Sabre_Lame_OffTarget_Threshold = 10;
int16_t Sabre_Ground_OffTarget_Threshold = 10;

int16_t Sabre_Weapon_OnTarget_Threshold = 400;
int16_t Sabre_Lame_OnTarget_Threshold = 300;
int16_t Sabre_Ground_OnTarget_Threshold = 300;
//=======================
// Lockouts
//=======================
uint16_t HIT_CHECK_INTERVAL = 1;
uint16_t Update_Interval = 20;
bool lockedOut = false;
uint32_t lastResetTime = 0;
uint32_t TimeResetWasCalled = millis();
uint16_t BUZZERTIME = 500;     // length of time the buzzer is kept on after a hit (ms)
uint16_t LIGHTTIME = 1000;     // length of time the lights are kept on after a hit (ms)
static int TimeOfLockout = 0;  // This is the time that the hit was detected
// Initialize variables to track the time of each hit
static uint32_t Hit_Time_A = 0;
static uint32_t Hit_Time_B = 0;
static uint32_t last_update = 0;
//==========================
// states
//==========================
bool depressedA = false;
bool depressedB = false;
bool Hit_On_Target_A = false;
bool Hit_Off_Target_A = false;
bool Hit_On_Target_B = false;
bool Hit_Off_Target_B = false;
// Assign each flag a unique power of 2
const int OnTargetA_Flag_Value = 2;    // 2^1
const int OnTargetB_Flag_Value = 4;    // 2^2
const int OffTargetA_Flag_Value = 8;   // 2^3
const int OffTargetB_Flag_Value = 16;  // 2^4
short int number = (Hit_On_Target_A * OnTargetA_Flag_Value) + (Hit_On_Target_B * OnTargetB_Flag_Value) + (Hit_Off_Target_A * OffTargetA_Flag_Value) + (Hit_Off_Target_B * OffTargetB_Flag_Value);
//==========================
//Forward Declare The functions used by struct WeaponMode
//==========================
void handleFoilHit();
void handleEpeeHit();
void handleSabreHit();
//==========================
// WeaponMode Changer
//==========================
int ModeLED;
struct WeaponMode {
  long lockoutTime;
  void (*Handle_Hit)();
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
WeaponMode foilMode = { 300, handleFoilHit, 0 };
WeaponMode epeeMode = { 45, handleEpeeHit, 1 };
WeaponMode sabreMode = { 170, handleSabreHit, 2 };
WeaponMode* currentMode = &foilMode;  // Default Mode

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


  long now = millis();  // Arduino uses millis() to get the number of milliseconds since the board started running.
  // Serial.println(lockedOut);
  //_____Check for lockout___
  if (((Hit_On_Target_A || Hit_Off_Target_A) && (TimeOfLockout < now))
      || ((Hit_On_Target_B || Hit_Off_Target_B) && (TimeOfLockout < now))) {
    lockedOut = true;
    //TimeResetWasCalled = millis();
    return;  // exit the function if we are locked out
  }

  // ___Weapon A___
  if (!Hit_On_Target_A && !Hit_Off_Target_A) {
    // Off target              // 1660                      // 10
    if ((weaponA > weapon_OffTarget_Threshold)) {  //&& (lameB < lame_OffTarget_Threshold)
      Hit_Off_Target_A = true;                        // Hit_Off_Target_A
      TimeOfLockout = millis() + foilMode.lockoutTime;
      Hit_Time_A = now;  // Record the time of the hit
    }
    // On target
    else if ((weaponA > weapon_OnTarget_Threshold && weaponA < 900) && (lameB < lame_OnTarget_Threshold && lameB > 600)) {  // Tested Value for lameB is 833
      Hit_On_Target_A = true;                                                                                                  // Hit_On_Target_A
      TimeOfLockout = millis() + foilMode.lockoutTime;
      Hit_Time_A = now;  // Record the time of the hit
    }
  }
  // ___Weapon B___
  if (!Hit_On_Target_B && !Hit_Off_Target_B) {
    // Off target
    if ((weaponB > weapon_OffTarget_Threshold)) {  //&& (lameA < lame_OffTarget_Threshold )
      Hit_Off_Target_B = true;                        // Hit_Off_Target_B
      TimeOfLockout = millis() + foilMode.lockoutTime;
      Hit_Time_B = now;  // Record the time of the hit
      // On target
    } else if ((weaponB > weapon_OnTarget_Threshold && weaponB < 900) && (lameA < lame_OnTarget_Threshold && lameA > 600)) {  // Tested Value for lameA is 833
      Hit_On_Target_B = true;                                                                                                    // Hit_On_Target_B
      TimeOfLockout = millis() + foilMode.lockoutTime;
      Hit_Time_B = now;  // Record the time of the hit
      //digitalWrite(buzzerPin, HIGH);
    }
  }
}

void handleEpeeHit() {
  //Serial.println("Inside: Handle EPEE Function");
  // Implement the specific behavior for a hit in Epee mode
  // read analog pins
  // int16_t weaponA, lameA, groundA, adc3;
  // int16_t weaponB, lameB, groundB, adc7;
  // weaponA = ads1015_A.readADC_SingleEnded(0);    // Read from channel 0 of the first ADS1115
  // lameA = ads1015_A.readADC_SingleEnded(1);    // Read from channel 1 of the first ADS1115
  // groundA = ads1015_A.readADC_SingleEnded(2);    // Read from channel 2 of the first ADS1115

  // weaponB = ads1015_B.readADC_SingleEnded(0);   // Read from channel 0 of the second ADS1115
  // lameB = ads1015_B.readADC_SingleEnded(1);   // Read from channel 1 of the second ADS1115
  // groundB = ads1015_B.readADC_SingleEnded(2);   // Read from channel 2 of the second ADS1115
  // Serial.println(lockedOut);
  // Serial.println(Hit_On_Target_A);
  // Serial.println(Hit_On_Target_B);
  // Serial.print("Default ADS1115 (0x48) weaponA 0: "); Serial.println(weaponA);
  // Serial.print("Default ADS1115 (0x48) lameA 1: "); Serial.println(lameA);
  // Serial.print("Default ADS1115 (0x48) groundA 2: "); Serial.println(groundA);

  // Serial.print("ADDR ADS1115 (0x49) weaponB 4: "); Serial.println(weaponB);
  // Serial.print("ADDR ADS1115 (0x49) lameB 5: "); Serial.println(lameB);
  // Serial.print("ADDR ADS1115 (0x49) groundB 6: "); Serial.println(groundB);
  //_____Check for lockout___
  // Serial.println(lockedOut);
  long now = millis();
  // Check for lockout
  if (lockedOut || (Hit_On_Target_A && (now - Hit_Time_A) > epeeMode.lockoutTime) || (Hit_On_Target_B && (now - Hit_Time_B) > epeeMode.lockoutTime)) {
    lockedOut = true;
    return;
  }
  // weapon A
  //  no hit for A yet    && weapon depress    && opponent lame touched
  if (!Hit_On_Target_A) {
    if ((weaponA > Epee_Weapon_OnTarget_Threshold) && (lameA > Epee_Lame_OnTarget_Threshold)) {
      Hit_On_Target_A = true;  // Hit_On_Target_A
      //TimeOfLockout = millis() + epeeMode.lockoutTime;
      Hit_Time_A = now;  // Record the time of the hit
    }
  }
  // weapon B
  //  no hit for B yet    && weapon depress    && opponent lame touched
  if (!Hit_On_Target_B) {
    if ((weaponB > Epee_Weapon_OnTarget_Threshold) && (lameB > Epee_Lame_OnTarget_Threshold)) {
      Hit_On_Target_B = true;  // Hit_On_Target_B
      //TimeOfLockout = millis() + epeeMode.lockoutTime;
      Hit_Time_B = now;  // Record the time of the hit
    }
  }
}

void handleSabreHit() {
  //Serial.println("Inside: Handle Sabre Function");
  // Implement the specific behavior for a hit in Sabre mode
  // read analog pins
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
  if (lockedOut || (Hit_On_Target_A && (now - Hit_Time_A) > sabreMode.lockoutTime) || (Hit_On_Target_B && (now - Hit_Time_B) > sabreMode.lockoutTime)) {
    lockedOut = true;
    return;
  }
  // ___Weapon A___
  if (!Hit_On_Target_A || !Hit_Off_Target_A) {  // We dont need to check if we have already got a hit off target herre since the off target will not need to trigger a lockout
    // Off target
    if ((weaponA > weapon_OffTarget_Threshold) && (lameB < lame_OffTarget_Threshold)) {
      Hit_Off_Target_A = true;  // Hit_Off_Target_A
      // On target
    } else if ((weaponA > Sabre_Weapon_OnTarget_Threshold && weaponA < 700) && (lameB > Sabre_Lame_OnTarget_Threshold)) {
      Hit_On_Target_A = true;  // Hit_On_Target_A
      // TimeOfLockout = millis() + sabreMode.lockoutTime;
      Hit_Time_A = now;
    }
  }
  // ___Weapon B___
  if (!Hit_On_Target_B || !Hit_Off_Target_B) {
    // Off target
    if ((weaponB > weapon_OffTarget_Threshold) && (lameA < lame_OffTarget_Threshold)) {
      Hit_Off_Target_B = true;  // Hit_Off_Target_B
    }
    // On target          // 500                                                 // 500
    else if ((weaponB > Sabre_Weapon_OnTarget_Threshold && weaponB < 700) && (lameA > Sabre_Lame_OnTarget_Threshold)) {
      Hit_On_Target_B = true;  // Hit_On_Target_B
      // TimeOfLockout = millis() + sabreMode.lockoutTime;
      Hit_Time_B = now;
    }
  }
}

void Handle_Hit() {
  currentMode->Handle_Hit();  // will pint to a Mode instance in struct Mode
}

void Mode_Change() {
  buttonState = LOW;
  if (currentMode == &foilMode) {
    //set an LED to show Mode

    pixels[0] = CRGB::Black;
    pixels[1] = CRGB::Black;
    pixels[2] = CRGB::Black;
    HardReset();
    pixels[1] = CRGB(255, 0, 255);  // Purple is a mix of full red and blue with no green
    FastLED.show();  // Display the newly-written colors
    Serial.println("changed mode was called: to Epee");
    currentMode = &epeeMode;
  } else if (currentMode == &epeeMode) {
    //set an LED to show Mode
    for (int i = 0; i <= 2; i++) {
      pixels[i] = CRGB::Black;
    }
    HardReset();
    pixels[2] = CRGB(255, 0, 255);  // Purple is a mix of full red and blue with no green
    FastLED.show();                 // Display the newly-written colors
    Serial.println("changed mode was called: to Sabre");
    currentMode = &sabreMode;
  } else {
    //set an LED to show Mode
    for (int i = 0; i <= 2; i++) {
      pixels[i] = CRGB::Black;
    }
    HardReset();
    pixels[0] = CRGB(255, 0, 255);  // Purple is a mix of full red and blue with no green
    FastLED.show();                 // Display the newly-written colors
    Serial.println("changed mode was called: to Foil");
    currentMode = &foilMode;
  }
}

void BUTTON_ISR() {
  ISR_StateChanged = true;
}

bool BUTTON_Debounce(int Some_Button_PIN) {
  int reading = digitalRead(Some_Button_PIN);
  static int buttonState;
  static int lastButtonState = LOW;
  static unsigned long lastDebounceTime = 0;
  int DEBOUNCE_DELAY = 50;  // Adjust this value based on your specific button and requirements

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
// The ResetValues will mainly occour when the lockout time has been reached
// HardReset will be used for mode changes and its main difference is that it does not case about light time.
//======================
void HardReset() {
  digitalWrite(buzzerPin, LOW);
  //Removed this from all Fill color commands and placed here to see if this command is being excicuted
  fill_solid(pixels, NUMPIXELS, CRGB::Black);
  fill_solid(pixels222, NUMPIXELS, CRGB::Black);
  FastLED.show();
  number = 0;
  lockedOut = false;
  Hit_On_Target_A  = false;
  Hit_Off_Target_A = false;
  Hit_On_Target_B  = false;
  Hit_Off_Target_B = false;
}

void ResetValues() {  //might need to pass in a value when this function is called of the time that the function is called
  static unsigned long buzzerOFFTime = 0;
  static unsigned long lightsOFFTime = 0;
  static bool isBuzzerOFF = false;
  if (TimeResetWasCalled - lastResetTime >= BUZZERTIME) {  // This makes sure that the buzzer is turned off
    if (!isBuzzerOFF && millis() - lastResetTime >= BUZZERTIME) {
      digitalWrite(buzzerPin, LOW);
      buzzerOFFTime = millis();
      isBuzzerOFF = true;
    }
    //Before the if statement the LEDS are ON and the Buzzer is OFF
    if (isBuzzerOFF && millis() - buzzerOFFTime >= LIGHTTIME - BUZZERTIME) {
      fill_solid(pixels, NUMPIXELS, CRGB::Black);
      fill_solid(pixels222, NUMPIXELS, CRGB::Black);
      FastLED.show();
      number = 0;
      lockedOut = false;
      Hit_On_Target_A  = false;
      Hit_Off_Target_A = false;
      Hit_On_Target_B  = false;
      Hit_Off_Target_B = false;
      lastResetTime = TimeResetWasCalled;
      for (int i = 0; i <= 2; i++) {
        pixels[i] = CRGB::Black;
      }
      pixels[currentMode->ModeLED] = CRGB(255, 0, 255);  // Turn on the light for the current mode
      FastLED.show();
      isBuzzerOFF = false;
    }
  }
}

void setup() {
  Serial.begin(BAUDRATE);
  setCpuFrequencyMhz(240); //Set CPU clock to 240MHz fo example
  Wire.setClock(400000UL);  // Increase I2C clock speed to 400kHz UL is to tell the compiler that it is an unsigned long
  Serial.println("Score_box_V4_BetterMode_Change");
  Serial.println("Getting single-ended readings from AIN0..3");
  Serial.println("ADC GAIN_ONE Range: +/- 4.096V (1 bit = 0.125mV/ADS1115)");
  pinMode(Mode_Button_PIN, INPUT_PULLDOWN);
  pinMode(buzzerPin, OUTPUT);
   Serial.println("Three Weapon Scoring Box");
   Serial.println("================");
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
  ads1015_A.begin(0x48);
  ads1015_B.begin(0x49);
  ads1015_A.setGain(GAIN_ONE); // 1x gain   +/- 4.096V  1 bit = 2mV      0.125mV
  ads1015_B.setGain(GAIN_ONE); // 1x gain   +/- 4.096V  1 bit = 2mV      0.125mV
  ads1015_A.setDataRate(3300); // 3300 samples per second 
  ads1015_B.setDataRate(3300); // 3300 samples per second

  Serial.println(ads1015_A.getDataRate()); 
  Serial.println(ads1015_B.getDataRate());
  //====ADS_A====
  while (!Serial)
    ;  // Wait for the serial port to connect
  if (!ads1015_A.begin(0x48)) {
    Serial.println("Failed to initialize ADS1015 at default 0x48!");
  }
  //====ADS_B====
  if (!ads1015_B.begin(0x49)) {
    Serial.println("Failed to initialize ADS1015 at address 0x49!");
  }



  //FAST LED SETUP
  FastLED.addLeds<CHIPSET, DATA_PIN, COLOR_ORDER>(pixels, NUMPIXELS);
  FastLED.addLeds<CHIPSET, DATA_PIN2, COLOR_ORDER>(pixels222, NUMPIXELS);
  FastLED.setBrightness(BRIGHTNESS);
  // pixels[0] = CRGB::Black;
  // pixels[1] = CRGB::Black;
  // pixels[2] = CRGB::Black;
  // pixels[1] = CRGB(255, 0, 255);  // Purple LED in Foil position to show the mode
  // FastLED.show();  // Display the newly-written colors
  //attatch the interrupts we need to check
  //attachInterrupt(digitalPinToInterrupt(ads1115_A.readADC_SingleEnded(0)), Handle_Hit, CHANGE);
  //attachInterrupt(digitalPinToInterrupt(ads1115_B.readADC_SingleEnded(0)), Handle_Hit, CHANGE); //SEt a different interrupt PIN
  attachInterrupt(digitalPinToInterrupt(Mode_Button_PIN), BUTTON_ISR, FALLING);
  ResetValues();
}

void loop() {
  unsigned long now = millis();

  weaponA = ads1015_A.readADC_SingleEnded(0);  // Read from channel 0 of the first ADS1115
  lameA = ads1015_A.readADC_SingleEnded(1);    // Read from channel 1 of the first ADS1115
  //groundA = ads1015_A.readADC_SingleEnded(2);  // Read from channel 2 of the first ADS1115

  weaponB = ads1015_B.readADC_SingleEnded(0);  // Read from channel 0 of the second ADS1115
  lameB = ads1015_B.readADC_SingleEnded(1);    // Read from channel 1 of the second ADS1115
  //groundB = ads1015_B.readADC_SingleEnded(2);  // Read from channel 2 of the second ADS1115
  
  if (ISR_StateChanged){
    if (BUTTON_Debounce(Mode_Button_PIN)) {
    Mode_Change();
    }
  }
  // Check for hits
  Handle_Hit();




  if (now - last_update > Update_Interval) {
    last_update = now;
    number = (Hit_On_Target_A * OnTargetA_Flag_Value) + (Hit_On_Target_B * OnTargetB_Flag_Value) + (Hit_Off_Target_A * OffTargetA_Flag_Value) + (Hit_Off_Target_B * OffTargetB_Flag_Value);
    // if (number != 0){
    // Serial.println(number);
    // }
    switch (number) {
      case OnTargetA_Flag_Value:
        // Serial.println("ON TARGET A");
        fill_solid(pixels222, NUMPIXELS, CRGB::Green);  // Moderately bright GREEN color.
        FastLED.show();                                 // This sends the updated pixel color to the hardware.
        digitalWrite(buzzerPin, HIGH);
        break;
      case OnTargetB_Flag_Value:
        // Serial.println("ON TARGET B");
        fill_solid(pixels, NUMPIXELS, CRGB::Red);  // Moderately bright RED color.
        FastLED.show();                            // This sends the updated pixel color to the hardware.
        digitalWrite(buzzerPin, HIGH);
        break;
      case OffTargetA_Flag_Value:
        // Serial.println("OFF TARGET A");
        fill_solid(pixels222, NUMPIXELS, CRGB::Yellow);  // Yellow color.
        FastLED.show();                                  // This sends the updated pixel color to the hardware.
        digitalWrite(buzzerPin, HIGH);
        if (currentMode == &sabreMode) {
          fill_solid(pixels222, NUMPIXELS, CRGB::Black);  // turn back off.
          FastLED.show();                                 // This sends the updated pixel color to the hardware.
          Hit_Off_Target_A = false;
        }
        break;
      case OffTargetB_Flag_Value:
        // Serial.println("OFF TARGET B");
        fill_solid(pixels, NUMPIXELS, CRGB::Yellow);  // Bright Blue color.
        FastLED.show();                               // This sends the updated pixel color to the hardware.
        digitalWrite(buzzerPin, HIGH);
        if (currentMode == &sabreMode) {
          fill_solid(pixels, NUMPIXELS, CRGB::Black);  // turn back off.
          FastLED.show();                              // This sends the updated pixel color to the hardware.
          Hit_Off_Target_B = false;
        }
        break;
      case OnTargetA_Flag_Value + OnTargetB_Flag_Value:
        // Serial.println("ON TARGET A AND ON TARGET B");
        fill_solid(pixels222, NUMPIXELS, CRGB::Green);  // Moderately bright GREEN color.
        fill_solid(pixels, NUMPIXELS, CRGB::Red);       // Moderately bright RED color.
        FastLED.show();                                 // This sends the updated pixel color to the hardware.
        digitalWrite(buzzerPin, HIGH);
        break;
      case OnTargetA_Flag_Value + OffTargetB_Flag_Value:
        fill_solid(pixels222, NUMPIXELS, CRGB::Green);  // Moderately bright GREEN color.
        fill_solid(pixels, NUMPIXELS, CRGB::Yellow);    // Bright Blue color.
        FastLED.show();                                 // This sends the updated pixel color to the hardware.
        digitalWrite(buzzerPin, HIGH);
        break;
      case OnTargetA_Flag_Value + OffTargetA_Flag_Value:
        fill_solid(pixels222, NUMPIXELS, CRGB::Green);   // Moderately bright GREEN color.
        fill_solid(pixels222, NUMPIXELS, CRGB::Yellow);  // Yellow color.
        FastLED.show();                                  // This sends the updated pixel color to the hardware.
        digitalWrite(buzzerPin, HIGH);
        break;
      case OnTargetB_Flag_Value + OffTargetA_Flag_Value:
        fill_solid(pixels, NUMPIXELS, CRGB::Red);        // Moderately bright RED color.
        fill_solid(pixels222, NUMPIXELS, CRGB::Yellow);  // Yellow color.
        FastLED.show();                                  // This sends the updated pixel color to the hardware.
        digitalWrite(buzzerPin, HIGH);
        break;
      case OnTargetB_Flag_Value + OffTargetB_Flag_Value:
        fill_solid(pixels, NUMPIXELS, CRGB::Red);     // Moderately bright RED color.
        fill_solid(pixels, NUMPIXELS, CRGB::Yellow);  // Bright Blue color.
        FastLED.show();                               // This sends the updated pixel color to the hardware.
        digitalWrite(buzzerPin, HIGH);
        break;
      case OffTargetA_Flag_Value + OffTargetB_Flag_Value:
        // Serial.println("OFF TTARGET A AND OFF TARGET B");
        fill_solid(pixels, NUMPIXELS, CRGB::Yellow);     // Bright Blue color.
        fill_solid(pixels222, NUMPIXELS, CRGB::Yellow);  // Yellow color.
        FastLED.show();                                  // This sends the updated pixel color to the hardware.
        digitalWrite(buzzerPin, HIGH);
        if (currentMode == &sabreMode) {
          fill_solid(pixels, NUMPIXELS, CRGB::Black);     // turn back off.
          fill_solid(pixels222, NUMPIXELS, CRGB::Black);  // turn back off.
          FastLED.show();                                 // This sends the updated pixel color to the hardware.
          Hit_Off_Target_A = false;
          Hit_Off_Target_B = false;
        }
        break;
      default:
        break;
    }  // end of switch statement
  }

  // String serData = String("Hit_On_Target_A  : ") + Hit_Off_Target_A +"\n"
  //                       + "Hit_Off_Target_A : "  + Hit_Off_Target_A + "\n"
  //                       + "Hit_Off_Target_B : "  + Hit_Off_Target_B + "\n"
  //                       + "Hit_On_Target_B  : "  + Hit_On_Target_B  + "\n"
  //                       + "Number        : "  + number  + "\n"
  //                       + "Locked Out    : "  + lockedOut   + "\n";
  // Serial.println(serData);
  //|| (currentMode == &sabreMode && (Hit_Off_Target_B || Hit_Off_Target_A))
  // if ((lockedOut == true)){ // i dont want this if statement in the main loop, i might have to suck it up
  //   ResetValues();
  //   TimeResetWasCalled = millis();
  // }

  // Check if it's time to reset after LIGHTTIME has elapsed since the last hit

  if (lockedOut && ((now - max(Hit_Time_A, Hit_Time_B) > LIGHTTIME) || (Hit_On_Target_A && now - Hit_Time_A > LIGHTTIME) || (Hit_On_Target_B && now - Hit_Time_B > LIGHTTIME))) {
    // Serial.println("Locked Out: Resetting Values");
    ResetValues();  // This will turn off LEDs and reset hit flags
    TimeResetWasCalled = millis();
  }
}
