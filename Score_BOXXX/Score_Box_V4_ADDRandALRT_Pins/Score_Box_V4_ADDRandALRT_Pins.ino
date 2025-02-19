//Score_Box_V4_ADDRandALRT_Pins
#include <Wire.h>

#define ADS_A 0x48 // ADDR Connected to GND
#define ADS_B 0x49 // ADDR Connected to VCC
#define ADS_C 0x4A // ADDR Connected to SDA
#define ADS_D 0x4B // ADDR Connected to SCL

// Assign Variables
uint16_t RawValue_ADS_A; // Raw value from the ADC 16bit
uint16_t RawValue_ADS_B; // Raw value from the ADC 16bit
uint16_t RawValue_ADS_C; // Raw value from the ADC 16bit
uint16_t RawValue_ADS_D; // Raw value from the ADC 16bit
uint16_t ConvertedValue; // Contains the End result of the ADS rawdata
byte error; // Contains the error code
int HiLimit = 2.4; //3.0V this allows us to contain decimals
int LoLimit = 1; //2.5V this allows us to contain decimals

int HiOffTargetLimit = 3.75; //3.0V this allows us to contain decimals
int LoOffTargetLimit = 2.5; //3.0V this allows us to contain decimals

//PIN connections
int WeaponA_ONTARGET_ALRT_PIN = 34; // ADS1115 Ready Pin good
int WeaponB_ONTARGET_ALRT_PIN = 35; // ADS1115 Ready Pin 
int WeaponA_OFFTARGET_ALRT_PIN = 32; // ADS1115 Ready Pin
int WeaponB_OFFTARGET_ALRT_PIN = 33; // ADS1115 Ready Pin

 
// ADS1115 Register Addresses, These are pointers to the address we want to access
const short ConvReg     = 0b00000000; // Conversion Register Stores the result of the last conversion
const short CfgReg      = 0b00000001; // Config Register contains settings for how the ADC operates
const short LoThreshReg = 0b00000010; // Low Threshold Register
const short HiThreshReg = 0b00000011; // High Threshold Register
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
  const short MSBcfg  = 0b01000010; // config for single-ended Continous conversion for 4.096v Readings on A0               
//const short MSBcfg  = 0b01000010; // config for single-ended Continous conversion for 4.096v Readings on A0
//const short MSBcfg2 = 0b01010010; // config for single-ended Continous conversion for 4.096v Readings on A1
//const short MSBcfg3 = 0b01100010; // config for single-ended Continous conversion for 4.096v Readings on A2
//const short MSBcfg4 = 0b01110010; // config for single-ended Continous conversion for 4.096v Readings on A3

const short MSBcfgCOMPAR_A0_A1 = 0b00000010; // config for comparitor A0 >A1 for 4.096v Readings, Continuous Conversion
const short MSBcfgCOMPAR_A0_A3 = 0b00010010; // config for comparitor A0 >A3 for 4.096v Readings, Continuous Conversion
// const short MSBcfgCOMPAR_A1_A3 = 0b00100010; // config for comparitor A1 >A3 for 4.096v Readings, Continuous Conversion
//const short MSBcfgCOMPAR_A2_A3 = 0b00110010; // config for comparitor A2 >A3 for 4.096v Readings, Continuous Conversion
//=================================LSB===================================================
/* Least Segnificant Byte, bits 0-7
    Bits are reas from left to right after the "0b"
Bits 7:5  000=8sps, 001=16sps, 010=32sps, 011=64sps, 100=128sps, 101=250sps, 110=475sps, 111=860/sps
Bit  4    0=Traditional Comparator with hysteresis, 1=Window Comparator
Bit  3    Alert/Ready Pin 0= Goes Low when it enters active state or 1= goes High when it enters active state
Bit  2    Alert/Ready pin Latching 0=No (pulse the Pin after each conversion), 1=Yes (stays in current state until the data is read)
Bits 1:0  Number of conversions to complete before PIN is active, 00=1, 01=2, 10=4, 11=disable comparator and set to default
*/                       //bits:7:5,4,3,2,1:0
                                    // 100       0                 0                          0                                  10
//const short LSBcfg  = 0b00001010; // 128sps, Traditional Mode, High at the end of the conversion, pulse the Pin after each conversion, 4 conversions          
                      //bits:7:5,4,3,2,1:0
                                   // 011       0                  0                          0                                  00
//const short LSBcfg = 0b01100010; // 64x sps, Traditional Mode, Low when in active state, pulse the Pin after each conversion, 1 conversions
                                 // 000       1             0                           0                                 10
  const short LSBcfg = 0b11110010;// 8x sps, Window mode, Low when in active state, pulse the Pin after each conversion, 4 conversions
                                 // goes ACTIVE after the fourth conversion, so we do 4 confirmations.

volatile int16_t timeHitWasMade; // This is the time the hit was made
volatile bool ISR_StateChanged = false; // This is the flag that is set when the button is pressed
volatile bool OnTargetA_Flag   = false;
volatile bool OnTargetB_Flag   = false;
volatile bool OffTargetA_Flag  = false;
volatile bool OffTargetB_Flag  = false;
