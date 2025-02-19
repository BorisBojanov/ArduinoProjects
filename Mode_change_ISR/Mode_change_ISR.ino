

  const int MODE_BUTTON_PIN = 13; // The pin where your mode button is connected
  const int DEBOUNCE_DELAY = 50; // The debounce delay in milliseconds
  unsigned long lastDebounceTime = 0; // The last time the button state changed  
  int lastButtonState = LOW; // The last known state of the button
  int buttonState;
  volatile bool ISR_StateChanged =false;
  
// Function prototypes
void handleFoilHit();
void handleEpeeHit();
void handleSabreHit();
//==========================
// WeaponMode Changer
//==========================
struct WeaponMode {
  int lockoutTime;
  int depressTime;
  void (*handleHit)();
};

WeaponMode foilMode = {300000, 14000, handleFoilHit};
WeaponMode epeeMode = {45000, 2000, handleEpeeHit};
WeaponMode sabreMode = {120000, 1000, handleSabreHit};
WeaponMode* currentMode = &foilMode; // Default Mode

void handleFoilHit(){
    // Do something when a hit is detected in foil mode
    Serial.println("Inside Foil Function");
}

void handleEpeeHit() {
    // Do something when a hit is detected in epee mode
    Serial.println("Inside Epee function");
}

void handleSabreHit() {
    // Do something when a hit is detected in sabre mode
    Serial.println("Inside Sabre Function");
}

void handleHit() {
    currentMode->handleHit(); // will pint to a Mode instance in struct Mode
}

void modeChange() { 
  Serial.println("modeChange");
  buttonState = LOW;
  if (currentMode == &foilMode) {
    currentMode = &epeeMode;
  } else if (currentMode == &epeeMode) {
    currentMode = &sabreMode;
  } else {
    currentMode = &foilMode;
  }
}
void BUTTON_ISR() {
  ISR_StateChanged = true;
}
/*
Function is universal for any button
Takes in the Int Value of the button PIN and returns Ture or False boolean values
*/
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
        Serial.println("changed mode was called");
        return true;
      }
    }
  }
  lastButtonState = reading;
  // Return false if no change or if not meeting the conditions
  return false;
}

void setup() {
  Serial.begin(115200);
  Serial.println("Hello world");
  pinMode(MODE_BUTTON_PIN, INPUT_PULLDOWN);
  attachInterrupt(digitalPinToInterrupt(MODE_BUTTON_PIN), BUTTON_ISR, FALLING);
}

void loop() {
  
  if(BUTTON_Debounce(MODE_BUTTON_PIN)){
    modeChange();
  }
  handleHit();
}

