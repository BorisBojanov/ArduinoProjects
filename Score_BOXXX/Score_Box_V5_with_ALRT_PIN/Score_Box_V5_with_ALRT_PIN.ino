//Score_Box_V5_with_ALRT_PIN
/*
  Remember that the Registers are 16bit, this means that any value you are storing in them needs to be 16bit.
  The ADS1015 is a 12bit ADC, this means that the values you are reading from the ADC are 12bit. 
  So when you are Reading valaues you do a bit shift of 4 to get a 12 bit value from the 16 bit register.
  
*/
#include <Arduino.h>
#include <FastLED.h>
#include <Wire.h>
#include <Adafruit_I2CDevice.h>
#include "esp32-hal-cpu.h" // for setting CPU speed
#include <Adafruit_ADS1X15.h>


//TODO: set up debug levels correctly
#define ADS_A 0x48 // ADDR Connected to GND
#define ADS_B 0x49 // ADDR Connected to VCC
#define ADS_C 0x4A // ADDR Connected to SDA
#define ADS_D 0x4B // ADDR Connected to SCL
// Adafruit_ADS1015 ads1015_A;  // ADS1015 with ADDR pin floating (default address 0x48)
// Adafruit_ADS1015 ads1015_B;  // ADS1015 with ADDR pin connected to 3.3V (address 0x49)
const uint16_t BAUDRATE = 115200; // baudrate of the serial debug interface
// Assign Variables
uint16_t RawValue_ADS_A; // Raw value from the ADC 16bit
uint16_t RawValue_ADS_B; // Raw value from the ADC 16bit
uint16_t RawValue_ADS_C; // Raw value from the ADC 16bit
uint16_t RawValue_ADS_D; // Raw value from the ADC 16bit
uint16_t ConvertedValue; // Contains the End result of the ADS rawdata
// ADS1015 Register Addresses, These are pointers to the address we want to access
const short ConvReg     = 0b00000000; // Conversion Register Stores the result of the last conversion
const short CfgReg      = 0b00000001; // Config Register contains settings for how the ADC operates
const short LoThreshReg = 0b00000010; // Low Threshold Register
const short HiThreshReg = 0b00000011; // High Threshold Register
byte error; // Contains the error int if I2C communication fails
// Setting the Config Register
//==============================
//               MSB
//==============================
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
const short MSBcfg  = 0b01000010; // config for single-ended Continous conversion for 4.096v Readings on A0               
//const short MSBcfg  = 0b01000010; // config for single-ended Continous conversion for 4.096v Readings on A0
const short MSBcfg2 = 0b01010010; // config for single-ended Continous conversion for 4.096v Readings on A1
//const short MSBcfg3 = 0b01100010; // config for single-ended Continous conversion for 4.096v Readings on A2
//const short MSBcfg4 = 0b01110010; // config for single-ended Continous conversion for 4.096v Readings on A3

const short MSBcfgCOMPAR_A0_A1 = 0b00000010; // config for comparitor A0 >A1 for 4.096v Readings, Continuous Conversion
const short MSBcfgCOMPAR_A0_A3 = 0b00010010; // config for comparitor A0 >A3 for 4.096v Readings, Continuous Conversion
// const short MSBcfgCOMPAR_A1_A3 = 0b00100010; // config for comparitor A1 >A3 for 4.096v Readings, Continuous Conversion
//const short MSBcfgCOMPAR_A2_A3 = 0b00110010; // config for comparitor A2 >A3 for 4.096v Readings, Continuous Conversion

//==============================
//               LSB
//==============================
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
                                 // 000       1             0                           0                                 10
  const short LSBcfg  = 0b11110000;// 860x sps, Window mode, Low when in active state, pulse the Pin after each conversion, 4 conversions
                                 // goes ACTIVE after the fourth conversion, so we do 4 confirmations.
  const short LSBcfg2 = 0b11111000;// 860x sps, Window mode, High when in active state, pulse the Pin after each conversion, 4 conversions
 
/*=======================
// GPIOs
//=======================
BAD PINS
GPIO 6 to GPIO 11 are exposed in some ESP32 development boards. However, chip and are not recommended for other uses. So, don’t use these pins in your projects:
// Gpios 6, 7, 8, 9, 10 and 11, 12 are a no go for ESP32 to use
GPIO 6 (SCK/CLK)
GPIO 7 (SDO/SD0)
GPIO 8 (SDI/SD1)
GPIO 9 (SHD/SD2)
GPIO 10 (SWP/SD3)
GPIO 11 (CSC/CMD)

GOOD PINS
ADC1_CH0 (GPI 36) = OnTargetA_ALRT
ADC1_CH3 (GPI 39) = OnTargetB_ALRT
ADC1_CH6 (GPI 34) 
ADC1_CH7 (GPI 35) 
ADC1_CH4 (GPIO 32) = LED_DATA_1
ADC1_CH5 (GPIO 33) = LED_DATA_2
VSPI MISO(GPIO 19) 
VSPI CLK (GPIO 18) = OffTargetB_ALRT
VSPI CSO (GPIO 5) = OffTargetA_ALRT
UART_2_TX(GPIO 17) = Buzzer
UART_2_RX(GPIO 16) = Mode_Change_Button
*/
//============
// Pin Setup
// Gpios 6, 7, 8, 9, 10 and 11, 12 are a no go for ESP32 to use
// Gpio 0 is always high at 4096
// Gpio 35 is always high at 4096
//============
const uint8_t Mode_Button_PIN = 5;  // The pin where your mode button is connected
const uint8_t buzzerPin = 18;         // buzzer pin
int WeaponA_ONTARGET_ALRT_PIN = 34; // ADS1015 Ready Pin good
int WeaponB_ONTARGET_ALRT_PIN = 35; // ADS1015 Ready Pin 
int WeaponA_OFFTARGET_ALRT_PIN = 16; // ADS1015 Ready Pin
int WeaponB_OFFTARGET_ALRT_PIN = 17; // ADS1015 Ready Pin
#define DATA_PIN  32   // input pin Neopixel is attached to
#define DATA_PIN2 33  // input for the seconf Neopixel LED
/*=======================
// ADC Defines and info
//=======================
// The ADC input range (or gain) can be changed via the following
// functions, but be careful never to exceed VDD +0.3V max, or to
// exceed the upper and lower limits if you adjust the input range!
// Setting these values incorrectly may destroy your ADC!
//                                                                ADS1015  ADS1015
//                                                                -------  -------
// ads.setGain(GAIN_TWOTHIRDS);  // 2/3x gain +/- 6.144V  1 bit = 3mV      0.1875mV (default)
// ads.setGain(GAIN_ONE);        // 1x gain   +/- 4.096V  1 bit = 2mV      0.125mV
// ads.setGain(GAIN_TWO);        // 2x gain   +/- 2.048V  1 bit = 1mV      0.0625mV
// ads.setGain(GAIN_FOUR);       // 4x gain   +/- 1.024V  1 bit = 0.5mV    0.03125mV
// ads.setGain(GAIN_EIGHT);      // 8x gain   +/- 0.512V  1 bit = 0.25mV   0.015625mV
// ads.setGain(GAIN_SIXTEEN);    // 16x gain  +/- 0.256V  1 bit = 0.125mV  0.0078125mV
*/
//ADC info
// The ADC input range (or gain) can be changed via the following
// functions, but be careful never to exceed VDD +0.3V max, or to
// exceed the upper and lower limits if you adjust the input range!
// Setting these values incorrectly may destroy your ADC!
//                                                                ADS1015  ADS1015
//                                                                -------  -------
// ads.setGain(GAIN_TWOTHIRDS);  // 2/3x gain +/- 6.144V  1 bit = 3mV      0.1875mV (default)
// ads.setGain(GAIN_ONE);        // 1x gain   +/- 4.096V  1 bit = 2mV      0.125mV
// ads.setGain(GAIN_TWO);        // 2x gain   +/- 2.048V  1 bit = 1mV      0.0625mV
// ads.setGain(GAIN_FOUR);       // 4x gain   +/- 1.024V  1 bit = 0.5mV    0.03125mV
// ads.setGain(GAIN_EIGHT);      // 8x gain   +/- 0.512V  1 bit = 0.25mV   0.015625mV
// ads.setGain(GAIN_SIXTEEN);    // 16x gain  +/- 0.256V  1 bit = 0.125mV  0.0078125mV
//=======================
// Fast LED Setup
//=======================
#define NUMPIXELS 64  // number of neopixels in strip
#define CHIPSET WS2812B
#define COLOR_ORDER GRB
#define BRIGHTNESS 20
struct CRGB pixels[NUMPIXELS];
struct CRGB pixels222[NUMPIXELS];
//============
// ANY BUTTON DEBOUNCE
//============
const int DEBOUNCE_DELAY = 50;       // The debounce delay in milliseconds
unsigned long lastDebounceTime = 0;  // The last time the button state changed
int lastButtonState = LOW;           // The last known state of the button
int buttonState;
volatile unsigned long lastInterruptTime = 0;
//============
// ISR Variables
//============
volatile int16_t timeHitWasMade; // This is the time the hit was made
volatile bool Mode_ISR_StateChanged = false;
volatile bool Hit_Type_ISR_StateChanged = false;
// This is the flag that is set when the hit is detected by ADS1015
// They are different from (bool Hit_On_Target_A = false;) 
volatile bool OnTargetA_Flag   = false; 
volatile bool OnTargetB_Flag   = false;
volatile bool OffTargetA_Flag  = false;
volatile bool OffTargetB_Flag  = false;
// Assign each flag a unique power of 2
const int OnTargetA_Flag_Value = 2; // 2^1
const int OnTargetB_Flag_Value = 4; // 2^2
const int OffTargetA_Flag_Value = 8; // 2^3
const int OffTargetB_Flag_Value = 16; // 2^4
//==========================
// states
//==========================
bool Hit_On_Target_A = false;
bool Hit_Off_Target_A = false;
bool Hit_On_Target_B = false;
bool Hit_Off_Target_B = false;
// Assign each flag a unique power of 2
// const int OnTargetA_Flag_Value = 2;    // 2^1
// const int OnTargetB_Flag_Value = 4;    // 2^2
// const int OffTargetA_Flag_Value = 8;   // 2^3
// const int OffTargetB_Flag_Value = 16;  // 2^4
short int number =  (Hit_On_Target_A * OnTargetA_Flag_Value) + 
                    (Hit_On_Target_B * OnTargetB_Flag_Value) + 
                    (Hit_Off_Target_A * OffTargetA_Flag_Value) + 
                    (Hit_Off_Target_B * OffTargetB_Flag_Value);
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


