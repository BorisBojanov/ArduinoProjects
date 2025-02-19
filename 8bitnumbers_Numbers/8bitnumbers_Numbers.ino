#include <FastLED.h>
#define NUM_LEDS 64 // 64 LEDs per matrix, 2 matrices
#define DATA_PIN 27
#define DATA_PIN2 26
#define CHIPSET WS2812B
#define COLOR_ORDER GRB
#define BRIGHTNESS 20
CRGB leds[NUM_LEDS];
CRGB leds2[NUM_LEDS];


void setup() {
  Serial.begin(115200);
  FastLED.addLeds<CHIPSET, DATA_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  FastLED.addLeds<CHIPSET, DATA_PIN, COLOR_ORDER>(leds2, NUM_LEDS);
  FastLED.setBrightness(BRIGHTNESS);

}

void loop() {
  int minutes = 3;
  int seconds = 4;
  // put your main code here, to run repeatedly:
  displayNumberOnMatrix(leds, minutes);
  displayNumberOnMatrix(leds2, seconds);
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
  fill_solid(leds, NUM_LEDS, CRGB::Black);
  // Display the number on the matrix
  for(int row = 0; row < 8; row++) {
    for(int col = 0; col < 8; col++) {
      int ledIndex = row * 8 + col;
      if(bitRead(numberPatterns[number][row], 7 - col)) {
        matrix[ledIndex] = CRGB::Green;
    }
  }
}
FastLED.show(); 
}