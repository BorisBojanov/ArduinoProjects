#define START_SENSOR_PIN_Weapon D2
#define START_SENSOR_PIN_Wires D1   // Analog input pin connected to D7, change according to your setup
#define WEAPON_CHECK_MODE 111
#define BODY_WIRE_CHECK_MODE 222
#define SENSOR_PIN_A0 A0 // Pin connected to the sensor, checkts the voltage for the equipment
#define SENSOR_PIN_A1 A1
#define SENSOR_PIN_A2 A2

#define Transistor_PIN_Foil A3 //These pins output HIGH when you want to connect the 
#define Transistor_PIN_Foil A4 //input socket A and C lines to ground to test Epees or Foils

#define LED_D2 D3 // Pin connected to the LED these LEDs turn on to show that the line is working
#define LED_D3 D4 // Pin connected to the LED
#define LED_D4 D5 // Pin connected to the LED


int mode = WEAPON_CHECK_MODE; // Default mode

void setup() {
  pinMode(START_SENSOR_PIN_Wires, INPUT);
  pinMode(START_SENSOR_PIN_Weapon, INPUT);
  pinMode(SENSOR_PIN_A0, INPUT);
  pinMode(SENSOR_PIN_A1, INPUT);
  pinMode(SENSOR_PIN_A2, INPUT);
  pinMode(LED_D2, OUTPUT);
  pinMode(LED_D3, OUTPUT);
  pinMode(LED_D4, OUTPUT);
  Serial.begin(115200);


}
  // The ESP32 uses a 12-bit ADC, meaning the values range from 0-4095
  // With a 3.3V reference voltage, each step represents approximately 0.0008 volts (3.3V / 4095 = 0.0008V)
  // Therefore, to get the voltage of the pin we must multiply the sensor value by 0.0008
void loop() {
  int sensorValue1 = analogRead(SENSOR_PIN_A0);
  int sensorValue2 = analogRead(SENSOR_PIN_A1);
  int sensorValue3 = analogRead(SENSOR_PIN_A2);
  double voltage1 = sensorValue1 * 0.0008;
  double voltage2 = sensorValue2 * 0.0008;
  double voltage3 = sensorValue3 * 0.0008; 

  
  if(digitalRead(DigitalInputPIN) == HIGH){
    mode = WEAPON_CHECK_MODE;
    //Serial.println("Mode: Weapon Check");
  }
  else if(digitalRead(DigitalInputPIN) == LOW){
    mode = BODY_WIRE_CHECK_MODE;
    //Serial.println("Mode: Body Wire Check");
  }


  // mode specific code here
  switch(mode) {

    case WEAPON_CHECK_MODE:
        // wire 1 LED check
        if(voltage1 < 0.752){
            digitalWrite(LED_D2, HIGH);
        } else {
            digitalWrite(LED_D2, LOW);
        }
        // wire 2 LED check
        if(voltage2 < 0.752){
            digitalWrite(LED_D3, HIGH);
        } else {
            digitalWrite(LED_D3, LOW);
        }
        // wire 3 LED check
        if(voltage3 < 0.752){
            digitalWrite(LED_D4, HIGH);
        } else {
            digitalWrite(LED_D4, LOW);
        }        
      break;

            // Code body wire Check 
    case BODY_WIRE_CHECK_MODE:
        // wire 1 LED check
        if(voltage1 < 0.604){
            digitalWrite(LED_D2, HIGH);
        } else {
            digitalWrite(LED_D2, LOW);
        }
        // wire 2 LED check
        if(voltage2 < 0.602){
            digitalWrite(LED_D3, HIGH);
        } else {
            digitalWrite(LED_D3, LOW);
        }
        // wire 3 LED check
        if(voltage1 < 0.604){
            digitalWrite(LED_D4, HIGH);
        } else {
            digitalWrite(LED_D4, LOW);
        }
        

      break;
    default:
      // Default behavior
      break;
  }

  delay(1000);  // Delay to limit the frequency of mode checking and serial output
}
