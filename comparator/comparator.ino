#include <Adafruit_ADS1X15.h>

// Adafruit_ADS1115 ads;  /* Use this for the 16-bit version */
Adafruit_ADS1015 ads;     /* Use this for the 12-bit version */
const int ALRT_PIN = 34;
volatile bool OnTargetA_Flag = false;
void ISR_OnTargetA(){//This function is potentially being called very often 
  OnTargetA_Flag = true;
}
void setup(void)
{
  Serial.begin(9600);
  Serial.println("Hello!");

  Serial.println("Single-ended readings from AIN0 with >3.0V comparator");
  Serial.println("ADC Range: +/- 6.144V (1 bit = 3mV/ADS1015, 0.1875mV/ADS1115)");
  Serial.println("Comparator Threshold: 1000 (3.000V)");

  pinMode(ALRT_PIN, INPUT);
  attachInterrupt(digitalPinToInterrupt(ALRT_PIN), ISR_OnTargetA, RISING); // Delta of A0 and A1 will go from Delta = Large value to Delta = 0

  ads.begin(0x48);
  // The ADC input range (or gain) can be changed via the following
  // functions, but be careful never to exceed VDD +0.3V max, or to
  // exceed the upper and lower limits if you adjust the input range!
  // Setting these values incorrectly may destroy your ADC!
  //                                                                ADS1015  ADS1115
  //                                                                -------  -------
  // ads.setGain(GAIN_TWOTHIRDS);  // 2/3x gain +/- 6.144V  1 bit = 3mV      0.1875mV (default)
  ads.setGain(GAIN_ONE);        // 1x gain   +/- 4.096V  1 bit = 2mV      0.125mV
  // ads.setGain(GAIN_TWO);        // 2x gain   +/- 2.048V  1 bit = 1mV      0.0625mV
  // ads.setGain(GAIN_FOUR);       // 4x gain   +/- 1.024V  1 bit = 0.5mV    0.03125mV
  // ads.setGain(GAIN_EIGHT);      // 8x gain   +/- 0.512V  1 bit = 0.25mV   0.015625mV
  // ads.setGain(GAIN_SIXTEEN);    // 16x gain  +/- 0.256V  1 bit = 0.125mV  0.0078125mV
  ads.setDataRate(3300);
  ads.Set_HIGH_LOW_Threshold_REGisters(1500, 2000);
  if (!ads.begin()) {
    Serial.println("Failed to initialize ADS.");
    while (1);
  }
  // Setup 3V comparator on channel 0
  ads.startComparator_SingleEnded(0);
}

void loop(void)
{
  int16_t adc0;

  // Comparator will only de-assert after a read
  adc0 = ads.getLastConversionResults();
  Serial.print("ALRT_PIN: "); Serial.print(digitalRead(ALRT_PIN)); Serial.print(" AIN0: "); Serial.println(adc0);

  delay(100);
}