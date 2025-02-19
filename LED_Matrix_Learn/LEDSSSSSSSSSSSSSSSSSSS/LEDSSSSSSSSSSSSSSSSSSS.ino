#include <FastLED.h>
#include <Arduino.h>
// Define the number of LEDs and the data pin connected to the LED matrix
#define NUM_LEDS 60
#define DATA_PIN 16
#define DATA_PIN_2 18
#define CHIPSET WS2812B  // or other chipset if you're using a different type
#define COLOR_ORDER GRB
#define BRIGHTNESS 20

#define BUTTON_PIN 21
#define BUTTON_TWO_PIN 22

bool buttonState = LOW;          // variable for reading the pushbutton status
bool ledStatus = LOW;            // keep track of the LED status

bool buttonTWOState = LOW;
bool ledTWOStatus = LOW;

unsigned long lastButtonPressTime2 = 0;
unsigned long lastButtonPressTime = 0;
const unsigned long buttonPressDuration = 2000; // 3 seconds


CRGB leds[NUM_LEDS] = {0};
CRGB leds_TWO[NUM_LEDS] = {0};

void setup() {
  Serial.begin(115200);
  // pinMode(DATA_PIN, OUTPUT); //MIGHT BE REDUNDANT
  FastLED.addLeds<CHIPSET, DATA_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  FastLED.addLeds<CHIPSET, DATA_PIN_2, COLOR_ORDER>(leds_TWO, NUM_LEDS);
  FastLED.setBrightness(BRIGHTNESS);

//______BUTTONS____
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(BUTTON_TWO_PIN, INPUT_PULLUP);
}

void loop() {
  // Read the state of the button:
  buttonState = digitalRead(BUTTON_PIN);
  buttonTWOState = digitalRead(BUTTON_TWO_PIN);

  //_____Check Button 2____
  if (buttonTWOState == LOW) {
    // Record the time when the button is pressed
    lastButtonPressTime2 = millis();
    Serial.println("Turning GREEEEN LEDS ONNNN");
    fillWithColor(leds_TWO, 'Green');
    FastLED.show();
  }

  if (buttonState == LOW){
    // Record the time when the button is pressed
    lastButtonPressTime = millis();
    Serial.println("Turning RED LEDS ONNNN");
    fillWithColor(leds, 'Red');
    FastLED.show();
  }

  // Check if enough time has passed since the button was last pressed
  if (( millis() - lastButtonPressTime2 <= buttonPressDuration) ){
    // If yes, keep LEDs on
    fillWithColor(leds_TWO, 'Green');
    FastLED.show();
  } else {
    // Otherwise, turn off LEDs
    turnOffLEDs(leds_TWO, 'Green');
  }

  // Check if enough time has passed since the Left Side button was last pressed
  if ((millis() - lastButtonPressTime <= buttonPressDuration) ){
    // If yes, keep LEDs on
    fillWithColor(leds, 'Red');
    FastLED.show();
  } else {
    // Otherwise, turn off LEDs
    turnOffLEDs(leds, 'Red');
  }

   delay(50);  // Debouncing delay
}



//_______FILLS les_TWO GREEN___
//_______FILLS leds RED________
void fillWithColor(CRGB *ledArray,CRGB color) {
  
  if (color == 'Green'){
    fill_solid(leds_TWO, NUM_LEDS,CRGB::Green );
  }

  if (color == 'Red'){
    fill_solid(leds, NUM_LEDS, CRGB::Red);
  }

  if (color == 'Yellow' && ledArray == 'leds') {
    fill_solid(leds, NUM_LEDS, CRGB::Yellow)
  }
  //fill_solid(ledArray, NUM_LEDS, color);
}


void turnOffLEDs(CRGB *ledArray,CRGB color){
  Serial.println("turning LEDs OFF");
  if( color == 'Green'){
    fill_solid(leds_TWO, NUM_LEDS, CRGB:: Black);
  }

  if(color == 'Red') {
    fill_solid(leds, NUM_LEDS, CRGB:: Black);  
  }

  if(ledArray = 'leds') {
    fill_solid(leds, NUM_LEDS, CRGB::Black);
  }
  if(ledArray = 'leds_TWO') {
    fill_solid(leds, NUM_LEDS, CRGB::Black);
  }

  FastLED.show();
}

