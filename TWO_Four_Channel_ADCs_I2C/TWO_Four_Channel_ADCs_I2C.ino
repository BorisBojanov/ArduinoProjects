#include <Wire.h>
#include <Adafruit_ADS1X15.h>

Adafruit_ADS1115 ads1115_default;  // ADS1115 with ADDR pin floating (default address 0x48)
Adafruit_ADS1115 ads1115_addr;     // ADS1115 with ADDR pin connected to GND


/*
THIS IS THE CODE FOR THE ADS1015
It works in the current state as long as you are not Serial.Printing anything and you do not plan to re program the ADS during the  void loop();


*/

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
/*
For On Target we are taking the difference of A0 and A1 and triggering if that difference is between 0 and 1V
*/
int HiLimit = 1; // Vin 3.3 R1 = 1000 R2 = 1000
int LoLimit = 0; // Vout should be 1.65V
// A0 and A1 are the same voltage so the difference is 0V


/*
For Off Target we are taking the difference of A0 and GND and returning it as a singleended value. 
if that value is between 2.5V and 3.75V then we trigger the off target event.
*/
// any Value that is greater than 2.5V and less than 3.75V is considered off target
//No lights will be triggered if the value is between 1.65V and 2.2V
//PIN connections
int WeaponA_ONTARGET_ALRT_PIN = 34; // ADS1115 Ready Pin good
int WeaponB_ONTARGET_ALRT_PIN = 35; // ADS1115 Ready Pin 
int WeaponA_OFFTARGET_ALRT_PIN = 5; // ADS1115 Ready Pin
int WeaponB_OFFTARGET_ALRT_PIN = 18; // ADS1115 Ready Pin

 
// ADS1115 Register Addresses, These are pointers to the address we want to access
const short ConvReg     = 0b00000000; // Conversion Register Stores the result of the last conversion
const short CfgReg      = 0b00000001; // Config Register contains settings for how the ADC operates
const short LoThreshReg = 0b00000010; // Low Threshold Register
const short HiThreshReg = 0b00000011; // High Threshold Register
//==============================MSB=======================================================
/* Select the Most Segnificant Byte, bits 8-15
    Bits are reas from left to right after the "0b"
Bit 15    0=no effect, 1=Begin Single Conversion (in power down mode)
Bit 14:12 000 : AINP = AIN0 and AINN = AIN1 (default)
          001 : AINP = AIN0 and AINN = AIN3
          010 : AINP = AIN1 and AINN = AIN3
          011 : AINP = AIN2 and AINN = AIN3
          100 : AINP = AIN0 and AINN = GND
          101 : AINP = AIN1 and AINN = GND
          110 : AINP = AIN2 and AINN = GND
          111 : AINP = AIN3 and AINN = GND

Bit 11:9  Programable Gain 000=6.144v, 001=4.096v, 010=2.048v, 011=1.024v, 100=0.512v, 101=0.256v
bit 8     0=Continuous Conversion, 1=Power Down Single Shot
//const short MSBcfg  = 0b01000010; // config for single-ended Continous conversion for 4.096v Readings on A0
//const short MSBcfg2 = 0b01010010; // config for single-ended Continous conversion for 4.096v Readings on A1
//const short MSBcfg3 = 0b01100010; // config for single-ended Continous conversion for 4.096v Readings on A2
//const short MSBcfg4 = 0b01110010; // config for single-ended Continous conversion for 4.096v Readings on A3
*/
                         //bits:15,14,13:12,11:9,8
