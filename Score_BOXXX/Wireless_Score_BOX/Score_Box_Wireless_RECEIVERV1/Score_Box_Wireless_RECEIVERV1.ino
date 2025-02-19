/*
Goals
This is the Reciever Scetch
It will recieve a value that is Boolean or an int16_t (to be determined)
Need a way to Pair two ESP32s as slvaes
It will then make the LEDs turn on. 
It will also check for the lock out time.
    Measuring time between signals recieved
It will also check for the reset time.
    This will check if enough time has passed after a light being turned on and then turn them off

*/

// Include Libraries

#include <esp_now.h>
#include <WiFi.h>
#include <FastLED.h>

#define DATA_PIN  32   // input pin Neopixel is attached to
#define DATA_PIN2 33  // input for the seconf Neopixel LED
//=======================
// Fast LED Setup
//=======================
#define NUMPIXELS 64  // number of neopixels in strip
#define CHIPSET WS2812B
#define COLOR_ORDER GRB
#define BRIGHTNESS 20
struct CRGB pixels[NUMPIXELS];
struct CRGB pixels222[NUMPIXELS];
//=======================
// Structure example to receive data
// Must match the sender structure
typedef struct struct_message {
  int id;
  int x;
  int y;
}struct_message;

// Create a struct_message called myData
struct_message myData;

// Create a structure to hold the readings from each board
struct_message board1;
struct_message board2;
struct_message board3;

// Create an array with all the structures
struct_message boardsStruct[3] = {board1, board2, board3};
//==========================
// states
//==========================
bool Hit_On_Target_A = false;
bool Hit_Off_Target_A = false;
bool Hit_On_Target_B = false;
bool Hit_Off_Target_B = false;
// Assign each flag a unique power of 2
const int OnTargetA_Flag_Value = 2;    // 2^1
const int OnTargetB_Flag_Value = 4;    // 2^2
const int OffTargetA_Flag_Value = 8;   // 2^3
const int OffTargetB_Flag_Value = 16;  // 2^4

short int number = (Hit_On_Target_A * OnTargetA_Flag_Value)
                + (Hit_On_Target_B * OnTargetB_Flag_Value) 
                + (Hit_Off_Target_A * OffTargetA_Flag_Value) 
                + (Hit_Off_Target_B * OffTargetB_Flag_Value);

// callback function that will be executed when data is received
void OnDataRecv(const uint8_t * mac_addr, const uint8_t *incomingData, int len) {
  char macStr[18];
  Serial.print("Packet received from: ");
  snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
           mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
  Serial.println(macStr);
  memcpy(&myData, incomingData, sizeof(myData));
  Serial.printf("Board ID %u: %u bytes\n", myData.id, len);
  // Update the structures with the new incoming data
  boardsStruct[myData.id-1].x = myData.x;
  boardsStruct[myData.id-1].y = myData.y;
  Serial.printf("x value: %d \n", boardsStruct[myData.id-1].x);
  Serial.printf("y value: %d \n", boardsStruct[myData.id-1].y);
  Serial.println();
}
 
void setup() {
  //Initialize Serial Monitor
  Serial.begin(115200);
  //FAST LED SETUP
  FastLED.addLeds<CHIPSET, DATA_PIN, COLOR_ORDER>(pixels222, NUMPIXELS);
  FastLED.addLeds<CHIPSET, DATA_PIN2, COLOR_ORDER>(pixels, NUMPIXELS);
  FastLED.setBrightness(BRIGHTNESS);
  //Set device as a Wi-Fi Station
  WiFi.mode(WIFI_STA);

  //Init ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  
  // Once ESPNow is successfully Init, we will register for recv CB to
  // get recv packer info
  esp_now_register_recv_cb(OnDataRecv);
}
 
void loop() {
  // Acess the variables for each board
  /*
  int Hit_On_Target_A = boardsStruct[0].x;
  int Hit_Off_Target_A = boardsStruct[0].y;
  int Hit_On_Target_B = boardsStruct[1].x;
  int Hit_Off_Target_B = boardsStruct[1].y;
  int board3X = boardsStruct[2].x;
  int board3Y = boardsStruct[2].y;
  */
  int Hit_On_Target_A = boardsStruct[0].x;
  int Hit_Off_Target_A = boardsStruct[0].y;
  int Hit_On_Target_B = boardsStruct[1].x;
  int Hit_Off_Target_B = boardsStruct[1].y;

  
  number = (Hit_On_Target_A * OnTargetA_Flag_Value) 
        + (Hit_On_Target_B * OnTargetB_Flag_Value) 
        + (Hit_Off_Target_A * OffTargetA_Flag_Value) 
        + (Hit_Off_Target_B * OffTargetB_Flag_Value);

  switch (number) {
  case OnTargetA_Flag_Value:
    fill_solid(pixels222, NUMPIXELS, CRGB::Green);
    FastLED.show();   
    break;
  case OnTargetB_Flag_Value:
    fill_solid(pixels, NUMPIXELS, CRGB::Red);
    FastLED.show(); 
    break;
  case OffTargetA_Flag_Value:
    fill_solid(pixels222, NUMPIXELS, CRGB::White);
    FastLED.show();
    break;
  case OffTargetB_Flag_Value:
    fill_solid(pixels, NUMPIXELS, CRGB::White);
    FastLED.show();
    break;
  case OnTargetA_Flag_Value + OnTargetB_Flag_Value:
    fill_solid(pixels222, NUMPIXELS, CRGB::Green);
    fill_solid(pixels, NUMPIXELS, CRGB::Red);
    FastLED.show();
    break;
  case OnTargetA_Flag_Value + OffTargetA_Flag_Value:
    fill_solid(pixels222, NUMPIXELS, CRGB::Green);
    FastLED.show();
    fill_solid(pixels222, NUMPIXELS, CRGB::White);
    FastLED.show();
    break;
  case OnTargetA_Flag_Value + OffTargetB_Flag_Value:
    fill_solid(pixels222, NUMPIXELS, CRGB::Green);
    fill_solid(pixels, NUMPIXELS, CRGB::White);
    FastLED.show();
    break;
  case OnTargetB_Flag_Value + OffTargetA_Flag_Value:
    fill_solid(pixels, NUMPIXELS, CRGB::Red);
    fill_solid(pixels222, NUMPIXELS, CRGB::White);
    FastLED.show();
    break;
  case OnTargetB_Flag_Value + OffTargetB_Flag_Value:
    fill_solid(pixels, NUMPIXELS, CRGB::Red);
    FastLED.show();
    fill_solid(pixels, NUMPIXELS, CRGB::White);
    FastLED.show();
    break;
  default:
    fill_solid(pixels222, NUMPIXELS, CRGB::Black);
    fill_solid(pixels, NUMPIXELS, CRGB::Black);
    FastLED.show();
    
    break;
  }
  delay(100);  
}



