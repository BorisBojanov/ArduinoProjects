/*
Make this Foil only for now
Focus on the esentials
Logic of off target and on target
Timing acuracy
LEDs turning on for set amount of time at the correct moment
Lock out after a certain amount of time
Correct power managment
Speed of baud rate
*/
#include <FastLED.h>
#include <Arduino.h>
#define VOLTS 5
#define MAX_APMS 500 //mili amps
#define NUM_LEDS 60
#define NUM_LEDS_2 60
//#define CHIPSET WS2812B
//#define COLOR_ORDER GRB
#define BRIGHTNESS 30



//CRGB leds[NUM_LEDS]    = {0};
//CRGB leds2[NUM_LEDS_2] = {0};

const int BUTTON_PIN = 21;
const int BUTTON_TWO_PIN = 22;
const int DATA_PIN = 2;
const int DATA_PIN_2 = 4;

unsigned long lastButtonPressTime  = 0;
unsigned long lastButtonPressTime2 = 0; // change name to  tip depress time
const unsigned long ledDuration = 2000;

  //___________SCORE BOX SPECIFIC____________

 //_________Other PINS_______
 const int BUZZER_PIN  = 3; //Output
  //___________Pin Set up for score box____________
  //___________All are INPUT
 const int GROUND_PIN_A = 34; // == D15 //NEEDS A PIN 
 const int WEAPON_PIN_A = 35; // == D16 //NEEDS A PIN
 const int LAME_PIN_A   = 32; // == D17 //NEEDS A PIN
 const int LAME_PIN_B   = 15; // == D18 //NEEDS A PIN
 const int WEAPON_PIN_B = 2; // == D19 //NEEDS A PIN
 const int GROUND_PIN_B = 4; // == D21 //NEEDS A PIN

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
 const int LOCKOUT_TIME = 300000; //Lockout time between hits [foil, epee, sabre] 
 const int DEPRESS_TIME = 14000;   //Minimum depress time for each mode

// Global variables
bool Left  = false;
bool Right = false;
bool hitOnTargetA  = false;
bool hitOffTargetA = false;
bool hitOnTargetB  = false;
bool hitOffTargetB = false;
bool depressedA    = false;
bool depressedB    = false;

void setup() {
  // put your setup code here, to run once:
Serial.begin(115200);
//FastLED.addLeds<CHIPSET, DATA_PIN, COLOR_ORDER>(leds, NUM_LEDS);              //define leds
//FastLED.addLeds<CHIPSET, DATA_PIN_2, COLOR_ORDER>(leds2, NUM_LEDS_2);         // define leds2 
//FastLED.setMaxPowerInVoltsAndMilliamps(VOLTS, MAX_APMS);
//FastLED.setBrightness(BRIGHTNESS);

pinMode(WEAPON_PIN_A, INPUT);
pinMode(GROUND_PIN_A, INPUT);
pinMode(LAME_PIN_A  , INPUT);
pinMode(LAME_PIN_B  , INPUT);
pinMode(GROUND_PIN_B, INPUT);
pinMode(WEAPON_PIN_B, INPUT);



Serial.println("1 Weapon Scoring Box By Boris and Connor");
Serial.println("====================");
Serial.println("Mode: Foil");

//reset_values(); // how do we reset the values
}

void loop() {
  // put your main code here, to run repeatedly:
  // constantly read values from the pins 
  // then compare to the logic set out by the Foil() funcion
  // keep LEDs off unless they are turned on by the Foil function
  Serial.println("I AM ALIVE");
  read_analog_values();
  
  foil();
  //foil(); will call turnOnLEDs
  // then also will call turnOffLEDs
delay(3000);

}



// READ ANALOG VALUES
  void read_analog_values() {
 int weaponAValueVoltge = analogRead(WEAPON_PIN_A);
 int weaponBValueVoltge = analogRead(WEAPON_PIN_B);
 int lameAValueVoltge   = analogRead(LAME_PIN_A);
 int lameBValueVoltge   = analogRead(LAME_PIN_B);
 int groundAValueVoltge = analogRead(GROUND_PIN_A);
 int groundBValueVoltge = analogRead(GROUND_PIN_B);

  // The ESP32 uses a 12-bit ADC, meaning the values range from 0-4095
  // With a 3.3V reference voltage, each step represents approximately 0.0008 volts (3.3V / 4095 = 0.0008V)
  // Therefore, to get the voltage of the pin we must multiply the sensor value by 0.0008
 double weaponAValue = weaponAValueVoltge *0.0008;
 double weaponBValue = weaponBValueVoltge *0.0008;
 double lameAValue = lameAValueVoltge *0.0008;
 double lameBValue = lameBValueVoltge *0.0008;
 double groundAValue = groundAValueVoltge *0.0008;
 double groundBValue = groundBValueVoltge *0.0008;
  // weaponAValue = weaponAValueVoltge;
  // weaponBValue = weaponBValueVoltge;
  // lameAValue = lameAValueVoltge;
  // lameBValue = lameBValueVoltge;
  // groundAValue = groundAValueVoltge;
  // groundBValue = groundBValueVoltge;
  // Serial.println("A Side");
  // Serial.println(weaponAValue);
  // Serial.println(lameAValue);
  // Serial.println(groundAValue);
  // Serial.println("B Side");
  // Serial.println(weaponBValue);
  // Serial.println(lameBValue);
  // Serial.println(groundBValue);
} 