const short MSBcfg             = 0b01000010; // config for single-ended Continous conversion for 4.096v Readings on A0               
const short MSBcfgCOMPAR_A0_A1 = 0b00000010; // config for comparitor A0 >A1 for 4.096v Readings, Continuous Conversion
const short MSBcfgCOMPAR_A0_A3 = 0b00010010; // config for comparitor A0 >A3 for 4.096v Readings, Continuous Conversion
// const short MSBcfgCOMPAR_A1_A3 = 0b00100010; // config for comparitor A1 >A3 for 4.096v Readings, Continuous Conversion
// const short MSBcfgCOMPAR_A2_A3 = 0b00110010; // config for comparitor A2 >A3 for 4.096v Readings, Continuous Conversion

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
  const short LSBcfg = 0b11110000;// 3300x sps, Window mode, Low when in active state, pulse the Pin after each conversion, after 1 conversions
                                 // goes ACTIVE after the fourth conversion, so we do 4 confirmations.

volatile int16_t timeHitWasMade; // This is the time the hit was made
volatile bool ISR_StateChanged = false; // This is the flag that is set when the button is pressed
volatile bool OnTargetA_Flag   = false;
volatile bool OnTargetB_Flag   = false;
volatile bool OffTargetA_Flag  = false;
volatile bool OffTargetB_Flag  = false;

// Assign each flag a unique power of 2
const int OnTargetA_Flag_Value = 2; // 2^1
const int OnTargetB_Flag_Value = 4; // 2^2
const int OffTargetA_Flag_Value = 8; // 2^3
const int OffTargetB_Flag_Value = 16; // 2^4

// Calculate the state of the flags
int flagState = (OnTargetA_Flag * OnTargetA_Flag_Value) +
                (OnTargetB_Flag * OnTargetB_Flag_Value) +
                (OffTargetA_Flag * OffTargetA_Flag_Value) +
                (OffTargetB_Flag * OffTargetB_Flag_Value);
void ISR_OnTargetA(){//This function is potentially being called very often 
    // timeHitWasMade = micros();
    OnTargetA_Flag = true;
}
void ISR_OnTargetB(){
    timeHitWasMade = micros();
    OnTargetB_Flag = true;

}
void ISR_OffTargetA(){
    // timeHitWasMade = micros();
    OffTargetA_Flag = true; //test 


}
void ISR_OffTargetB(){
    timeHitWasMade = micros();
    OffTargetB_Flag = true; //test

}

