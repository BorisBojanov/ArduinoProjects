
#include "esp32-hal-cpu.h"

void setup(void)
{
  Serial.begin(115200);

  setCpuFrequencyMhz(240); //Set CPU clock to 80MHz fo example
  Serial.println();
  Serial.print("CPU Frequency is: ");
  Serial.print(getCpuFrequencyMhz()); //Get CPU clock
  Serial.print(" Mhz");
  Serial.println();
}

void loop()
{
}