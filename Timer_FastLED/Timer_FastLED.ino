/*
Tasks
make a timer that counts down from 3:00 to 0
display the timer using FastLED
make a button that starts the timer
the same button will stop the timer if the timer is running
make a button that resets the timer to 3:00
*/
#include <FastLED.h>
#define NUM_LEDS 64
#define DATA_PIN 27
#define DATA_PIN2 26
#define CHIPSET WS2812B
#define COLOR_ORDER GRB
#define BRIGHTNESS 20
CRGB leds[NUM_LEDS];
CRGB leds2[NUM_LEDS];
int buttonPin = 5;
bool button_State = false;
bool lastButton_State = false;
int buttonPushCounter = 0;
bool Timer_Running = false;
unsigned long startTime;
unsigned long currentTime;
unsigned long countdownTime = 180000; // 3 minutes in milliseconds
unsigned long remainingTime;

void setup() {
  Serial.begin(115200);
  FastLED.addLeds<CHIPSET, DATA_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  FastLED.addLeds<CHIPSET, DATA_PIN2, COLOR_ORDER>(leds2, NUM_LEDS);
  FastLED.setBrightness(BRIGHTNESS);
  pinMode(buttonPin, INPUT_PULLUP);
}

void loop() {
  bool buttonState = digitalRead(buttonPin);
  
  if (buttonState != lastButton_State) {
    if (buttonState == HIGH) {
      if (!Timer_Running) {
        startTime = millis();
        Timer_Running = true;
      } else {
        Timer_Running = false;
      }
    }
    lastButton_State = buttonState;
  }

  if (Timer_Running) {
    currentTime = millis();
    unsigned long elapsedTime = currentTime - startTime;
    remainingTime = countdownTime - elapsedTime;
    
    if (remainingTime <= 0) {
      Timer_Running = false;
    }

    // Display the remaining time using FastLED
    int remainingSeconds = remainingTime / 1000;
    int minutes = remainingSeconds / 60;
    int seconds = remainingSeconds % 60;

    // Limit the seconds to just one digit
    seconds = seconds % 10;
    displayNumberOnMatrix(leds, minutes);
    displayNumberOnMatrix(leds2, seconds);
    Serial.println(minutes);
    Serial.println(seconds);
    FastLED.show();
  }
}
// Helper function to display a number on the LED matrix
uint8_t numberPatterns[10][8] = {
  {0b00000000, 0b00011000, 0b00100100, 0b00100100, 0b00100100, 0b00100100, 0b00011000, 0b00000000}, // 0 
  {0b00000000, 0b00001000, 0b00011000, 0b00101000, 0b00010000, 0b00001000, 0b00111100, 0b00000000}, // 1 
  {0b00000000, 0b00011000, 0b00100100, 0b00000100, 0b00010000, 0b00010000, 0b00111100, 0b00000000}, // 2 
  {0b00000000, 0b00011000, 0b00100100, 0b00001100, 0b00110000, 0b00100100, 0b00011000, 0b00000000}, // 3 
  {0b00000000, 0b00100100, 0b00100100, 0b00111100, 0b00100000, 0b00000100, 0b00100000, 0b00000000}, // 4 
  {0b00000000, 0b00111100, 0b00000100, 0b00111000, 0b00100000, 0b00000100, 0b00011100, 0b00000000}, // 5 
  {0b00000000, 0b00011000, 0b00100100, 0b00100000, 0b00011100, 0b00100100, 0b00011000, 0b00000000}, // 6 
  {0b00000000, 0b00111110, 0b01000000, 0b00000100, 0b00010000, 0b00010000, 0b00000100, 0b00000000}, // 7 
  {0b00000000, 0b00111100, 0b00100100, 0b00111100, 0b00100100, 0b00100100, 0b00111100, 0b00000000}, // 8 
  {0b00000000, 0b00011000, 0b00100100, 0b00011100, 0b00100000, 0b00000100, 0b00011000, 0b00000000}  // 9 
};

void displayNumberOnMatrix(CRGB* matrix, int number) {
  // Clear the matrix
  for(int i = 0; i < 64; i++) {
    matrix[i] = CRGB::Black;
  }
  // Display the number on the matrix
  for(int row = 0; row < 8; row++) {
    for(int col = 0; col < 8; col++) {
      if(bitRead(numberPatterns[number][row], 7 - col)) {
        matrix[row * 8 + col] = CRGB::Green;
    }
  }
}
}