//=======================
// Lockouts
//=======================
uint16_t HIT_CHECK_INTERVAL = 1;
uint16_t Update_Interval = 20;
bool lockedOut = false;
uint32_t lastResetTime = 0;
uint32_t TimeResetWasCalled = millis();
uint16_t BUZZERTIME = 500;     // length of time the buzzer is kept on after a hit (ms)
uint16_t LIGHTTIME = 2000;     // length of time the lights are kept on after a hit (ms)

static int TimeOfLockout = 0;  // This is the time that the hit was detected
//Initialize variables to track the time of each hit
static uint32_t Hit_Time_A = 0;
static uint32_t Hit_Time_B = 0;
static uint32_t last_update = 0;
//=======================
// ADS Setup and Config
//=======================
void Foil_ADS_Configurator(){
  // SET GAIN
  // SET SPS
  // SET COMPARATOR MODE (Window with HI and LO values, Ain0 - Ain3)
  const int HI = 13280; // 1.66V Convert the voltage to a 16bit value
  const int LO = 12800;  // 1.60V Convert the voltage to a 16bit value
  /*
  3.3v
  |  ALRT == T
  3.0V
  |  ALRT == F
  1.30V
  |  ALRT == T
  0V
  */
  const int HIOffTarget = 32500; // 4.75V Convert the voltage to a 15bit value 0.000125mVV per bit
  const int LOOffTarget = 24000; // 3.0V Convert the voltage to a 15bit value 0.000125V per bit
  Wire.begin();
  Wire.setClock(400000); //Increase I2C clock speed to 400kHz
  //======================ADS_A================================
  delay(10);
  //the following will disable the HI/LO Thresholds registers to use pin ## as the READY pin
  Wire.beginTransmission(ADS_A); // Start I2C Transmission to the ADS1015
  Wire.write(HiThreshReg); // 16bit register, so we must send the number in two parts
  Wire.write(highByte(HI)); //Write the MSB of the HI Threshold
  Wire.write(lowByte(HI)); //Write the LSB of the HI Threshold
  // Wire.write(0b10000000); // the 1 states that this is the MSB Set the HI Threshold to 0
  // Wire.write(0b00000000); // the 0 states that this is the LSB Set the HI Threshold to 0
  error = Wire.endTransmission(); // End the I2C Transmission
  if (error > 0) {
    Serial.print("Error: ");
    Serial.println(error);
    Serial.println(ADS_A);
    Serial.println(HiThreshReg);
  } else {
    Serial.println("Command ackowledged");
  }

  delay(10); //wait 10milliseconds before sending the next I2C transmission

  Wire.beginTransmission(ADS_A); // Start I2C Transmission to the ADS1015
  Wire.write(LoThreshReg); // 16bit register, so we must send the number in two parts
  Wire.write(highByte(LO)); //Write the MSB of the LO Threshold
  Wire.write(lowByte(LO)); //Write the LSB of the LO Threshold
  // Wire.write(0b01111111); // the 1 states that this is the MSB 
  // Wire.write(0b11111111); // the 0 states that this is the LSB 
  error = Wire.endTransmission(); // End the I2C Transmission
  if (error > 0) {
    Serial.print("Error: ");
    Serial.println(error);
  } else {
    Serial.println("Command ackowledged");
  }

  delay(10);
  //This is sending the configuration to the ADS1015
  Wire.beginTransmission(ADS_A); // Start I2C Transmission to the ADS1015
  Wire.write(CfgReg); // write the register address to point to the configuration register
  Wire.write(MSBcfg); // Write the MSB of the configuration register
  Wire.write(LSBcfg); // Write the LSB of the configuration register
  error = Wire.endTransmission(); // End the I2C Transmission
  if (error > 0) {
    Serial.print("Error: ");
    Serial.println(error);
  } else {
    Serial.println("Configuration Set for ADS_A");
  }
//======================ADS_B================================
  delay(10);
  //the following will disable the HI/LO Thresholds registers to use pin ## as the READY pin
  Wire.beginTransmission(ADS_B); // Start I2C Transmission to the ADS1015
  Wire.write(HiThreshReg); // 16bit register, so we must send the number in two parts
  Wire.write(highByte(HI)); //Write the MSB of the HI Threshold
  Wire.write(lowByte(HI)); //Write the LSB of the HI Threshold
  // Wire.write(0b10000000); // the 1 states that this is the MSB Set the HI Threshold to 0
  // Wire.write(0b00000000); // the 0 states that this is the LSB Set the HI Threshold to 0
  error = Wire.endTransmission(); // End the I2C Transmission
  if (error > 0) {
    Serial.print("Error: ");
    Serial.println(error);
  } else {
    Serial.println("Command ackowledged");
  }

  delay(10); //wait 10milliseconds before sending the next I2C transmission

  Wire.beginTransmission(ADS_B); // Start I2C Transmission to the ADS1015
  Wire.write(LoThreshReg); // 16bit register, so we must send the number in two parts
  Wire.write(highByte(LO)); //Write the MSB of the LO Threshold
  Wire.write(lowByte(LO)); //Write the LSB of the LO Threshold
  // Wire.write(0b01111111); // the 1 states that this is the MSB 
  // Wire.write(0b11111111); // the 0 states that this is the LSB 
  error = Wire.endTransmission(); // End the I2C Transmission
  if (error > 0) {
    Serial.print("Error: ");
    Serial.println(error);
  } else {
    Serial.println("Command ackowledged");
  }

  delay(10);
  //This is sending the configuration to the ADS1015
  Wire.beginTransmission(ADS_B); // Start I2C Transmission to the ADS1015
  Wire.write(CfgReg); // write the register address to point to the configuration register
  Wire.write(MSBcfg); // Write the MSB of the configuration register
  Wire.write(LSBcfg); // Write the LSB of the configuration register
  error = Wire.endTransmission(); // End the I2C Transmission
  if (error > 0) {
    Serial.print("Error: ");
    Serial.println(error);
  } else {
    Serial.println("Configuration Set for ADS_B");
  }
//======================ADS_C================================
  delay(10);
  //the following will disable the HI/LO Thresholds registers to use pin ## as the READY pin
  Wire.beginTransmission(ADS_C); // Start I2C Transmission to the ADS1015
  Wire.write(HiThreshReg); // 16bit register, so we must send the number in two parts
  Wire.write(highByte(HIOffTarget)); //Write the MSB of the HI Threshold
  Wire.write(lowByte(HIOffTarget)); //Write the LSB of the HI Threshold
  // Wire.write(0b10000000); // the 1 states that this is the MSB Set the HI Threshold to 0
  // Wire.write(0b00000000); // the 0 states that this is the LSB Set the HI Threshold to 0
  error = Wire.endTransmission(); // End the I2C Transmission
  if (error > 0) {
    Serial.print("Error: ");
    Serial.println(error);
  } else {
    Serial.println("Command ackowledged");
  }

  delay(10); //wait 10milliseconds before sending the next I2C transmission

  Wire.beginTransmission(ADS_C); // Start I2C Transmission to the ADS1015
  Wire.write(LoThreshReg); // 16bit register, so we must send the number in two parts
  Wire.write(highByte(LOOffTarget)); //Write the MSB of the LO Threshold
  Wire.write(lowByte(LOOffTarget)); //Write the LSB of the LO Threshold
  // Wire.write(0b01111111); // the 1 states that this is the MSB 
  // Wire.write(0b11111111); // the 0 states that this is the LSB 
  error = Wire.endTransmission(); // End the I2C Transmission
  if (error > 0) {
    Serial.print("Error: ");
    Serial.println(error);
  } else {
    Serial.println("Command ackowledged");
  }

  delay(10);
  //This is sending the configuration to the ADS1015
  Wire.beginTransmission(ADS_C); // Start I2C Transmission to the ADS1015
  Wire.write(CfgReg); // write the register address to point to the configuration register
  Wire.write(MSBcfg); // Write the MSB of the configuration register
  Wire.write(LSBcfg); // Write the LSB of the configuration register
  error = Wire.endTransmission(); // End the I2C Transmission
  if (error > 0) {
    Serial.print("Error: ");
    Serial.println(error);
  } else {
    Serial.println("Configuration Set for ADS_C");
  }
//======================ADS_D================================
  delay(10);
  //the following will disable the HI/LO Thresholds registers to use pin ## as the READY pin
  Wire.beginTransmission(ADS_D); // Start I2C Transmission to the ADS1015
  Wire.write(HiThreshReg); // 16bit register, so we must send the number in two parts
  Wire.write(highByte(HIOffTarget)); //Write the MSB of the HI Threshold
  Wire.write(lowByte(HIOffTarget)); //Write the LSB of the HI Threshold
  // Wire.write(0b10000000); // the 1 states that this is the MSB Set the HI Threshold to 0
  // Wire.write(0b00000000); // the 0 states that this is the LSB Set the HI Threshold to 0
  error = Wire.endTransmission(); // End the I2C Transmission
  if (error > 0) {
    Serial.print("Error: ");
    Serial.println(error);
  } else {
    Serial.println("Command ackowledged");
  }

  delay(10); //wait 10milliseconds before sending the next I2C transmission

  Wire.beginTransmission(ADS_D); // Start I2C Transmission to the ADS1015
  Wire.write(LoThreshReg); // 16bit register, so we must send the number in two parts
  Wire.write(highByte(LOOffTarget)); //Write the MSB of the LO Threshold
  Wire.write(lowByte(LOOffTarget)); //Write the LSB of the LO Threshold
  // Wire.write(0b01111111); // the 1 states that this is the MSB 
  // Wire.write(0b11111111); // the 0 states that this is the LSB 
  error = Wire.endTransmission(); // End the I2C Transmission
  if (error > 0) {
    Serial.print("Error: ");
    Serial.println(error);
  } else {
    Serial.println("Command ackowledged");
  }

  delay(10);
  //This is sending the configuration to the ADS1015
  Wire.beginTransmission(ADS_D); // Start I2C Transmission to the ADS1015
  Wire.write(CfgReg); // write the register address to point to the configuration register
  Wire.write(MSBcfg); // Write the MSB of the configuration register
  Wire.write(LSBcfg); // Write the LSB of the configuration register
  error = Wire.endTransmission(); // End the I2C Transmission
  if (error > 0) {
    Serial.print("Error: ");
    Serial.println(error);
  } else {
    Serial.println("Configuration Set for ADS_D");
  }
  
}

