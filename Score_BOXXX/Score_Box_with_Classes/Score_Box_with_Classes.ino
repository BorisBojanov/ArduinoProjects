//Score Box with Classes
/*
Remember, in Arduino, micros() rolls over (goes back to zero) approximately every 70 minutes,
so if your application runs longer than that, you'll need to handle the rollover case.
*/

#include <Wire.h>
#include <Adafruit_ADS1X15.h>
#include <Arduino.h>
#include <iostream>

int32_t timerStartA;
int32_t timerStartB;

Adafruit_ADS1115 ads1115_weaponONTARGET_A_B;  // ADS1115 with ADDR pin floating (default address 0x48)
Adafruit_ADS1115 ads1115_weaponOFFTARGET_A_B;     // ADS1115 with ADDR pin connected to 3.3V (address 0x49)

// Representing 300 microseconds
const uint32_t Foil_LOCKOUT_TIME = micros(300);
// Representing 45 microseconds
const uint32_t Epee_LOCKOUT_TIME = micros(45);
// Representing 170 microseconds
const uint32_t Sabre_LOCKOUT_TIME = micros(170);

const int32_t Foil_DEPRESS_TIME  = micros(2000); // 2000 microseconds = 2 milliseconds
const int32_t Epee_DEPRESS_TIME  = micros(1000); // 1000 microseconds = 1 milliseconds
const int32_t Sabre_DEPRESS_TIME = micros(10); // 10 microseconds = .01 milliseconds

int handleHitOnTargetA(int32_t weaponA_ADS_reading, int32_t lameB_ADS_reading, int32_t& timeHitWasMade) {
    timeHitWasMade = micros(); 
    if(1000 >= ads1115_weaponONTARGET_A_B.readADC_Differential_0_1()){
        return 1;
    }
}
int handleHitOnTargetB(int32_t weaponB_ADS_reading, int32_t lameB_ADS_reading, int32_t& timeHitWasMade) {
    timeHitWasMade = micros(); 
    if (1000 < ads1115_weaponONTARGET_A_B.readADC_Differential_2_3()) {
        return 2;
    }
}
int handleHitOffTargetA(int32_t weaponA_ADS_reading, int32_t& timeHitWasMade) {
    timeHitWasMade = micros(); 
    if (weaponA_ADS_reading > 26100) {
        return 3;
    }
}
int handleHitOffTargetB(int32_t weaponB_ADS_reading, int32_t& timeHitWasMade) {
    timeHitWasMade = micros(); 
    if (weaponB_ADS_reading > 26100) {
        return 4;
    }
}


class Weapon {
public:
    //name of constructor is the same as name of the class

    virtual int getLockoutTime() const = 0;
    virtual int getDepressTime() const = 0;
    // virtual methods as needed
    int handleHitOnTargetA(int32_t weaponA_ADS_reading, int32_t lameB_ADS_reading, int32_t& timeHitWasMade) {
    timeHitWasMade = micros(); 
    if(1000 >= ads1115_weaponONTARGET_A_B.readADC_Differential_0_1()){
        return 1;
    }
    }
    int handleHitOnTargetB(int32_t weaponB_ADS_reading, int32_t lameB_ADS_reading, int32_t& timeHitWasMade) {
        timeHitWasMade = micros(); 
        if (1000 < (ads1115_weaponONTARGET_A_B.readADC_Differential_2_3)) {
            return 2;
        }
    }
    int handleHitOffTargetA(int32_t weaponA_ADS_reading, int32_t& timeHitWasMade) {
        timeHitWasMade = micros(); 
        if (weaponA_ADS_reading > 26100) {
            return 3;
        }
    }
    int handleHitOffTargetB(int32_t weaponB_ADS_reading, int32_t& timeHitWasMade) {
        timeHitWasMade = micros(); 
        if (weaponB_ADS_reading > 26100) {
            return 4;
        }
    }
};

