#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>
#include <Adafruit_ADS1X15.h>
Adafruit_ADS1115 adsA;  // Create an instance of the ADS1115
Adafruit_ADS1115 adsB;  // Create an instance of the ADS1115
uint16_t weaponA = 0;
uint16_t lameA = 0;
uint16_t groundA = 0;
uint16_t weaponB = 0;
uint16_t lameB = 0;
uint16_t groundB = 0;
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
const short MSBcfg  = 0b01000010; // config for single-ended Continous conversion for 4.096v Readings on A0 
const short MSBcfg2 = 0b01010010; // config for single-ended Continous conversion for 4.096v Readings on A1
const short MSBcfg3 = 0b01100010; // config for single-ended Continous conversion for 4.096v Readings on A2
const short MSBcfg4 = 0b01110010; // config for single-ended Continous conversion for 4.096v Readings on A3
const short MSBcfgCOMPAR_A0_A1 = 0b00000010; // config for comparitor A0 >A1 for 4.096v Readings, Continuous Conversion
const short MSBcfgCOMPAR_A0_A3 = 0b00010010; // config for comparitor A0 >A3 for 4.096v Readings, Continuous Conversion
const short MSBcfgCOMPAR_A1_A3 = 0b00100010; // config for comparitor A1 >A3 for 4.096v Readings, Continuous Conversion
const short MSBcfgCOMPAR_A2_A3 = 0b00110010; // config for comparitor A2 >A3 for 4.096v Readings, Continuous Conversion
const short LSBcfg  = 0b11110000;// 3300x sps, Window mode, Low when in active state, pulse the Pin after each conversion, 1 conversions
const short LSBcfg2 = 0b11111000;// 3300x sps, Window mode, High when in active state, pulse the Pin after each conversion, 1 conversions
const int ALRT_PIN = 34; // GPIO34
const int ALRT_PIN2= 35; // GPIO35
void Foil_ADS_Configurator(){
  // SET GAIN
  // SET SPS
  // SET COMPARATOR MODE (Window with HI and LO values, Ain0 - Ain3)
  const int HI = 17600; // 1.70V Convert the voltage to a 16bit value
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
  Wire.write(MSBcfg2); // Write the MSB of the configuration register
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

  Wire.beginTransmission(ADS_B); // Start I2C Transmission to the ADS1015
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
}

// REPLACE WITH THE RECEIVER'S MAC Address
// 48:E7:29:98:54:B0
uint8_t broadcastAddress[] = {0x48, 0xE7, 0x29, 0x98, 0x54, 0xB0};

// Structure example to send data
// Must match the receiver structure
typedef struct struct_message {
    int id; // must be unique for each sender board
    int x;
    int y;
} struct_message;

// Create a struct_message called myData
struct_message myData;

// Create peer interface
esp_now_peer_info_t peerInfo;

// callback when data is sent
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  // Serial.print("\r\nLast Packet Send Status:\t");
  // Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}
 
void setup() {
  // Init Serial Monitor
  Serial.begin(115200);
  
  pinMode(ALRT_PIN, INPUT);
  pinMode(ALRT_PIN2, INPUT);
  Wire.begin();
  Foil_ADS_Configurator();
  // adsA.begin(0x48);
  // adsB.begin(0x49);
  // adsA.setGain(GAIN_ONE); // 1x gain   +/- 4.096V  1 bit = 2mV      0.125mV
  // adsB.setGain(GAIN_ONE); // 1x gain   +/- 4.096V  1 bit = 2mV      0.125mV
  // adsA.setDataRate(3300); // 3300 samples per second 
  // adsB.setDataRate(3300); // 3300 samples per second

  // Serial.println(adsA.getDataRate());
  // Serial.println(adsB.getDataRate()); 
  //====ADS_A====
  while (!Serial);  // Wait for the serial port to connect
  if (!adsA.begin(0x48)) {
    Serial.println("Failed to initialize ADS1015 at default 0x48!");
  }
  //====ADS_B====
  if (!adsB.begin(0x49)) {
    Serial.println("Failed to initialize ADS1015 at address 0x49!");
  }
  // Set device as a Wi-Fi Station
  WiFi.mode(WIFI_STA);

  // Init ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  // Once ESPNow is successfully Init, we will register for Send CB to
  // get the status of Trasnmitted packet
  esp_now_register_send_cb(OnDataSent);
  
  // Register peer
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;  
  peerInfo.encrypt = false;
  
  // Add peer        
  if (esp_now_add_peer(&peerInfo) != ESP_OK){
    Serial.println("Failed to add peer");
    return;
  }
}
 
void loop() {
  // Set values to send
  myData.id = 1;
  if (digitalRead(ALRT_PIN) == HIGH) {
    myData.x = 1;
  } else {
    myData.x = 0;
  }
  if (digitalRead(ALRT_PIN2) == HIGH) {
    myData.y = 1;
  } else {
    myData.y = 0;
  }
  // weaponA = adsA.readADC_SingleEnded(0);  // Read from channel 0 of the first ADS1115
  // lameA = adsA.readADC_SingleEnded(1);    // Read from channel 1 of the first ADS1115
  // //groundA = adsA.readADC_SingleEnded(2);  // Read from channel 2 of the first ADS1115

  // weaponB = adsB.readADC_SingleEnded(0);  // Read from channel 0 of the second ADS1115
  // lameB = adsB.readADC_SingleEnded(1);    // Read from channel 1 of the second ADS1115
  // //groundB = adsB.readADC_SingleEnded(2);  // Read from channel 2 of the second ADS1115

  //Request Data
  Wire.beginTransmission(ADS_A); // Start I2C Transmission to the ADS1115
  Wire.write(ConvReg); //point to the convergen register to read conversion data
  Wire.endTransmission(); // End the I2C Transmission
  Wire.requestFrom(ADS_A, 2); // Request 2 bytes from the ADS1115 (MSB then the LSB)
  /* Since the ADS1115 sends the MSB first we need to Read the first byte (MSB) and shift it 8 bits to the left
  then we read the second byte (LSB) and add it to the first byte. This will give us the 16bit value of the conversion
  */
  float RawValue_ADS_A = Wire.read() << 8 | Wire.read(); // Read the MSB and shift it 8 bits to the left, then read the LSB and add it to the MSB
  RawValue_ADS_A = RawValue_ADS_A * 0.000125; // Convert the 11bit value to a voltage

  Wire.beginTransmission(ADS_B);
  Wire.write(ConvReg);
  Wire.endTransmission();
  Wire.requestFrom(ADS_B, 2); //Request the LSB of the data from ADS_C
  float RawValue_ADS_B = Wire.read() << 8 | Wire.read(); // Read the MSB and shift it 8 bits to the left, then read the LSB and add it to the MSB
  RawValue_ADS_B = RawValue_ADS_B * 0.000125; // Convert the 11bit value to a voltage

  // Send message via ESP-NOW
  esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *) &myData, sizeof(myData));
   
  if (result == ESP_OK) {
    // Serial.println("Sent with success");
    // Serial.print("Value sent x : ");
    // Serial.println(myData.x);
    // Serial.print("Value sent y : ");
    // Serial.println(myData.y);
    Serial.println(RawValue_ADS_A);
    // Serial.println(RawValue_ADS_B);
  }
  else {
    Serial.println("Error sending the data");
  }
  delay(100);
}