void Epee_ADS_Configurator(){
  // SET GAIN
  // SET SPS
  // SET COMPARATOR MODE (Window with HI and LO values, Ain0 - Ain3)
  const int HI = 1000; // 1V Convert the voltage to a 16bit value
  const int LO = 0;  // 0V Convert the voltage to a 16bit value
  const int HIOffTarget = 32760; // 4.095V Convert the voltage to a 15bit value 0.000125mVV per bit
  const int LOOffTarget = 20000; // 3.28V Convert the voltage to a 15bit value 0.000125V per bit
  Wire.begin();
  Wire.setClock(400000); //Increase I2C clock speed to 400kHz
  //======================ADS_A================================
  delay(10);
  //the following will disable the HI/LO Thresholds registers to use pin ## as the READY pin
  Wire.beginTransmission(ADS_A); // Start I2C Transmission to the ADS1015
  Wire.write(HiThreshReg); // 16bit register, so we must send the number in two parts
  Wire.write(highByte(HI)); //Write the MSB of the HI Threshold
  Wire.write(lowByte(HI)); //Write the LSB of the HI Threshold
  // Wire.write(0b10000000); // the 1 states that this is the MSB Set the HI Threshold to 0
  // Wire.write(0b00000000); // the 0 states that this is the LSB Set the HI Threshold to 0
  error = Wire.endTransmission(); // End the I2C Transmission
  if (error > 0) {
    Serial.print("Error: ");
    Serial.println(error);
    Serial.println(ADS_A);
    Serial.println(HiThreshReg);
  } else {
    Serial.println("Command ackowledged");
  }

  delay(10); //wait 10milliseconds before sending the next I2C transmission

  Wire.beginTransmission(ADS_A); // Start I2C Transmission to the ADS1015
  Wire.write(LoThreshReg); // 16bit register, so we must send the number in two parts
  Wire.write(highByte(LO)); //Write the MSB of the LO Threshold
  Wire.write(lowByte(LO)); //Write the LSB of the LO Threshold
  // Wire.write(0b01111111); // the 1 states that this is the MSB 
  // Wire.write(0b11111111); // the 0 states that this is the LSB 
  error = Wire.endTransmission(); // End the I2C Transmission
  if (error > 0) {
    Serial.print("Error: ");
    Serial.println(error);
  } else {
    Serial.println("Command ackowledged");
  }

  delay(10);
  //This is sending the configuration to the ADS1015
  Wire.beginTransmission(ADS_A); // Start I2C Transmission to the ADS1015
  Wire.write(CfgReg); // write the register address to point to the configuration register
  Wire.write(MSBcfgCOMPAR_A0_A3); // Write the MSB of the configuration register
  Wire.write(LSBcfg); // Write the LSB of the configuration register
  error = Wire.endTransmission(); // End the I2C Transmission
  if (error > 0) {
    Serial.print("Error: ");
    Serial.println(error);
  } else {
    Serial.println("Configuration Set for ADS_A");
  }
//======================ADS_B================================
  delay(10);
  //the following will disable the HI/LO Thresholds registers to use pin ## as the READY pin
  Wire.beginTransmission(ADS_B); // Start I2C Transmission to the ADS1015
  Wire.write(HiThreshReg); // 16bit register, so we must send the number in two parts
  Wire.write(highByte(HI)); //Write the MSB of the HI Threshold
  Wire.write(lowByte(HI)); //Write the LSB of the HI Threshold
  // Wire.write(0b10000000); // the 1 states that this is the MSB Set the HI Threshold to 0
  // Wire.write(0b00000000); // the 0 states that this is the LSB Set the HI Threshold to 0
  error = Wire.endTransmission(); // End the I2C Transmission
  if (error > 0) {
    Serial.print("Error: ");
    Serial.println(error);
  } else {
    Serial.println("Command ackowledged");
  }

  delay(10); //wait 10milliseconds before sending the next I2C transmission

  Wire.beginTransmission(ADS_B); // Start I2C Transmission to the ADS1015
  Wire.write(LoThreshReg); // 16bit register, so we must send the number in two parts
  Wire.write(highByte(LO)); //Write the MSB of the LO Threshold
  Wire.write(lowByte(LO)); //Write the LSB of the LO Threshold
  // Wire.write(0b01111111); // the 1 states that this is the MSB 
  // Wire.write(0b11111111); // the 0 states that this is the LSB 
  error = Wire.endTransmission(); // End the I2C Transmission
  if (error > 0) {
    Serial.print("Error: ");
    Serial.println(error);
  } else {
    Serial.println("Command ackowledged");
  }

  delay(10);
  //This is sending the configuration to the ADS1015
  Wire.beginTransmission(ADS_B); // Start I2C Transmission to the ADS1015
  Wire.write(CfgReg); // write the register address to point to the configuration register
  Wire.write(MSBcfgCOMPAR_A0_A3); // Write the MSB of the configuration register
  Wire.write(LSBcfg); // Write the LSB of the configuration register
  error = Wire.endTransmission(); // End the I2C Transmission
  if (error > 0) {
    Serial.print("Error: ");
    Serial.println(error);
  } else {
    Serial.println("Configuration Set for ADS_B");
  }
//======================ADS_C================================
  delay(10);
  //the following will disable the HI/LO Thresholds registers to use pin ## as the READY pin
  Wire.beginTransmission(ADS_C); // Start I2C Transmission to the ADS1015
  Wire.write(HiThreshReg); // 16bit register, so we must send the number in two parts
  Wire.write(highByte(HIOffTarget)); //Write the MSB of the HI Threshold
  Wire.write(lowByte(HIOffTarget)); //Write the LSB of the HI Threshold
  // Wire.write(0b10000000); // the 1 states that this is the MSB Set the HI Threshold to 0
  // Wire.write(0b00000000); // the 0 states that this is the LSB Set the HI Threshold to 0
  error = Wire.endTransmission(); // End the I2C Transmission
  if (error > 0) {
    Serial.print("Error: ");
    Serial.println(error);
  } else {
    Serial.println("Command ackowledged");
  }

  delay(10); //wait 10milliseconds before sending the next I2C transmission

  Wire.beginTransmission(ADS_C); // Start I2C Transmission to the ADS1015
  Wire.write(LoThreshReg); // 16bit register, so we must send the number in two parts
  Wire.write(highByte(LOOffTarget)); //Write the MSB of the LO Threshold
  Wire.write(lowByte(LOOffTarget)); //Write the LSB of the LO Threshold
  // Wire.write(0b01111111); // the 1 states that this is the MSB 
  // Wire.write(0b11111111); // the 0 states that this is the LSB 
  error = Wire.endTransmission(); // End the I2C Transmission
  if (error > 0) {
    Serial.print("Error: ");
    Serial.println(error);
  } else {
    Serial.println("Command ackowledged");
  }

  delay(10);
  //This is sending the configuration to the ADS1015
  Wire.beginTransmission(ADS_C); // Start I2C Transmission to the ADS1015
  Wire.write(CfgReg); // write the register address to point to the configuration register
  Wire.write(MSBcfg); // Write the MSB of the configuration register
  Wire.write(LSBcfg); // Write the LSB of the configuration register
  error = Wire.endTransmission(); // End the I2C Transmission
  if (error > 0) {
    Serial.print("Error: ");
    Serial.println(error);
  } else {
    Serial.println("Configuration Set for ADS_C");
  }
//======================ADS_D================================
  delay(10);
  //the following will disable the HI/LO Thresholds registers to use pin ## as the READY pin
  Wire.beginTransmission(ADS_D); // Start I2C Transmission to the ADS1015
  Wire.write(HiThreshReg); // 16bit register, so we must send the number in two parts
  Wire.write(highByte(HIOffTarget)); //Write the MSB of the HI Threshold
  Wire.write(lowByte(HIOffTarget)); //Write the LSB of the HI Threshold
  // Wire.write(0b10000000); // the 1 states that this is the MSB Set the HI Threshold to 0
  // Wire.write(0b00000000); // the 0 states that this is the LSB Set the HI Threshold to 0
  error = Wire.endTransmission(); // End the I2C Transmission
  if (error > 0) {
    Serial.print("Error: ");
    Serial.println(error);
  } else {
    Serial.println("Command ackowledged");
  }

  delay(10); //wait 10milliseconds before sending the next I2C transmission

  Wire.beginTransmission(ADS_D); // Start I2C Transmission to the ADS1015
  Wire.write(LoThreshReg); // 16bit register, so we must send the number in two parts
  Wire.write(highByte(LOOffTarget)); //Write the MSB of the LO Threshold
  Wire.write(lowByte(LOOffTarget)); //Write the LSB of the LO Threshold
  // Wire.write(0b01111111); // the 1 states that this is the MSB 
  // Wire.write(0b11111111); // the 0 states that this is the LSB 
  error = Wire.endTransmission(); // End the I2C Transmission
  if (error > 0) {
    Serial.print("Error: ");
    Serial.println(error);
  } else {
    Serial.println("Command ackowledged");
  }

  delay(10);
  //This is sending the configuration to the ADS1015
  Wire.beginTransmission(ADS_D); // Start I2C Transmission to the ADS1015
  Wire.write(CfgReg); // write the register address to point to the configuration register
  Wire.write(MSBcfg); // Write the MSB of the configuration register
  Wire.write(LSBcfg); // Write the LSB of the configuration register
  error = Wire.endTransmission(); // End the I2C Transmission
  if (error > 0) {
    Serial.print("Error: ");
    Serial.println(error);
  } else {
    Serial.println("Configuration Set for ADS_D");
  }


}