class Foil : public Weapon {
private:
    /*To ensure that this instance is accessible within these classes. 
    Pass this instance to the constructor of each class or use a setter method.*/
    Adafruit_ADS1X15 ads1115_weaponONTARGET_A_B; // Reference to the ADS1115 instance
    Adafruit_ADS1115 ads1115_weaponOFFTARGET_A_B;
public:
    int getLockoutTime() const override { return Foil_LOCKOUT_TIME; }
    int getDepressTime() const override { return Foil_DEPRESS_TIME; }
    // Specific implementations for Foil

    int handleHitOnTargetA(int32_t& timeHitWasMade) {
        int32_t timeHitWasMade;
        timeHitWasMade = micros(); 
        int32_t weaponA_ADS_reading = ads1115_weaponONTARGET_A_B.readADC_SingleEnded(0); // Example reading
        int32_t lameB_ADS_reading = ads1115_weaponONTARGET_A_B.readADC_SingleEnded(1); // Example reading
        // Additional logic for handling the result
        // result is a number between 1 and 4 
        // 1 = WeaponA hit on target
        // 2 = WeaponB hit on target
        // 3 = WeaponA hit off target
        // 4 = WeaponB hit off target
        if(1000 > ads1115_weaponONTARGET_A_B.readADC_Differential_0_1()){
            return 1;
        }
        // Add more logic as needed
    }

};

class Epee : public Weapon {
public:
    int getLockoutTime() const override {
        // Implement for Epee
    }
    int getDepressTime() const override {
        // Implement for Epee
    }
    // Add other Epee-specific methods here
};

class Sabre : public Weapon {
public:
    int getLockoutTime() const override {
        // Implement for Sabre
    }
    int getDepressTime() const override {
        // Implement for Sabre
    }
    // Add other Sabre-specific methods here
};


enum WeaponType { FOIL, EPEE, SABRE };

Weapon* getWeapon() {
    WeaponType type;
    //buttonState = LOW;
    if (type == FOIL ) {
        return new Epee();
    } else if (type == EPEE) {
        return new Sabre();
    } else {
        return new Foil();
    }
}
//the getWeapon function is used like this:
//Weapon* weapon = getWeapon(FOIL);

void setup(void)
{
  Serial.begin(9600);
  Serial.println("Hello!");
  
  Serial.println("Getting single-ended readings from AIN0..3");
  Serial.println("ADC Range: +/- 6.144V (1 bit = 3mV)");
  ads1115_weaponONTARGET_A_B.begin(0x49);
  ads1115_weaponOFFTARGET_A_B.begin(0x48);
  ads1115_weaponONTARGET_A_B.setGain(GAIN_ONE);
  ads1115_weaponOFFTARGET_A_B.setGain(GAIN_ONE);
  WeaponType FOIL;
  Weapon* weapon = getWeapon(); // This is the weapon mode
  
  /*
  If weapon value is greater than 13100 then it is on target
  If weapon value is greater than 26000 then it is off target
  ADS1 Checks for onTarget PIN; 0 = WeaponA, 1 = LameA, 2 = WeaponB, 3 = LameB
  ADS2 Checks for OffTarget PIN; 0 = WeaponA, 1 = LameA, 2 = WeaponB, 3 = LameB
  */
   // Setup 3V comparator on channel 1,3 for Lame A and B for the OnTarget checks
   // Setup 3V comparator on channel 0,2 for Weapon A and B for the OffTarget checks
   //void startComparator_SingleEnded(uint8_t channel, int16_t threshold);
   //this will trigger the interrupt pin when the value is greater than the threshold on the ALRT pin
  ads1115_weaponONTARGET_A_B.startComparator_SingleEnded(1, 13100);//WeaponA would hitOnTargetA = true pin 0
  ads1115_weaponONTARGET_A_B.startComparator_SingleEnded(3, 13100);//WeaponB would hitOnTargetB = true pin 3
  ads1115_weaponOFFTARGET_A_B.startComparator_SingleEnded(0, 26100);//WeaponA would hitOffTargetA = true pin 0
  ads1115_weaponOFFTARGET_A_B.startComparator_SingleEnded(2, 26100);//WeaponB would hitOffTargetB = true pin 3
}

void loop{// Example usage
currentWeapon->handleOnTargetHit();
currentWeapon->handleOffTargetHit();
}