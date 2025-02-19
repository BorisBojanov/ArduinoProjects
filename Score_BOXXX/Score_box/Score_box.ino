/*TODO
- make sure that the analog value is converted to a known range of didgets
    - Copy this from tester V2 
    - ADC Specs 
    - 12-bit ADC, meaning the values range from 0-4095
    - google ADC Specs of EPS32 Devkit V4
- Find a way to make TurOnLEDs() do the same thing as signal_hits()
- Introduce the idea of making the whole thing a CLASS called something like "Fencing_Match"

*/
//DEFFFINE THINGSSS
//____________FAST LED SPECIFIC____________
  #include <FastLED.h>
  #include <Arduino.h>
  #define VOLTS 5
  #define MAX_APMS 500 //mili amps
  #define NUM_LEDS 60
  #define NUM_LEDS_2 60
  const int DATA_PIN   = 16; //33;// 0  DO NOT ASSIGN THESE
  const int DATA_PIN_2 = 18; //32;// 1 DO NOT ASSIGN THESE
  #define CHIPSET WS2812B
  #define COLOR_ORDER GRB
  #define BRIGHTNESS 30
//_________FAST LEDS CONSTS___________
  CRGB leds[NUM_LEDS]    = {0};
  CRGB leds2[NUM_LEDS_2] = {0};

  #define BUTTON_PIN 21
  #define BUTTON_TWO_PIN 22
  bool buttonState = LOW;          // variable for reading the pushbutton status
  bool ledStatus = LOW;            // keep track of the LED status

  bool buttonTWOState = LOW;       // variable for reading the pushbutton status
  bool ledTWOStatus = LOW;         // keep track of the LED status

//_____________________________
  unsigned long lastButtonPressTime  = 0;
  unsigned long lastButtonPressTime2 = 0;
  const unsigned long ledDuration = 2000;


  //___________SCORE BOX SPECIFIC____________
  //___________A Side_______
 const int SHORT_LED_A  = 9;  //DOES NOT NEED A PIN
 const int ON_TARGET_A  = 10; //DOES NOT NEED A PIN
 const int OFF_TARGET_A = 11; //DOES NOT NEED A PIN
  //___________B Side_______
 const int OFF_TARGET_B = 12; //DOES NOT NEED A PIN
 const int ON_TARGET_B  = 13; //DOES NOT NEED A PIN
 const int SHORT_LED_B  = 14; //DOES NOT NEED A PIN
 //_________Other PINS_______
 const int MODE_PIN    = 2; //Input 
 const int BUZZER_PIN  = 3; //Output
 const int MODE_LEDS[] ={4, 5, 6};//DOES NOT NEED A PIN
  //___________Pin Set up for score box____________
  //___________All are INPUT
 const int GROUND_PIN_A = 15; // == D15 //NEEDS A PIN 
 const int WEAPON_PIN_A = 16; // == D16 //NEEDS A PIN
 const int LAME_PIN_A   = 17; // == D17 //NEEDS A PIN
 const int LAME_PIN_B   = 18; // == D18 //NEEDS A PIN
 const int WEAPON_PIN_B = 19; // == D19 //NEEDS A PIN
 const int GROUND_PIN_B = 21; // == D21 //NEEDS A PIN

  //___________Defaul analog values_____________
  // Values of analog reads
 int weaponAValue = 0;
 int weaponBValue = 0;
 int lameAValue   = 0;
 int lameBValue   = 0;
 int groundAValue = 0;
 int groundBValue = 0;

// Depress and timeouts
 unsigned long depressAtime = 0;
 unsigned long depressBtime = 0;
 bool lockedOut   = false;

// Lockout & Depress Times
 const int LOCKOUT_TIME[] = {300000, 45000, 170000}; //Lockout time between hits [foil, epee, sabre] 
 const int DEPRESS_TIME[] = {14000, 2000, 1000};    //Minimum depress time for each mode

// Global variables
bool Left  = false;
bool Right = false;
bool hitOnTargetA  = false;
bool hitOffTargetA = false;
bool hitOnTargetB  = false;
bool hitOffTargetB = false;
bool depressedA    = false;
bool depressedB    = false;