void Sabre_ADS_Configurator(){
  // SET GAIN
  // SET SPS
  // SET COMPARATOR MODE (Window with HI and LO values, Ain0 - Ain3)
  const int HI = 1000; // 1V Convert the voltage to a 16bit value
  const int LO = 1;  // 0V Convert the voltage to a 16bit value
  const int HIOffTarget = 32500; // 4.095V Convert the voltage to a 15bit value 0.000125mVV per bit
  const int LOOffTarget = 20000; // 3.00V Convert the voltage to a 15bit value 0.000125V per bit
  Wire.begin();
  Wire.setClock(400000); //Increase I2C clock speed to 400kHz
//======================ADS_A================================
  delay(10);
  //the following will disable the HI/LO Thresholds registers to use pin ## as the READY pin
  Wire.beginTransmission(ADS_A); // Start I2C Transmission to the ADS1015
  Wire.write(HiThreshReg); // 16bit register, so we must send the number in two parts
  Wire.write(highByte(HI)); //Write the MSB of the HI Threshold
  Wire.write(lowByte(HI)); //Write the LSB of the HI Threshold
  // Wire.write(0b10000000); // the 1 states that this is the MSB Set the HI Threshold to 0
  // Wire.write(0b00000000); // the 0 states that this is the LSB Set the HI Threshold to 0
  error = Wire.endTransmission(); // End the I2C Transmission
  if (error > 0) {
    Serial.print("Error: ");
    Serial.println(error);
    Serial.println(ADS_A);
    Serial.println(HiThreshReg);
  } else {
    Serial.println("Command ackowledged");
  }

  delay(10); //wait 10milliseconds before sending the next I2C transmission

  Wire.beginTransmission(ADS_A); // Start I2C Transmission to the ADS1015
  Wire.write(LoThreshReg); // 16bit register, so we must send the number in two parts
  Wire.write(highByte(LO)); //Write the MSB of the LO Threshold
  Wire.write(lowByte(LO)); //Write the LSB of the LO Threshold
  // Wire.write(0b01111111); // the 1 states that this is the MSB 
  // Wire.write(0b11111111); // the 0 states that this is the LSB 
  error = Wire.endTransmission(); // End the I2C Transmission
  if (error > 0) {
    Serial.print("Error: ");
    Serial.println(error);
  } else {
    Serial.println("Command ackowledged");
  }

  delay(10);
  //This is sending the configuration to the ADS1015
  Wire.beginTransmission(ADS_A); // Start I2C Transmission to the ADS1015
  Wire.write(CfgReg); // write the register address to point to the configuration register
  Wire.write(MSBcfgCOMPAR_A0_A1); // Write the MSB of the configuration register
  Wire.write(LSBcfg); // Write the LSB of the configuration register
  error = Wire.endTransmission(); // End the I2C Transmission
  if (error > 0) {
    Serial.print("Error: ");
    Serial.println(error);
  } else {
    Serial.println("Configuration Set for ADS_A");
  }
//======================ADS_B================================
  delay(10);
  //the following will disable the HI/LO Thresholds registers to use pin ## as the READY pin
  Wire.beginTransmission(ADS_B); // Start I2C Transmission to the ADS1015
  Wire.write(HiThreshReg); // 16bit register, so we must send the number in two parts
  Wire.write(highByte(HI)); //Write the MSB of the HI Threshold
  Wire.write(lowByte(HI)); //Write the LSB of the HI Threshold
  // Wire.write(0b10000000); // the 1 states that this is the MSB Set the HI Threshold to 0
  // Wire.write(0b00000000); // the 0 states that this is the LSB Set the HI Threshold to 0
  error = Wire.endTransmission(); // End the I2C Transmission
  if (error > 0) {
    Serial.print("Error: ");
    Serial.println(error);
  } else {
    Serial.println("Command ackowledged");
  }

  delay(10); //wait 10milliseconds before sending the next I2C transmission

  Wire.beginTransmission(ADS_B); // Start I2C Transmission to the ADS1015
  Wire.write(LoThreshReg); // 16bit register, so we must send the number in two parts
  Wire.write(highByte(LO)); //Write the MSB of the LO Threshold
  Wire.write(lowByte(LO)); //Write the LSB of the LO Threshold
  // Wire.write(0b01111111); // the 1 states that this is the MSB 
  // Wire.write(0b11111111); // the 0 states that this is the LSB 
  error = Wire.endTransmission(); // End the I2C Transmission
  if (error > 0) {
    Serial.print("Error: ");
    Serial.println(error);
  } else {
    Serial.println("Command ackowledged");
  }

  delay(10);
  //This is sending the configuration to the ADS1015
  Wire.beginTransmission(ADS_B); // Start I2C Transmission to the ADS1015
  Wire.write(CfgReg); // write the register address to point to the configuration register
  Wire.write(MSBcfgCOMPAR_A0_A1); // Write the MSB of the configuration register
  Wire.write(LSBcfg); // Write the LSB of the configuration register
  error = Wire.endTransmission(); // End the I2C Transmission
  if (error > 0) {
    Serial.print("Error: ");
    Serial.println(error);
  } else {
    Serial.println("Configuration Set for ADS_B");
  }
//======================ADS_C================================
  delay(10);
  //the following will disable the HI/LO Thresholds registers to use pin ## as the READY pin
  Wire.beginTransmission(ADS_C); // Start I2C Transmission to the ADS1015
  Wire.write(HiThreshReg); // 16bit register, so we must send the number in two parts
  Wire.write(highByte(HIOffTarget)); //Write the MSB of the HI Threshold
  Wire.write(lowByte(HIOffTarget)); //Write the LSB of the HI Threshold
  // Wire.write(0b10000000); // the 1 states that this is the MSB Set the HI Threshold to 0
  // Wire.write(0b00000000); // the 0 states that this is the LSB Set the HI Threshold to 0
  error = Wire.endTransmission(); // End the I2C Transmission
  if (error > 0) {
    Serial.print("Error: ");
    Serial.println(error);
  } else {
    Serial.println("Command ackowledged");
  }

  delay(10); //wait 10milliseconds before sending the next I2C transmission

  Wire.beginTransmission(ADS_C); // Start I2C Transmission to the ADS1015
  Wire.write(LoThreshReg); // 16bit register, so we must send the number in two parts
  Wire.write(highByte(LOOffTarget)); //Write the MSB of the LO Threshold
  Wire.write(lowByte(LOOffTarget)); //Write the LSB of the LO Threshold
  // Wire.write(0b01111111); // the 1 states that this is the MSB 
  // Wire.write(0b11111111); // the 0 states that this is the LSB 
  error = Wire.endTransmission(); // End the I2C Transmission
  if (error > 0) {
    Serial.print("Error: ");
    Serial.println(error);
  } else {
    Serial.println("Command ackowledged");
  }

  delay(10);
  //This is sending the configuration to the ADS1015
  Wire.beginTransmission(ADS_C); // Start I2C Transmission to the ADS1015
  Wire.write(CfgReg); // write the register address to point to the configuration register
  Wire.write(MSBcfg); // Write the MSB of the configuration register
  Wire.write(LSBcfg); // Write the LSB of the configuration register
  error = Wire.endTransmission(); // End the I2C Transmission
  if (error > 0) {
    Serial.print("Error: ");
    Serial.println(error);
  } else {
    Serial.println("Configuration Set for ADS_C");
  }
//======================ADS_D================================
  delay(10);
  //the following will disable the HI/LO Thresholds registers to use pin ## as the READY pin
  Wire.beginTransmission(ADS_D); // Start I2C Transmission to the ADS1015
  Wire.write(HiThreshReg); // 16bit register, so we must send the number in two parts
  Wire.write(highByte(HIOffTarget)); //Write the MSB of the HI Threshold
  Wire.write(lowByte(HIOffTarget)); //Write the LSB of the HI Threshold
  // Wire.write(0b10000000); // the 1 states that this is the MSB Set the HI Threshold to 0
  // Wire.write(0b00000000); // the 0 states that this is the LSB Set the HI Threshold to 0
  error = Wire.endTransmission(); // End the I2C Transmission
  if (error > 0) {
    Serial.print("Error: ");
    Serial.println(error);
  } else {
    Serial.println("Command ackowledged");
  }

  delay(10); //wait 10milliseconds before sending the next I2C transmission

  Wire.beginTransmission(ADS_D); // Start I2C Transmission to the ADS1015
  Wire.write(LoThreshReg); // 16bit register, so we must send the number in two parts
  Wire.write(highByte(LOOffTarget)); //Write the MSB of the LO Threshold
  Wire.write(lowByte(LOOffTarget)); //Write the LSB of the LO Threshold
  // Wire.write(0b01111111); // the 1 states that this is the MSB 
  // Wire.write(0b11111111); // the 0 states that this is the LSB 
  error = Wire.endTransmission(); // End the I2C Transmission
  if (error > 0) {
    Serial.print("Error: ");
    Serial.println(error);
  } else {
    Serial.println("Command ackowledged");
  }

  delay(10);
  //This is sending the configuration to the ADS1015
  Wire.beginTransmission(ADS_D); // Start I2C Transmission to the ADS1015
  Wire.write(CfgReg); // write the register address to point to the configuration register
  Wire.write(MSBcfg); // Write the MSB of the configuration register
  Wire.write(LSBcfg); // Write the LSB of the configuration register
  error = Wire.endTransmission(); // End the I2C Transmission
  if (error > 0) {
    Serial.print("Error: ");
    Serial.println(error);
  } else {
    Serial.println("Configuration Set for ADS_D");
  }


}

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
//==========================
// ISR Functions
//==========================
void ISR_OnTargetA(){//This function is potentially being called very often 
  OnTargetA_Flag = true;
  Hit_Type_ISR_StateChanged = true;
}
void ISR_OnTargetB(){
  OnTargetB_Flag = true;
  Hit_Type_ISR_StateChanged = true;
}
void ISR_OffTargetA(){
  OffTargetA_Flag = true;
  Hit_Type_ISR_StateChanged = true;
}
void ISR_OffTargetB(){
  OffTargetB_Flag = true;
  Hit_Type_ISR_StateChanged = true;
}
void BUTTON_ISR() {
  Mode_ISR_StateChanged = true;
}

