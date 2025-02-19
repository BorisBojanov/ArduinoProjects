#ifndef ARDUINO_TIMED_STATE_H

#define ARDUINO_TIMED_STATE_H

#include <Arduino.h>

// An abstract interface for different types of State. enter() to enter a state.
// exit() to manually exit a state. isInside() to check whether inside the
// state. By default, if the state has been entered already, calling enter()
// again would have no effect. It is possible to override this behavior by
// calling forceEnter(), which will enter() the state again even it has been
// entered previously.
class State {
 public:
  // Enter a state. Calling enter() after the state is already entered will have
  // no effect.
  virtual void enter();

  // Forcefully enter a state. Calling forceEnter() after the state is already
  // entered will enter the state again. This is very useful for resetting the
  // timer for types of State that will exit itself automatically.
  virtual void forceEnter();

  // Manually exit a state. There are no guarantees that the state will only be
  // exited after exit() is called. For example, some time-based types of State
  // will exit itself automatically after a certain period of time. However, it
  // is guaranteed that, after this exit() is called, the state must be exited,
  // and will not be re-entered unless enter() or forceenter() is called.
  virtual void exit();

  // Return whether is inside the state.
  virtual bool isInside();
};

// A simple State. Initially, isInside() returns false. Once isInside() returns
// true once enter() is called, until exit() is called, in which isInside()
// returns false after exit() is called. Calling forceEnter() is same as calling
// enter().
//
// Logic flow:
// (false) --> [enter()] --> (true) --> [exit()] --> (false)
class SimpleState : State {
 protected:
  bool is_entered = false;

 public:
  void enter() override;
  void forceEnter() override;
  void exit() override;
  bool isInside() override;
};

// A timed State that will exit() itself after a certain period. Once enter() is
// called, isInside() will become true, until either exit() has been called, or
// [period] milliseconds has passed since the enter() has been called. If
// forceEnter() is called, isInside() will become true, and the timer will be
// resetted as well.
//
// Logic flow:
// (false) --> [enter()] --> (true) --> [exit() or period milliseconds has
// passed] --> (false)
class TimedState : public State {
 protected:
  bool is_entered = false;
  unsigned long timestamp = 0;
  unsigned long period = 100;

 public:
  TimedState(unsigned long period);
  void enter() override;
  void forceEnter() override;
  void exit() override;
  bool isInside() override;
};

// A repeating timed State that will automatically alternating between true and
// false for a specificiable period. If [initial_state] is true and enter() is
// called, isInside() will return true for [true_period] milliseconds, and then
// return false for [false_period] milliseconds, and then true for [true_period]
// milliseconds, and then keep looping until exit() has been called, in which
// isInside() must return false since then. If [initial_state] is false and
// enter() is called, isInside() will return false for [false_period]
// milliseconds, and then return true for [true_period] milliseconds, and then
// false for [false_period] milliseconds, and then keep looping until exit() has
// been called, in which isInside() must return false since then. Calling
// forceEnter() will reset the timer and hence restarting the whole loop.
//
// Logic flow for [initial_state] == true:
// (false) --> [enter()] --> (true) --> [true_period milliseconds has passed]
// --> (false) --> [false_period milliseconds has passed] --> (true) -->
// [true_period milliseconds has passed] --> ... --> [exit()] --> (false)
//
// Logic flow for [initial_state] == false:
// (false) --> [enter()] --> (false) --> [false_period milliseconds has passed]
// --> (true) --> [true_period milliseconds has passed] --> (false) -->
// [false_period milliseconds has passed] --> ... --> [exit()] --> (false)
class RepeatingTimedState : public State {
 protected:
  bool is_entered = false;
  unsigned long timestamp = 0;
  unsigned long true_period = 100;
  unsigned long false_period = 100;
  bool initial_state;

 public:
  RepeatingTimedState(unsigned long true_period, unsigned long false_period,
                      bool initial_state);
  void enter() override;
  void forceEnter() override;
  void exit() override;
  bool isInside() override;
};

#endif