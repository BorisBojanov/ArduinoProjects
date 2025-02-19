#include "TimedState.h"

#include <Arduino.h>

void SimpleState::enter() { forceEnter(); };

void SimpleState::forceEnter() { is_entered = true; }

void SimpleState::exit() { is_entered = false; }

bool SimpleState::isInside() { return is_entered; }

TimedState::TimedState(unsigned long period) : period(period) {}

void TimedState::enter() {
  if (!isInside()) {
    forceEnter();
  }
}

void TimedState::forceEnter() {
  is_entered = true;
  timestamp = millis();
}

void TimedState::exit() {
  is_entered = false;
  timestamp = millis() - period - 100;
}

bool TimedState::isInside() {
  return is_entered && millis() - timestamp <= period;
}

RepeatingTimedState::RepeatingTimedState(unsigned long true_period,
                                         unsigned long false_period,
                                         bool initial_state)
    : true_period(true_period),
      false_period(false_period),
      initial_state(initial_state) {}

void RepeatingTimedState::enter() {
  if (!is_entered) {
    forceEnter();
  }
}

void RepeatingTimedState::forceEnter() {
  is_entered = true;
  timestamp = millis();
}

void RepeatingTimedState::exit() { is_entered = false; }

bool RepeatingTimedState::isInside() {
  if (!is_entered) {
    return false;
  }
  unsigned long current_millis = millis();
  if (current_millis < timestamp) {
    // Actually impossible case, just guard against unsigned long overflow.
    return false;
  }
  unsigned long time_passed = current_millis - timestamp;
  unsigned long mod_time_passed = time_passed % (true_period + false_period);
  if (initial_state) {
    if (mod_time_passed < true_period) {
      return true;
    } else {
      return false;
    }
  } else {
    if (mod_time_passed < false_period) {
      return false;
    } else {
      return true;
    }
  }
}
