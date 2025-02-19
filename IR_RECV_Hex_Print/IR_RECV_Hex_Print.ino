#include <IRremote.h>

const int RECV_PIN = 4;
//Define IR Receiver and Results Objects
IRrecv irrecv(RECV_PIN);
decode_results results;

void setup() {
  Serial.begin(9600);
  //Enable the IR Reciever
  irrecv.EnableIRIN();
}

void loop() {
  //if we decode AND get results in the results Object then
  if (irrecv.decode(&results)) { 
    //Print the Results to Serial Monitor in HEX and then resume
    Serial.println(results.value, HEX);
    irrecv.resume();
  }
}
