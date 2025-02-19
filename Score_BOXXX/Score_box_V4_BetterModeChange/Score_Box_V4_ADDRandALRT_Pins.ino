//Score_Box_V4_ADDRandALRT_Pins
//Each Weapon will have its own ADS1115
//Each ADS1115 will have its own I2C address
//Each ADS1115 will have its own Alert Pin
//Each Alert Pin will be connected to a different pin on the Arduino and set up as interrupts

#include <Wire.h>
#include <Adafruit_ADS1X15.h>

Adafruit_ADS1115 ads;  // Create an instance of the ADS1115

// ISR for the ALERT pin
void IRAM_ATTR onADS1115Alert() {
    // Read the conversion value
    int16_t adcValue1 = ads.readADC_SingleEnded(0); // Weapon A
    int16_t adcValue2 = ads.readADC_SingleEnded(1); // Lame B
    int16_t adcValue3 = ads.readADC_SingleEnded(2); // Weapon B
    int16_t adcValue4 = ads.readADC_SingleEnded(3); // Lame A

    ads.readADC_Differential_0_1();
    ads.readADC_Differential_2_3();

    Serial.println(adcValue1);
    Serial.println(adcValue2);
    Serial.println(adcValue3);
    Serial.println(adcValue4);
}

void setup() {
    Serial.begin(115200);
    Wire.begin();
    ads.begin();

    ads.setGain(GAIN_ONE); // Set gain depending on your expected voltage range
    ads.startComparator_SingleEnded(0, 8500); // Assuming you're using channel 0 and threshold of 8500
    ads.startComparator_SingleEnded(1, 8500); // Assuming you're using channel 1 and threshold of 8500
    ads.startComparator_SingleEnded(2, 8500); // Assuming you're using channel 2 and threshold of 8500
    ads.startComparator_SingleEnded(3, 8500); // Assuming you're using channel 3 and threshold of 8500
    // Set up the ESP32 pin for external interrupt
    pinMode(27, INPUT_PULLUP); // Set Pin 27 as input with pull-up
    attachInterrupt(digitalPinToInterrupt(27), onADS1115Alert, RISING); // Trigger ISR on falling edge
    
}

void loop() {
// Your main code
// The ISR will handle the ADS1115 readings
}

//ADS1 For OnTarget
/*
ALRT = CHANG >= OnTarget
A0 = weaponA
A1 = weaponB
A2 = lameA
A3 = lameB
*/

//ADS2 For OffTarget
/*
ALRT = CHANG >= OffTarget
A0 = weaponA
A1 = weaponB
A2 = lameA
A3 = lameB
*/

//ADS3 For Short to ground
/*
ALRT = CHANG >= Short
A0 = weaponA
A1 = weaponB
A2 = groundA
A3 = groundB
*/ 