//==========================
// Weapon Logic
//==========================
void handleFoilHit() {
  //Serial.println("Inside: Handle Foil Function");
  // int16_t weaponA, lameA, groundA, adc3;
  // int16_t weaponB, lameB, groundB, adc7;
  // weaponA = ads1015_A.readADC_SingleEnded(0);    // Read from channel 0 of the first ADS1015
  // lameA = ads1015_A.readADC_SingleEnded(1);    // Read from channel 1 of the first ADS1015
  // groundA = ads1015_A.readADC_SingleEnded(2);    // Read from channel 2 of the first ADS1015

  // weaponB = ads1015_B.readADC_SingleEnded(0);   // Read from channel 0 of the second ADS1015
  // lameB = ads1015_B.readADC_SingleEnded(1);   // Read from channel 1 of the second ADS1015
  // groundB = ads1015_B.readADC_SingleEnded(2);   // Read from channel 2 of the second ADS1015
  // Serial.print("Default ADS1015 (0x48) weaponA 0: "); Serial.println(weaponA);
  // Serial.print("Default ADS1015 (0x48) lameA 1: "); Serial.println(lameA);
  // Serial.print("Default ADS1015 (0x48) groundA 2: "); Serial.println(groundA);

  // Serial.print("ADDR ADS1015 (0x49) weaponB 4: "); Serial.println(weaponB);
  // Serial.print("ADDR ADS1015 (0x49) lameB 5: "); Serial.println(lameB);
  // Serial.print("ADDR ADS1015 (0x49) groundB 6: "); Serial.println(groundB);


  long now = millis();  // Arduino uses millis() to get the number of milliseconds since the board started running.
  // Serial.println(lockedOut);
  //_____Check for lockout___
  if (((Hit_On_Target_A || Hit_Off_Target_A) && (TimeOfLockout < now))
      || ((Hit_On_Target_B || Hit_Off_Target_B) && (TimeOfLockout < now))) {
    lockedOut = true;
    //TimeResetWasCalled = millis();
    return;  // exit the function if we are locked out
  }
  // This if Statement will make sure that you cannon get a light 
  // just from touching the side of the blade on the lame in FOIL.
  if ((OffTargetA_Flag && OnTargetA_Flag)||(OffTargetB_Flag && OnTargetB_Flag)){ 
    OffTargetA_Flag = false;
    OnTargetA_Flag = false;
    OffTargetB_Flag = false;
    OnTargetB_Flag = false;
    return;
  }
  // ___Weapon A___
  if (!Hit_On_Target_A && !Hit_Off_Target_A) {
    // Off target              
    if ((OffTargetA_Flag && !OnTargetA_Flag)) { 
      Hit_Off_Target_A = true;                        // Hit_Off_Target_A
      TimeOfLockout = millis() + foilMode.lockoutTime;
      Hit_Time_A = now;  // Record the time of the hit
    }
    // On target
    else if (OnTargetA_Flag && !OffTargetA_Flag) {  
      Hit_On_Target_A = true;                                                                                                  // Hit_On_Target_A
      TimeOfLockout = millis() + foilMode.lockoutTime;
      Hit_Time_A = now;  // Record the time of the hit
    }
  }
  // ___Weapon B___
  if (!Hit_On_Target_B && !Hit_Off_Target_B) {
    // Off target
    if ((OffTargetB_Flag && !OnTargetB_Flag)) {  
      Hit_Off_Target_B = true;                        
      TimeOfLockout = millis() + foilMode.lockoutTime;
      Hit_Time_B = now;  // Record the time of the hit
      // On target
    } else if (OnTargetB_Flag && !OffTargetB_Flag) {  // Tested Value for lameA is 833
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
  // weaponA = ads1015_A.readADC_SingleEnded(0);    // Read from channel 0 of the first ADS1015
  // lameA = ads1015_A.readADC_SingleEnded(1);    // Read from channel 1 of the first ADS1015
  // groundA = ads1015_A.readADC_SingleEnded(2);    // Read from channel 2 of the first ADS1015

  // weaponB = ads1015_B.readADC_SingleEnded(0);   // Read from channel 0 of the second ADS1015
  // lameB = ads1015_B.readADC_SingleEnded(1);   // Read from channel 1 of the second ADS1015
  // groundB = ads1015_B.readADC_SingleEnded(2);   // Read from channel 2 of the second ADS1015
  // Serial.println(lockedOut);
  // Serial.println(Hit_On_Target_A);
  // Serial.println(Hit_On_Target_B);
  // Serial.print("Default ADS1015 (0x48) weaponA 0: "); Serial.println(weaponA);
  // Serial.print("Default ADS1015 (0x48) lameA 1: "); Serial.println(lameA);
  // Serial.print("Default ADS1015 (0x48) groundA 2: "); Serial.println(groundA);

  // Serial.print("ADDR ADS1015 (0x49) weaponB 4: "); Serial.println(weaponB);
  // Serial.print("ADDR ADS1015 (0x49) lameB 5: "); Serial.println(lameB);
  // Serial.print("ADDR ADS1015 (0x49) groundB 6: "); Serial.println(groundB);
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
    if (OnTargetA_Flag) {
      Hit_On_Target_A = true;  // Hit_On_Target_A
      //TimeOfLockout = millis() + epeeMode.lockoutTime;
      Hit_Time_A = now;  // Record the time of the hit
    }
  }
  // weapon B
  //  no hit for B yet    && weapon depress    && opponent lame touched
  if (!Hit_On_Target_B) {
    if (OnTargetB_Flag) {
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
  // weaponA = ads1015_A.readADC_SingleEnded(0);    // Read from channel 0 of the first ADS1015
  // lameA = ads1015_A.readADC_SingleEnded(1);    // Read from channel 1 of the first ADS1015
  // groundA = ads1015_A.readADC_SingleEnded(2);    // Read from channel 2 of the first ADS1015

  // weaponB = ads1015_B.readADC_SingleEnded(0);   // Read from channel 0 of the second ADS1015
  // lameB = ads1015_B.readADC_SingleEnded(1);   // Read from channel 1 of the second ADS1015
  // groundB = ads1015_B.readADC_SingleEnded(2);   // Read from channel 2 of the second ADS1015
  // Serial.print("Default ADS1015 (0x48) weaponA 0: "); Serial.println(weaponA);
  // Serial.print("Default ADS1015 (0x48) lameA 1: "); Serial.println(lameA);
  // Serial.print("Default ADS1015 (0x48) groundA 2: "); Serial.println(groundA);

  // Serial.print("ADDR ADS1015 (0x49) weaponB 4: "); Serial.println(weaponB);
  // Serial.print("ADDR ADS1015 (0x49) lameB 5: "); Serial.println(lameB);
  // Serial.print("ADDR ADS1015 (0x49) groundB 6: "); Serial.println(groundB);
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
    if (OffTargetA_Flag) {
      Hit_Off_Target_A = true;  // Hit_Off_Target_A
      // On target
    } else if (OnTargetA_Flag) {
      Hit_On_Target_A = true;  // Hit_On_Target_A
      // TimeOfLockout = millis() + sabreMode.lockoutTime;
      Hit_Time_A = now;
    }
  }
  // ___Weapon B___
  if (!Hit_On_Target_B || !Hit_Off_Target_B) {
    // Off target
    if (OffTargetB_Flag) {
      Hit_Off_Target_B = true;  // Hit_Off_Target_B
    }
    // On target          // 500                                                 // 500
    else if (OnTargetB_Flag) {
      Hit_On_Target_B = true;  // Hit_On_Target_B
      // TimeOfLockout = millis() + sabreMode.lockoutTime;
      Hit_Time_B = now;
    }
  }
}

//==========================
// Calls Foil,Epee,Sabre Functions
// will pint to a Mode instance in struct Mode
//==========================
void Handle_Hit() {
  currentMode->Handle_Hit();  // will pint to a Mode instance in struct Mode
}

//==========================
// Mode Change Function
//==========================
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
    Epee_ADS_Configurator();  // This will set the ADS1015 to the Epee Mode Configuration
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
    Sabre_ADS_Configurator();  // This will set the ADS1015 to the Sabre Mode Configuration
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
    Foil_ADS_Configurator();  // This will set the ADS1015 to the Foil Mode Configuration
    Serial.println("changed mode was called: to Foil");
    currentMode = &foilMode;
  }
}