/*
# Mode constants
*/
#define FOIL_MODE  0
#define EPEE_MODE  1
#define SABRE_MODE 2

int currentMode = FOIL_MODE;
bool modeJustChangedFlag = false;


//____________________________


void setup() {
    // Initialize serial communication
    Serial.begin(115200);

    FastLED.addLeds<CHIPSET, DATA_PIN, COLOR_ORDER>(leds, NUM_LEDS);
    FastLED.addLeds<CHIPSET, DATA_PIN_2, COLOR_ORDER>(leds2, NUM_LEDS_2);
    FastLED.setMaxPowerInVoltsAndMilliamps(VOLTS, MAX_APMS);
    FastLED.setBrightness(BRIGHTNESS);
    
        // Pin Modes
    // pinMode(MODE_LEDS[0], OUTPUT); //THIS CAUSES PROBLEMSSS
    // pinMode(MODE_LEDS[1], OUTPUT); //THIS CAUSES PROBLEMSSS
    // pinMode(MODE_LEDS[2], OUTPUT); //THIS CAUSES PROBLEMSSS
    

    pinMode(WEAPON_PIN_A, INPUT);
    pinMode(GROUND_PIN_A, INPUT);
    pinMode(LAME_PIN_A  , INPUT);
    pinMode(LAME_PIN_B  , INPUT);
    pinMode(GROUND_PIN_B, INPUT);
    pinMode(WEAPON_PIN_B, INPUT);
    // Initial states
    // digitalWrite(SHORT_LED_A,  LOW);
    // digitalWrite(ON_TARGET_A,  LOW);
    // digitalWrite(OFF_TARGET_A, LOW);
    // digitalWrite(SHORT_LED_B,  LOW); 
    // digitalWrite(ON_TARGET_B,  LOW);
    // digitalWrite(OFF_TARGET_B, LOW);



    Serial.println("3 Weapon Scoring Box By Boris and Connor");
    Serial.println("====================");
    Serial.print("Mode: ");
    //Serial.println(currentMode);

    //reset_values(); // how do we reset the values
}


void loop() {
    // The main code goes here
    check_if_mode_changed();
    read_analog_values();

    //____TEST_____
    //hitOffTargetA = digitalRead(BUTTON_PIN); // This is done by the analog Read function
    //bool hitOnTargetA = true;
    //_____________
    
    Serial.println("WOWOWOWO");
    
    if (currentMode == FOIL_MODE) {
        foil();
    } else if (currentMode == EPEE_MODE) {
        epee();
    } else if (currentMode == SABRE_MODE) {
        sabre();
    }

    signal_hits(); //this should be after the logic from the analog read has been turned into Bool values that we can use to signal LEDs


    delay(10); 
}

// Add other functions to score box
// ...
//_______MODE RELATED_______
void check_if_mode_changed(){
  // check if mode changed
  if (modeJustChangedFlag) {
    if(digitalRead(MODE_PIN)) {
      if (currentMode == SABRE_MODE) {
        currentMode = FOIL_MODE;
      } else {
        currentMode += 1;
      }
    }
    set_mode_leds();
    Serial.print("Mode changed to:");
    Serial.println(currentMode);
    modeJustChangedFlag = false;

  }
}

void change_mode() {
    modeJustChangedFlag = true;
}

void set_mode_leds() {
  for (int i = 0; i < sizeof(MODE_LEDS) / sizeof(MODE_LEDS[0]); i++) {
    digitalWrite(MODE_LEDS[i], (i == currentMode) ? HIGH : LOW);
  }
  delay(500);
}

// READ ANALOG VALUES
void read_analog_values() {
    weaponAValue = analogRead(WEAPON_PIN_A);
    weaponBValue = analogRead(WEAPON_PIN_B);
    lameAValue   = analogRead(LAME_PIN_A);
    lameBValue   = analogRead(LAME_PIN_B);
    groundAValue = analogRead(GROUND_PIN_A);
    groundBValue = analogRead(GROUND_PIN_B);
    hitOffTargetA = digitalRead(BUTTON_PIN); // TEST
}