void setup(void) {
  pinMode(WeaponA_ONTARGET_ALRT_PIN, INPUT); // Set the RDY pin as an input
  pinMode(WeaponB_ONTARGET_ALRT_PIN, INPUT); // Set the RDY pin as an input
  pinMode(WeaponA_OFFTARGET_ALRT_PIN, INPUT); // Set the RDY pin as an input
  pinMode(WeaponB_OFFTARGET_ALRT_PIN, INPUT); // Set the RDY pin as an input
  // digitalWrite(WeaponA_ONTARGET_ALRT_PIN, LOW);
  // digitalWrite(WeaponB_ONTARGET_ALRT_PIN, LOW);
  
  Serial.begin(9600);
  Serial.println("Hello world");

  ads1115_default.begin(0x48);    // Initialize the first ADS1115 at default address 0x48 (ADDR is a Floating Pin)
  // ads1115_addr.begin(0x49);   // Initialize the second ADS1115 at address 0x49 (ADDR connected to VDD aka power of ADC Pin)

  //====
  while (!Serial); // Wait for the serial port to connect

  if (!ads1115_default.begin(0x48)) {
    Serial.println("Failed to initialize ADS1115 at default address!");
  }
  // if (!ads1115_addr.begin(0x49)) {
  //   Serial.println("Failed to initialize ADS1115 at address 0x49!");
  // }
  //====

  Serial.println("Getting single-ended readings from AIN0..3");
  Serial.println("ADC Range: +/- 6.144V (1 bit = 0.1875mV/ADS1115)"); 
  const int HI = 10; // 1V Convert the voltage to a 16bit value
  const int LO = 0;  // 0V Convert the voltage to a 16bit value
  const int HIOffTarget = 32760; // 4.095V 32760 Convert the voltage to a 16bit value 0.000125V per bit
  const int LOOffTarget = 26240; // 3.28V 26240 Convert the voltage to a 16bit value 0.000125V per bit
  attachInterrupt(digitalPinToInterrupt(WeaponA_ONTARGET_ALRT_PIN), ISR_OffTargetA, CHANGE);
  attachInterrupt(digitalPinToInterrupt(WeaponB_ONTARGET_ALRT_PIN), ISR_OnTargetA, CHANGE);
  attachInterrupt(digitalPinToInterrupt(WeaponA_OFFTARGET_ALRT_PIN), ISR_OffTargetA, RISING); 
  attachInterrupt(digitalPinToInterrupt(WeaponB_OFFTARGET_ALRT_PIN), ISR_OffTargetB, RISING );
  Wire.begin();
  Wire.setClock(400000); //Increase I2C clock speed to 400kHz
//======================ADS_A================================
  delay(10);
  //the following will disable the HI/LO Thresholds registers to use pin ## as the READY pin
  Wire.beginTransmission(ADS_A); // Start I2C Transmission to the ADS1115
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

  Wire.beginTransmission(ADS_A); // Start I2C Transmission to the ADS1115
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
  //This is sending the configuration to the ADS1115
  Wire.beginTransmission(ADS_A); // Start I2C Transmission to the ADS1115
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
  Wire.beginTransmission(ADS_B); // Start I2C Transmission to the ADS1115
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

  Wire.beginTransmission(ADS_B); // Start I2C Transmission to the ADS1115
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
  //This is sending the configuration to the ADS1115
  Wire.beginTransmission(ADS_B); // Start I2C Transmission to the ADS1115
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
// //======================ADS_C================================
//   delay(10);
//   //the following will disable the HI/LO Thresholds registers to use pin ## as the READY pin
//   Wire.beginTransmission(ADS_C); // Start I2C Transmission to the ADS1115
//   Wire.write(HiThreshReg); // 16bit register, so we must send the number in two parts
//   Wire.write(highByte(HIOffTarget)); //Write the MSB of the HI Threshold
//   Wire.write(lowByte(HIOffTarget)); //Write the LSB of the HI Threshold
//   // Wire.write(0b10000000); // the 1 states that this is the MSB Set the HI Threshold to 0
//   // Wire.write(0b00000000); // the 0 states that this is the LSB Set the HI Threshold to 0
//   error = Wire.endTransmission(); // End the I2C Transmission
//   if (error > 0) {
//     Serial.print("Error: ");
//     Serial.println(error);
//   } else {
//     Serial.println("Command ackowledged");
//   }

//   delay(10); //wait 10milliseconds before sending the next I2C transmission

//   Wire.beginTransmission(ADS_C); // Start I2C Transmission to the ADS1115
//   Wire.write(LoThreshReg); // 16bit register, so we must send the number in two parts
//   Wire.write(highByte(LOOffTarget)); //Write the MSB of the LO Threshold
//   Wire.write(lowByte(LOOffTarget)); //Write the LSB of the LO Threshold
//   // Wire.write(0b01111111); // the 1 states that this is the MSB 
//   // Wire.write(0b11111111); // the 0 states that this is the LSB 
//   error = Wire.endTransmission(); // End the I2C Transmission
//   if (error > 0) {
//     Serial.print("Error: ");
//     Serial.println(error);
//   } else {
//     Serial.println("Command ackowledged");
//   }

//   delay(10);
//   //This is sending the configuration to the ADS1115
//   Wire.beginTransmission(ADS_C); // Start I2C Transmission to the ADS1115
//   Wire.write(CfgReg); // write the register address to point to the configuration register
//   Wire.write(MSBcfg); // Write the MSB of the configuration register
//   Wire.write(LSBcfg); // Write the LSB of the configuration register
//   error = Wire.endTransmission(); // End the I2C Transmission
//   if (error > 0) {
//     Serial.print("Error: ");
//     Serial.println(error);
//   } else {
//     Serial.println("Configuration Set for ADS_C");
//   }
// //======================ADS_D================================
//   delay(10);
//   //the following will disable the HI/LO Thresholds registers to use pin ## as the READY pin
//   Wire.beginTransmission(ADS_D); // Start I2C Transmission to the ADS1115
//   Wire.write(HiThreshReg); // 16bit register, so we must send the number in two parts
//   Wire.write(highByte(HIOffTarget)); //Write the MSB of the HI Threshold
//   Wire.write(lowByte(HIOffTarget)); //Write the LSB of the HI Threshold
//   // Wire.write(0b10000000); // the 1 states that this is the MSB Set the HI Threshold to 0
//   // Wire.write(0b00000000); // the 0 states that this is the LSB Set the HI Threshold to 0
//   error = Wire.endTransmission(); // End the I2C Transmission
//   if (error > 0) {
//     Serial.print("Error: ");
//     Serial.println(error);
//   } else {
//     Serial.println("Command ackowledged");
//   }

//   delay(10); //wait 10milliseconds before sending the next I2C transmission

//   Wire.beginTransmission(ADS_D); // Start I2C Transmission to the ADS1115
//   Wire.write(LoThreshReg); // 16bit register, so we must send the number in two parts
//   Wire.write(highByte(LOOffTarget)); //Write the MSB of the LO Threshold
//   Wire.write(lowByte(LOOffTarget)); //Write the LSB of the LO Threshold
//   // Wire.write(0b01111111); // the 1 states that this is the MSB 
//   // Wire.write(0b11111111); // the 0 states that this is the LSB 
//   error = Wire.endTransmission(); // End the I2C Transmission
//   if (error > 0) {
//     Serial.print("Error: ");
//     Serial.println(error);
//   } else {
//     Serial.println("Command ackowledged");
//   }

//   delay(10);
//   //This is sending the configuration to the ADS1115
//   Wire.beginTransmission(ADS_D); // Start I2C Transmission to the ADS1115
//   Wire.write(CfgReg); // write the register address to point to the configuration register
//   Wire.write(MSBcfg); // Write the MSB of the configuration register
//   Wire.write(LSBcfg); // Write the LSB of the configuration register
//   error = Wire.endTransmission(); // End the I2C Transmission
//   if (error > 0) {
//     Serial.print("Error: ");
//     Serial.println(error);
//   } else {
//     Serial.println("Configuration Set for ADS_D");
//   }

}