//==========================
// Debounce Function
//==========================
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
  OnTargetA_Flag = false;
  OnTargetB_Flag = false;
  OffTargetA_Flag = false;
  OffTargetB_Flag = false;
}

void ResetValues() {
  static unsigned long buzzerOFFTime = 0;
  static unsigned long lightsOFFTime = 0;
  static bool isBuzzerOFF = false;
  if (TimeResetWasCalled - lastResetTime >= LIGHTTIME) {  // This makes sure that the buzzer is turned off
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
      OnTargetA_Flag = false;
      OnTargetB_Flag = false;
      OffTargetA_Flag = false;
      OffTargetB_Flag = false;
      lastResetTime = TimeResetWasCalled;
      for (int i = 0; i <= 2; i++) {  // Turn off the previous mode LED
        pixels[i] = CRGB::Black;
      }
      pixels[currentMode->ModeLED] = CRGB(255, 0, 255);  // Turn on the light for the current mode
      FastLED.show();
      isBuzzerOFF = false;
    }
  }
}

void setup() {
  Serial.begin(9600);
  setCpuFrequencyMhz(240); //Set CPU clock to 240MHz fo example
  pinMode(WeaponA_ONTARGET_ALRT_PIN, INPUT); // Set the RDY pin as an input
  pinMode(WeaponB_ONTARGET_ALRT_PIN, INPUT); // Set the RDY pin as an input
  pinMode(WeaponA_OFFTARGET_ALRT_PIN, INPUT); // Set the RDY pin as an input
  pinMode(WeaponB_OFFTARGET_ALRT_PIN, INPUT); // Set the RDY pin as an input
  Serial.println("Score_Box_V5_with_ALRT_PIN");
  Serial.println("Getting single-ended readings from AIN0..3");
  Serial.println("ADC GAIN_ONE Range: +/- 4.096V (1 bit = 0.125mV/ADS1015)");
  pinMode(Mode_Button_PIN, INPUT_PULLDOWN);
  pinMode(buzzerPin, OUTPUT);
  Serial.println("Three Weapon Scoring Box");
  Serial.println("================");
  // //FAST LED SETUP
  FastLED.addLeds<CHIPSET, DATA_PIN, COLOR_ORDER>(pixels222, NUMPIXELS);
  FastLED.addLeds<CHIPSET, DATA_PIN2, COLOR_ORDER>(pixels, NUMPIXELS);
  FastLED.setBrightness(BRIGHTNESS);
  // Set an initial mode, for example, FOIL
  Mode_Change();  // Epee on start up
  Mode_Change();  // Sabre on start up
  Mode_Change();  // Foil on start up
    // Set up the interrupt pins
  attachInterrupt(digitalPinToInterrupt(Mode_Button_PIN), BUTTON_ISR, FALLING); // Pin is set to Pull_Down and will recieve a HIGH signal when pressed, for less noise we will only trigger when High is going to Low
  attachInterrupt(digitalPinToInterrupt(WeaponA_ONTARGET_ALRT_PIN), ISR_OnTargetA, CHANGE); // Delta of A0 and A1 will go from Delta = Large value to Delta = 0
  attachInterrupt(digitalPinToInterrupt(WeaponB_ONTARGET_ALRT_PIN), ISR_OnTargetB, CHANGE);
  attachInterrupt(digitalPinToInterrupt(WeaponA_OFFTARGET_ALRT_PIN), ISR_OffTargetA, CHANGE); // Delta of A0 and GND will go from Delta = Large value to Delta = 0
  attachInterrupt(digitalPinToInterrupt(WeaponB_OFFTARGET_ALRT_PIN), ISR_OffTargetB, CHANGE);
  // ADS_Names.begin(Adress);
  // SET GAIN
  // SET SPS
  // SET COMPARATOR MODE (Window with HI and LO values, Ain0 - Ain3)
  Foil_ADS_Configurator();
  // Epee_ADS_Configurator();
  // Sabre_ADS_Configurator();
  ResetValues();
}