//_______WEAPON RELATED_____
void foil() {
  unsigned long now = millis(); // Arduino uses millis() to get the number of milliseconds since the board started running.
                                //It's similar to the Python monotonic_ns() function but gives time in ms not ns.
  //_____Check for lockout___
    if (((hitOnTargetA || hitOffTargetA) && (depressAtime + LOCKOUT_TIME[FOIL_MODE] < now)) ||
            ((hitOnTargetB || hitOffTargetB) && (depressBtime + LOCKOUT_TIME[FOIL_MODE] < now))) {
        lockedOut = true;
    }

    // ___Weapon A___
    if (!hitOnTargetA && !hitOffTargetA) {
        // Off target
        if (weaponAValue > 900 && lameBValue < 100) {
            if (!depressedA) {
                depressAtime = now;
                depressedA = true;
            } else if (depressAtime + DEPRESS_TIME[FOIL_MODE] <= now) {
                hitOffTargetA = true;
                Left = true;


            }
        } else {
            // On target
            if (weaponAValue > 400 && weaponAValue < 600 && lameBValue > 400 && lameBValue < 600) {
                if (!depressedA) {
                    depressAtime = now;
                    depressedA = true;
                } else if (depressAtime + DEPRESS_TIME[FOIL_MODE] <= now) {
                    hitOnTargetA = true;
                }
            } else {
                depressAtime = 0;
                depressedA = false;
            }
        }
    }

    // ___Weapon B___
    if (!hitOnTargetB && !hitOffTargetB) {
        // Off target
        if (weaponBValue > 900 && lameAValue < 100) {
            if (!depressedB) {
                depressBtime = now;
                depressedB = true;
            } else if (depressBtime + DEPRESS_TIME[FOIL_MODE] <= now) {
                hitOffTargetB = true;
                Left = true;
            }
        } else {
            // On target
            if (weaponBValue > 400 && weaponBValue < 600 && lameAValue > 400 && lameAValue < 600) {
                if (!depressedB) {
                    depressBtime = now;
                    depressedB = true;
                } else if (depressBtime + DEPRESS_TIME[FOIL_MODE] <= now) {
                    hitOnTargetB = true;
                }
            } else {
                depressBtime = 0;
                depressedB = false;
            }
        }
    }
}

void epee() {
  unsigned long now = millis();

  if (( hitOnTargetA && (depressAtime + LOCKOUT_TIME[EPEE_MODE] < now)) || (hitOnTargetB && (depressBtime + LOCKOUT_TIME[EPEE_MODE] < now))) {
    lockedOut = true;
  }

  // Weapon A
  if (!hitOnTargetA) {
    if (weaponAValue > 400 && weaponAValue < 600 && lameAValue > 400 && lameAValue < 600) {
      if (!depressedA) {
        depressAtime = now;
        depressedA = true;
      } else if (depressAtime + DEPRESS_TIME[EPEE_MODE]  <= now) {
          hitOffTargetA = true;
    
      }

    } else {
        depressAtime = 0;
        depressedA = false;
    }
  }

  //Weapon B
  if(!hitOffTargetB) {
    if(weaponBValue > 400 && weaponBValue < 600 && lameBValue > 400 && lameBValue < 600 ) {
      if(!depressedB) {
        depressBtime = now;
        depressedB = true;
      } else if ( depressBtime + DEPRESS_TIME[EPEE_MODE] <= now){
        hitOffTargetB = true;
      }
    
    } else {
      depressBtime = 0;
      depressedB = false;
    }
  }
}

