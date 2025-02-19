#include <Arduino.h>
#include <TimedState.h>

/**
 * @brief This is the example Arduino sketch for the TimedState library.
 *
 * What are we going to do here:
 * 1. In the first 20 seconds, we only print message through Serial port.
 * 2. After that, we make the builtin LED blink in a 2 seconds off and 1 seconds
 * on loop, with the 2 seconds off starts first.
 * 3. However, we shutdown the blinking LED for 5 seconds per 20 seconds, which
 * means that for the first 15 seconds of the loop the LED is allowed to blink,
 * and then it will shutdown for 5 seconds in the end of the loop.
 * 4. After two minutes (120 seconds), shutdown everything.
 *
 * With delay() you think of it to be something like:
 * startPrintingSerialMessage();
 * stopBlinking();
 * delay(20 * 1000);
 * while (true){
 *  startBlinking();
 *  delay(15 * 1000);
 *  stopBlinking();
 *  delay(5 * 1000);
 * }
 *
 */

TimedState before_shutdown_state(120000ul);
TimedState serial_only_state(20 * 1000);
RepeatingTimedState blinking_state(1 * 1000, 2 * 1000, false);
RepeatingTimedState led_allowed_state(20 * 1000, 5 * 1000, true);
RepeatingTimedState log_serial_state(1, 199, true);

void setup() {
  // initialize digital pin LED_BUILTIN as an output.
  Serial.begin(9600);
  pinMode(LED_BUILTIN, OUTPUT);

  // Using forceEnter() just for testing. Normally you should use enter().
  serial_only_state.forceEnter();
  blinking_state.forceEnter();
  led_allowed_state.forceEnter();
  before_shutdown_state.forceEnter();
  log_serial_state.forceEnter();
  // serial_only_state.enter();
  // blinking_state.enter();
  // led_allowed_state.enter();
  // before_shutdown_state.enter();
}

void loop() {
  if (before_shutdown_state.isInside()) {
    // Sanity test here, enter() here should not have any effect
    blinking_state.enter();
    led_allowed_state.enter();
  } else {
    serial_only_state.exit();
    blinking_state.exit();
    led_allowed_state.exit();
  }

  unsigned long current_millis = millis();
  if (log_serial_state.isInside()) {
    Serial.print("time: ");
    Serial.print(current_millis);
    Serial.print(", serial_only: ");
    Serial.print(serial_only_state.isInside() ? "true" : "false");
    Serial.print(", blinking: ");
    Serial.print(blinking_state.isInside() ? "true" : "false");
    Serial.print(", led_allowed: ");
    Serial.print(led_allowed_state.isInside() ? "true" : "false");
    Serial.print(", led: ");
    Serial.print((!serial_only_state.isInside() && blinking_state.isInside() &&
                  led_allowed_state.isInside())
                     ? "true"
                     : "false");
    Serial.println(":");
  }

  if (!serial_only_state.isInside() && blinking_state.isInside() &&
      led_allowed_state.isInside())
    digitalWrite(LED_BUILTIN, HIGH);
  else
    digitalWrite(LED_BUILTIN, LOW);
}