void foil() {

 int weaponAValueVoltge = analogRead(WEAPON_PIN_A);
 int weaponBValueVoltge = analogRead(WEAPON_PIN_B);
 int lameAValueVoltge   = analogRead(LAME_PIN_A);
 int lameBValueVoltge   = analogRead(LAME_PIN_B);
 int groundAValueVoltge = analogRead(GROUND_PIN_A);
 int groundBValueVoltge = analogRead(GROUND_PIN_B);

 double weaponAValue = weaponAValueVoltge *0.0008;
 double weaponBValue = weaponBValueVoltge *0.0008;
 double lameAValue = lameAValueVoltge *0.0008;
 double lameBValue = lameBValueVoltge *0.0008;
 double groundAValue = groundAValueVoltge *0.0008;
 double groundBValue = groundBValueVoltge *0.0008;

  Serial.println("A Side");
  Serial.println(weaponAValue);
  Serial.println(lameAValue);
  Serial.println(groundAValue);
  Serial.println("B Side");
  Serial.println(weaponBValue);
  Serial.println(lameBValue);
  Serial.println(groundBValue);
  unsigned long now = millis(); // Arduino uses millis() to get the number of milliseconds since the board started running.
                                //It's similar to the Python monotonic_ns() function but gives time in ms not ns.
  //_____Check for lockout___
    if (((hitOnTargetA || hitOffTargetA) && (depressAtime + LOCKOUT_TIME < now)) ||
            ((hitOnTargetB || hitOffTargetB) && (depressBtime + LOCKOUT_TIME < now))) {
        lockedOut = true;
        Serial.println("Finished Check for lockout");
    }

    // ___Weapon A___RED SIDE___
    if (!hitOnTargetA && !hitOffTargetA) {
      Serial.println("Weapon A before Off target evaluation");
        // Off target
        if (weaponAValue > 3.0 && lameBValue < 0.9) {
            Serial.println("Inside OFF TARGET A Side");
            if (!depressedA) {
                depressAtime = now;
                depressedA = true;
            }
            if (depressAtime + DEPRESS_TIME <= now) {
                hitOffTargetA = true;
                Serial.println("Red Side OFF TARGET");
                turnOnLEDs();


            }
        } else {
            // On target
            if (weaponAValue > 1.30 && weaponAValue < 1.60 && lameBValue > 1.30 && lameBValue < 1.60) {
                if (!depressedA) {
                    depressAtime = now;
                    depressedA = true;
                } 
                if (depressAtime + DEPRESS_TIME <= now) {
                    hitOnTargetA = true;
                    Serial.println("Red Side ONN TARGET");
                    turnOnLEDs();
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
        if (weaponBValue > 2.0 && lameAValue < 0.9) {
            Serial.println("Inside OFF TARGET B Side");
            if (!depressedB) {
                depressBtime = now;
                depressedB = true;
            }
            if (depressBtime + DEPRESS_TIME <= now) {
                hitOffTargetB = true;
                Serial.print("Green Side OFF TARGET");
                turnOnLEDs();
            }
        } else {
            // On target
            if (weaponBValue > 1.30 && weaponBValue < 1.60 && lameAValue > 1.30 && lameAValue < 1.60) {
                if (!depressedB) {
                    depressBtime = now;
                    depressedB = true;
                }
                if (depressBtime + DEPRESS_TIME <= now) {
                  hitOnTargetB = true;
                  Serial.println("Green Side ONN TARGET");
                  turnOnLEDs();
                }
            } else {
                depressBtime = 0;
                depressedB = false;
            }
        }
    }
}




void turnOnLEDs() {
  Serial.println("Entered turnOnLEDs function.");
  Serial.println(hitOffTargetA);
  //-------------A-Side----------------------------------
  if (hitOffTargetA) {
    Serial.println("Turning leds Yellow due to hitOffTargetA");

    depressAtime = millis();
  } else if (hitOnTargetA) {
    Serial.println("Turning leds Red due to hitOnTargetA");

    depressAtime = millis();
  }
  // Check if enough time has passed since the button was last pressed
  if ( !(millis() - depressAtime <= ledDuration)) {
    //if yes keep the LEDs on
    //KEEP LEDS ONN 
    turnOffLEDs();

  }

  //-------------B-Side----------------------------------

  if (hitOffTargetB) {
    Serial.println("Turning leds2 Blue due to hitOffTargetB");

    depressBtime = millis();
  } else if (hitOnTargetB) {
    Serial.println("Turning leds2 Green due to hitOnTargetB");

    depressBtime = millis();
  }

  if ( !(millis() - depressBtime <= ledDuration)) {
  //if yes keep the LEDs on
  //KEEP LEDS ONN 
  turnOffLEDs( );

  }
  //FastLED.show();

}

void turnOffLEDs(){
    Serial.println("turning LEDs OFF");
    Serial.println("TURNING GREEN LEDS OFF");
    Serial.println("TURNING RED LEDS OFF");  
    Serial.println("TURNING Yellow LEDS OFF");
    Serial.println("TURNING OTHER YELLOW LEDS OFF");

}
