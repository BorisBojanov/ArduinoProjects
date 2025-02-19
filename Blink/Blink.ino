/*
  Blink

  Turns an LED on for one second, then off for one second, repeatedly.

  Most Arduinos have an on-board LED you can control. On the UNO, MEGA and ZERO
  it is attached to digital pin 13, on MKR1000 on pin 6. LED_BUILTIN is set to
  the correct LED pin independent of which board is used.
  If you want to know what pin the on-board LED is connected to on your Arduino
  model, check the Technical Specs of your board at:
  https://www.arduino.cc/en/Main/Products

  modified 8 May 2014
  by Scott Fitzgerald
  modified 2 Sep 2016
  by Arturo Guadalupi
  modified 8 Sep 2016
  by Colby Newman

  This example code is in the public domain.

  https://www.arduino.cc/en/Tutorial/BuiltInExamples/Blink
*/

// Potentiometer is connected to GPIO 34 (Analog ADC1_CH6) 
const int potPin34 = 34;
const int potPin35 = 35;
const int potPin32 = 32;

const int potPin15 = 15;
const int potPin2 = 2;
const int potPin4 = 4;

// variable for storing the potentiometer value
int potValue34 = 0;
int potValue35 = 0;
int potValue32 = 0;

int potValue15 = 0;
int potValue2 = 0;
int potValue4 = 0;

void setup() {
  Serial.begin(115200);
  delay(1000);
}

void loop() {
  // Reading potentiometer value
  potValue34 = analogRead(potPin34);
  potValue35 = analogRead(potPin35);
  potValue32 = analogRead(potPin32);

  potValue15 = analogRead(potPin15);
  potValue2 = analogRead(potPin2);
  potValue4 = analogRead(potPin4);
  Serial.print("Lame pin: ");
  Serial.println(potValue15);
  Serial.print("Weapon Pin: ");
  Serial.println(potValue35);

  //Serial.println(potValue34);
  //Serial.println(potValue35);
  //Serial.println(potValue32);
  delay(500);
}