void loop() {
  unsigned long now = millis();
  //This loop should call one function that is a pointer to the mode that has been selected
  //This function should be called in a switch statement
  //A function that checks the if any interrupts have set a flag to Ture
  //============================TESTING=========================================
  // int16_t weaponA, lameA, groundA, weaponB, lameB, groundB;
  // weaponA = ads_OnTarget.readADC_SingleEnded(0);
  // lameA = ads_OnTarget.readADC_SingleEnded(1);
  // weaponB = ads_OnTarget.readADC_SingleEnded(2);
  // lameB = ads_OnTarget.readADC_SingleEnded(3);

  // weaponA = ads_OffTarget.readADC_SingleEnded(0);
  // lameA = ads_OffTarget.readADC_SingleEnded(1);
  // weaponB = ads_OffTarget.readADC_SingleEnded(2);
  // lameB = ads_OffTarget.readADC_SingleEnded(3);
  
  // Serial.print("ads_OnTarget (0x48) Channel 0 - weaponA: "); Serial.println(weaponA);
  // Serial.print("ads_OnTarget (0x48) Channel 1 - weaponB: "); Serial.println(weaponB);
  // Serial.print("ads_OnTarget (0x48) Channel 2 - lameA: "); Serial.println(lameA);
  // Serial.print("ads_OffTarget (0x49) Channel 3 - lameB: "); Serial.println(lameB);

  // Serial.print("ads_OffTarget (0x49) Channel 4 - weaponA: "); Serial.println(weaponA);
  // Serial.print("ads_OffTarget (0x49) Channel 5 - weaponB: "); Serial.println(weaponB);
  // Serial.print("ads_OffTarget (0x49) Channel 6 - lameA: "); Serial.println(lameA);
  // Serial.print("ads_OffTarget (0x49) Channel 7 - lameB: "); Serial.println(lameB);
  //============================TESTING=========================================

  if(Mode_ISR_StateChanged){
    if (BUTTON_Debounce(Mode_Button_PIN)) {
      Mode_Change();
      Mode_ISR_StateChanged = false;
    }
  }
  // Check if any interrupts have set a flag to True
  // If so, call the Handle_Hit function

  Handle_Hit();

  // This Switch is used to turn on the correct LEDS
  if (now - last_update > Update_Interval) {
    last_update = now;
    number =  (Hit_On_Target_A * OnTargetA_Flag_Value) + 
              (Hit_On_Target_B * OnTargetB_Flag_Value) + 
              (Hit_Off_Target_A * OffTargetA_Flag_Value) + 
              (Hit_Off_Target_B * OffTargetB_Flag_Value);
    
    switch (number) {
      case OnTargetA_Flag_Value:
        // Serial.println("ON TARGET A");
        fill_solid(pixels222, NUMPIXELS, CRGB::Green);  // Moderately bright GREEN color.
        FastLED.show();                                 // This sends the updated pixel color to the hardware.
        digitalWrite(buzzerPin, HIGH);
        tone(buzzerPin, 1000, BUZZERTIME); // Pin , Frequency, Duration in mS
        break;
      case OnTargetB_Flag_Value:
        // Serial.println("ON TARGET B");
        fill_solid(pixels, NUMPIXELS, CRGB::Red);  // Moderately bright RED color.
        FastLED.show();                            // This sends the updated pixel color to the hardware.
        digitalWrite(buzzerPin, HIGH);
        break;
      case OffTargetA_Flag_Value:
        // Serial.println("OFF TARGET A");
        fill_solid(pixels222, NUMPIXELS, CRGB::White);  // Yellow color.
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
        fill_solid(pixels, NUMPIXELS, CRGB::White);  // Bright Blue color.
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
        // Serial.println("ON TARGET A AND OFF TARGET B");
        fill_solid(pixels222, NUMPIXELS, CRGB::Green);  // Moderately bright GREEN color.
        fill_solid(pixels, NUMPIXELS, CRGB::White);    // Bright Blue color.
        FastLED.show();                                 // This sends the updated pixel color to the hardware.
        digitalWrite(buzzerPin, HIGH);
        break;
      case OnTargetA_Flag_Value + OffTargetA_Flag_Value:
        // Serial.println("ON TARGET A AND OFF TARGET A");
        fill_solid(pixels222, NUMPIXELS, CRGB::Green);   // Moderately bright GREEN color.
        fill_solid(pixels222, NUMPIXELS, CRGB::White);  // Yellow color.
        FastLED.show();                                  // This sends the updated pixel color to the hardware.
        digitalWrite(buzzerPin, HIGH);
        break;
      case OnTargetB_Flag_Value + OffTargetA_Flag_Value:
        // Serial.println("ON TARGET B AND OFF TARGET A");
        fill_solid(pixels, NUMPIXELS, CRGB::Red);        // Moderately bright RED color.
        fill_solid(pixels222, NUMPIXELS, CRGB::White);  // Yellow color.
        FastLED.show();                                  // This sends the updated pixel color to the hardware.
        digitalWrite(buzzerPin, HIGH);
        break;
      case OnTargetB_Flag_Value + OffTargetB_Flag_Value:
        // Serial.println("ON TARGET B AND OFF TARGET B");
        fill_solid(pixels, NUMPIXELS, CRGB::Red);     // Moderately bright RED color.
        fill_solid(pixels, NUMPIXELS, CRGB::White);  // Bright Blue color.
        FastLED.show();                               // This sends the updated pixel color to the hardware.
        digitalWrite(buzzerPin, HIGH);
        break;
      case OffTargetA_Flag_Value + OffTargetB_Flag_Value:
        // Serial.println("OFF TTARGET A AND OFF TARGET B");
        fill_solid(pixels, NUMPIXELS, CRGB::White);     // Bright Blue color.
        fill_solid(pixels222, NUMPIXELS, CRGB::White);  // Yellow color.
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

  //============================TESTING=========================================
  // //Request Data
  // Wire.beginTransmission(ADS_A); // Start I2C Transmission to the ADS1115
  // Wire.write(ConvReg); //point to the convergen register to read conversion data
  // Wire.endTransmission(); // End the I2C Transmission
  // Wire.requestFrom(ADS_A, 2); // Request 2 bytes from the ADS1115 (MSB then the LSB)
  // /* Since the ADS1115 sends the MSB first we need to Read the first byte (MSB) and shift it 8 bits to the left
  // then we read the second byte (LSB) and add it to the first byte. This will give us the 16bit value of the conversion
  // */
  // float RawValue_ADS_A = Wire.read() << 4 | Wire.read(); // Read the MSB and shift it 8 bits to the left, then read the LSB and add it to the MSB
  // RawValue_ADS_A = RawValue_ADS_A * 0.002; // Convert the 11bit value to a voltage

  // Wire.beginTransmission(ADS_B);
  // Wire.write(ConvReg);
  // Wire.endTransmission();
  // Wire.requestFrom(ADS_B, 2); //Request the LSB of the data from ADS_C
  // float RawValue_ADS_B = Wire.read() << 4 | Wire.read(); // Read the MSB and shift it 8 bits to the left, then read the LSB and add it to the MSB
  // RawValue_ADS_B = RawValue_ADS_B * 0.002; // Convert the 11bit value to a voltage

  // Wire.beginTransmission(ADS_C);
  // Wire.write(ConvReg);
  // Wire.endTransmission();
  // Wire.requestFrom(ADS_C, 2); //Request the LSB of the data from ADS_C
  // float RawValue_ADS_C = Wire.read() << 4 | Wire.read(); // Read the MSB and shift it 8 bits to the left, then read the LSB and add it to the MSB
  // RawValue_ADS_C = RawValue_ADS_C * 0.002; // Convert the 11bit value to a voltage

  // Wire.beginTransmission(ADS_D);
  // Wire.write(ConvReg);
  // Wire.endTransmission();
  // Wire.requestFrom(ADS_D, 2); //Request the LSB of the data from ADS_C
  // float RawValue_ADS_D = Wire.read() << 4 | Wire.read(); // Read the MSB and shift it 8 bits to the left, then read the LSB and add it to the MSB
  // RawValue_ADS_D = RawValue_ADS_D * 0.002; // Convert the 11bit value to a voltage


  // if (OnTargetA_Flag||OnTargetB_Flag||OffTargetA_Flag||OffTargetB_Flag) {
  // Serial.print(OnTargetA_Flag); Serial.print('|'); Serial.print(OnTargetB_Flag); Serial.print('|'); Serial.print(OffTargetA_Flag); Serial.print('|'); Serial.println(OffTargetB_Flag);
  // Serial.print(',');
  // Serial.print(RawValue_ADS_A);
  // Serial.print(',');
  // Serial.print(RawValue_ADS_B);
  // Serial.print(',');
  // Serial.print(RawValue_ADS_C);
  // Serial.print(',');
  // Serial.println(RawValue_ADS_D);
  // }
  // String serData = String("RawValue_ADS_A  : ") + RawValue_ADS_A  + "\n"
  //                       + "RawValue_ADS_B  : "  + RawValue_ADS_B  + "\n"
  //                       + "RawValue_ADS_C  : "  + RawValue_ADS_C  + "\n"
  //                       + "RawValue_ADS_D  : "  + RawValue_ADS_D  + "\n"
  //                       + "Number          : "  + number  + "\n"
  //                       + "Locked Out      : "  + lockedOut   + "\n";
  // Serial.println(serData);
  //============================TESTING=========================================

  // // Check if it's time to reset after LIGHTTIME has elapsed since the last hit
  if (lockedOut && ((now - max(Hit_Time_A, Hit_Time_B) > LIGHTTIME) || (Hit_On_Target_A && now - Hit_Time_A > LIGHTTIME) || (Hit_On_Target_B && now - Hit_Time_B > LIGHTTIME))) {
    ResetValues();  // This will turn off LEDs and reset hit flags
    TimeResetWasCalled = millis();
  }
  OnTargetA_Flag = digitalRead(WeaponA_ONTARGET_ALRT_PIN);
  OnTargetB_Flag = digitalRead(WeaponB_ONTARGET_ALRT_PIN);
  OffTargetA_Flag = digitalRead(WeaponA_OFFTARGET_ALRT_PIN);
  OffTargetB_Flag = digitalRead(WeaponB_OFFTARGET_ALRT_PIN);
  // Serial.print(OnTargetA_Flag); Serial.print(" | "); Serial.print(OnTargetB_Flag); Serial.print(" | "); Serial.print(OffTargetA_Flag); Serial.print(" | "); Serial.println(OffTargetB_Flag);
}