void loop(void) {
  delay(5);

  // "do" is a keyword known to C
  // do{
  //   //Serial.println("Doing Nothing, waiting for RDY pin to go high");
  //   //Do nothing until conversion is ready
  // } while(digitalRead(RDY) == 0); //Wait for the ADS1115 to be ready
  // if RDY pin is HIGH then the ADS1115 is ready to be read
  // Until then it will be stuck in this loop
  
  //Request Data
  Wire.beginTransmission(ADS_A); // Start I2C Transmission to the ADS1115
  Wire.write(ConvReg); //point to the convergen register to read conversion data
  Wire.endTransmission(); // End the I2C Transmission
  Wire.requestFrom(ADS_A, 2); // Request 2 bytes from the ADS1115 (MSB then the LSB)
  /* Since the ADS1115 sends the MSB first we need to Read the first byte (MSB) and shift it 8 bits to the left
  then we read the second byte (LSB) and add it to the first byte. This will give us the 16bit value of the conversion
  */
  RawValue_ADS_A = Wire.read() << 4 | Wire.read(); // Read the MSB and shift it 8 bits to the left, then read the LSB and add it to the MSB
  
  Wire.beginTransmission(ADS_B);
  Wire.write(ConvReg);
  Wire.endTransmission();
  Wire.requestFrom(ADS_B, 2); //Request the LSB of the data from ADS_C
  RawValue_ADS_B = Wire.read() << 4 | Wire.read(); // Read the MSB and shift it 8 bits to the left, then read the LSB and add it to the MSB

  // Wire.beginTransmission(ADS_C);
  // Wire.write(ConvReg);
  // Wire.endTransmission();
  // Wire.requestFrom(ADS_C, 2); //Request the LSB of the data from ADS_C
  // RawValue_ADS_C = Wire.read() << 8 | Wire.read(); // Read the MSB and shift it 8 bits to the left, then read the LSB and add it to the MSB

  // Wire.beginTransmission(ADS_D);
  // Wire.write(ConvReg);
  // Wire.endTransmission();
  // Wire.requestFrom(ADS_D, 2); //Request the LSB of the data from ADS_C
  // RawValue_ADS_C = Wire.read() << 8 | Wire.read(); // Read the MSB and shift it 8 bits to the left, then read the LSB and add it to the MSB
  
  
  // Display the results
  // Serial.println("Display the results");
  OffTargetA_Flag = digitalRead(WeaponA_ONTARGET_ALRT_PIN);
  OnTargetA_Flag = digitalRead(WeaponB_ONTARGET_ALRT_PIN);
  Serial.print("RawValue_ADS_A: "); Serial.print(RawValue_ADS_A); Serial.print(" Alert/Ready pin = "); Serial.println(digitalRead(WeaponA_ONTARGET_ALRT_PIN)); //Display the voltage measured on A0 on the serial monitor
  Serial.print("RawValue_ADS_B: "); Serial.print(RawValue_ADS_B); Serial.print(" Alert/Ready pin = "); Serial.println(digitalRead(WeaponB_ONTARGET_ALRT_PIN)); //Display the voltage measured on A0 on the serial monitor
  // Serial.print("RawValue_ADS_C: "); Serial.print("Alert/Ready pin = "); Serial.println(digitalRead(WeaponA_OFFTARGET_ALRT_PIN)); //Display the voltage measured on A0 on the serial monitor
  // Serial.print("RawValue_ADS_D: "); Serial.print("Alert/Ready pin = "); Serial.println(digitalRead(WeaponB_OFFTARGET_ALRT_PIN)); //Display the voltage measured on A0 on the serial monitor
  flagState = (OnTargetA_Flag * OnTargetA_Flag_Value) +
              (OnTargetB_Flag * OnTargetB_Flag_Value) +
              (OffTargetA_Flag * OffTargetA_Flag_Value) +
              (OffTargetB_Flag * OffTargetB_Flag_Value);
  // Serial.println(flagState);
switch (flagState) {
    case OnTargetA_Flag_Value:
      // OnTargetA_Flag = false;
      Serial.print("Alert/Ready pin = "); Serial.println(digitalRead(WeaponA_ONTARGET_ALRT_PIN));
      // digitalWrite(WeaponA_ONTARGET_ALRT_PIN, LOW);
      Serial.println("OnTargetA_DETECTED!!!");
    break;
    case OnTargetB_Flag_Value:
      // OnTargetB_Flag = false;
      Serial.print("Alert/Ready pin = "); Serial.println(digitalRead(WeaponB_ONTARGET_ALRT_PIN));
      // digitalWrite(WeaponB_ONTARGET_ALRT_PIN, LOW);
      // digitalWrite(WeaponB_ONTARGET_ALRT_PIN, LOW);
      Serial.println("OnTargetB_DETECTED!!!");
    break;
    case OffTargetA_Flag_Value:
      // OffTargetA_Flag = false;
      Serial.print("Alert/Ready pin = "); Serial.println(digitalRead(WeaponA_OFFTARGET_ALRT_PIN));
      // digitalWrite(WeaponA_ONTARGET_ALRT_PIN, LOW);
      Serial.println("OffTargetA_DETECTED!!!");
    break;
    case OffTargetB_Flag_Value:
      // OffTargetB_Flag = false;
      Serial.print("Alert/Ready pin = "); Serial.println(digitalRead(WeaponB_OFFTARGET_ALRT_PIN));
      // digitalWrite(WeaponA_ONTARGET_ALRT_PIN, LOW);
      Serial.println("OffTargetB_DETECTED!!!");
    break;
    case OnTargetA_Flag_Value+OnTargetB_Flag_Value:
      // OnTargetA_Flag = false;
      // OnTargetB_Flag = false;
      Serial.print("Alert/Ready pin = "); Serial.println(digitalRead(WeaponA_ONTARGET_ALRT_PIN));
      Serial.print("Alert/Ready pin = "); Serial.println(digitalRead(WeaponB_ONTARGET_ALRT_PIN));
      // digitalWrite(WeaponA_ONTARGET_ALRT_PIN, LOW);
      // digitalWrite(WeaponB_ONTARGET_ALRT_PIN, LOW);
      Serial.println("OnTargetA_DETECTED AND OnTargetB_DETECTED!!!");

    break;
    case OnTargetA_Flag_Value+OffTargetA_Flag_Value:
      // OnTargetA_Flag = false;
      // OffTargetA_Flag = false;
      Serial.println("OnTargetA_DETECTED AND OffTargetA_DETECTED!!!");
      Serial.print("Alert/Ready pin = "); Serial.println(digitalRead(WeaponA_ONTARGET_ALRT_PIN));
      Serial.print("Alert/Ready pin = "); Serial.println(digitalRead(WeaponA_OFFTARGET_ALRT_PIN));
    break;
    case OnTargetA_Flag_Value+OffTargetB_Flag_Value:
      // OnTargetB_Flag = false;
      // OffTargetB_Flag = false;
      // digitalWrite(WeaponA_ONTARGET_ALRT_PIN, LOW);
      // digitalWrite(WeaponB_ONTARGET_ALRT_PIN, LOW);
      Serial.println("OnTargetA_DETECTED AND OffTargetB_DETECTED!!!");
      Serial.print("Alert/Ready pin = "); Serial.println(digitalRead(WeaponA_ONTARGET_ALRT_PIN));
      Serial.print("Alert/Ready pin = "); Serial.println(digitalRead(WeaponB_OFFTARGET_ALRT_PIN));
    break;
    case OnTargetB_Flag_Value+OffTargetA_Flag_Value:
      // OnTargetB_Flag = false;
      // OffTargetA_Flag = false;
      // digitalWrite(WeaponA_ONTARGET_ALRT_PIN, LOW);
      // digitalWrite(WeaponB_ONTARGET_ALRT_PIN, LOW);
      Serial.println("OnTargetB_DETECTED AND OffTargetA_DETECTED!!!");
      Serial.print("Alert/Ready pin = "); Serial.println(digitalRead(WeaponB_ONTARGET_ALRT_PIN));
      Serial.print("Alert/Ready pin = "); Serial.println(digitalRead(WeaponA_OFFTARGET_ALRT_PIN));
    break;
    case OnTargetB_Flag_Value+OffTargetB_Flag_Value:
      // OnTargetB_Flag = false;
      // OffTargetB_Flag = false;
      // digitalWrite(WeaponA_ONTARGET_ALRT_PIN, LOW);
      // digitalWrite(WeaponB_ONTARGET_ALRT_PIN, LOW);
      Serial.println("OnTargetB_DETECTED AND OffTargetB_DETECTED!!!");
      Serial.print("Alert/Ready pin = "); Serial.println(digitalRead(WeaponB_ONTARGET_ALRT_PIN));
      Serial.print("Alert/Ready pin = "); Serial.println(digitalRead(WeaponB_OFFTARGET_ALRT_PIN));
    break;
    case 0:
      // Serial.println("All Flags Clear!!!");
    break;
    default:
      // Serial.println("All Flags Clear!!!");
    break;
    }

  OnTargetA_Flag = false;
  OnTargetB_Flag = false;
  OffTargetA_Flag = false;
  OffTargetB_Flag = false;
// Display the results
// Serial.print("Raw Data:"); Serial.print(RawValue); Serial.print(" x 0.0001865uV =");
// Serial.print(ConvertedValue, 3);/*3 decimal places*/ Serial.println("Volts"); //Display the voltage measured on A0

}