void sabre() {
  unsigned long now = millis();

  if (((hitOnTargetA || hitOffTargetA) && (depressAtime + LOCKOUT_TIME[SABRE_MODE] < now)) ||
          ((hitOnTargetB || hitOffTargetB) && (depressBtime + LOCKOUT_TIME[SABRE_MODE] < now))) {
      lockedOut = true;
  }

  // Weapon A
  if (!hitOnTargetA && !hitOffTargetA) {
      if (weaponAValue > 400 && weaponAValue < 600 && lameBValue > 400 && lameBValue < 600) {
          if (!depressedA) {
              depressAtime = now;
              depressedA = true;
          } else if (depressAtime + DEPRESS_TIME[SABRE_MODE] <= now) {
              hitOnTargetA = true;
          }
      } else {
          depressAtime = 0;
          depressedA = false;
      }
  }

  // Weapon B
  if (!hitOnTargetB && !hitOffTargetB) {
      if (weaponBValue > 400 && weaponBValue < 600 && lameAValue > 400 && lameAValue < 600) {
          if (!depressedB) {
              depressBtime = now;
              depressedB = true;
          } else if (depressBtime + DEPRESS_TIME[SABRE_MODE] <= now) {
              hitOnTargetB = true;
          }
      } else {
          depressBtime = 0;
          depressedB = false;
      }
  }
}


//_______LEDS____________
void signal_hits() {
    // Logic for signaling hits
    //________REDDDDDDD____________
  if ((hitOffTargetA == true) || (hitOnTargetA == true)) {
    lastButtonPressTime = millis();
    Serial.println("Turning RED LEDS ONNNN");
    turnOnLEDs();
  }
    // Check if enough time has passed since the Right Side button was last pressed
  if ( millis() - lastButtonPressTime <= ledDuration) {
    //if yes keep the LEDs on
    turnOnLEDs();
  }else {
    turnOffLEDs('Red');
    }

  //________GREEEEEEEEENNN________
  if ((hitOffTargetB == true) || (hitOnTargetB == true)) {
    lastButtonPressTime2 = millis();
    Serial.println("Turning GREEEEN LEDS ONNNN");
    turnOnLEDs();
  }

  // Check if enough time has passed since the button was last pressed
  if ( millis() - lastButtonPressTime2 <= ledDuration) {
    //if yes keep the LEDs on
    turnOnLEDs();    
  }else {
    turnOffLEDs('Green');
    }


}

// Take the hit XXXXTargetX and call the appropriate fillWithColor(XXXXX, XXX)
void turnOnLEDs() {
  Serial.println("Entered turnOnLEDs function.");
  Serial.println(hitOffTargetA);
  
  if (hitOffTargetA) {
    Serial.println("Turning leds Yellow due to hitOffTargetA");
    fill_solid(leds, NUM_LEDS, CRGB:: Yellow);
  } else if (hitOnTargetA) {
    Serial.println("Turning leds Red due to hitOnTargetA");
    fill_solid(leds, NUM_LEDS, CRGB::Red);
  }

  if (hitOffTargetB) {
    Serial.println("Turning leds2 Blue due to hitOffTargetB");
    fill_solid(leds2, NUM_LEDS_2, CRGB:: Yellow);
  }

  if (hitOnTargetB) {
    Serial.println("Turning leds2 Green due to hitOnTargetB");
    fill_solid(leds2, NUM_LEDS_2, CRGB::Green);
  }

  FastLED.show();
}

// Takes in two arguments, first the array that needs to be adressed. Second the color.
// It uses the color to identify which Matrix was on and turn it off.
void turnOffLEDs(CRGB color){
  Serial.println("turning LEDs OFF");
  if(color == 'Green'){
    fill_solid(leds2, NUM_LEDS, CRGB::Black);
  }
  if(color == 'Red') {
    fill_solid(leds, NUM_LEDS, CRGB:: Black);  
  }

  if(Left == true && color == 'Yellow') {
    fill_solid(leds, NUM_LEDS, CRGB:: Black);
    Left = false;
  }
  if(Right == true && color == 'Yellow') {
    fill_solid(leds2, NUM_LEDS, CRGB:: Black);
    Right = false;
  }

  FastLED.show();
}


//________EXTRA THAT I DONT HAVE A NAME FOR____
void adc_opt() {
    // Configure ADC optimization settings (if needed)
    // NOT YET!!!!!!!!!!
}


void reset_values() {
    delay(1000);
    digitalWrite(BUZZER_PIN, LOW);
    delay(2000);
    // Reset all the other values
    // ...
}









