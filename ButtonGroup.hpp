#ifndef __BUTTONGROUP_H
#define __BUTTONGROUP_H

#include "Arduino.h"

template <unsigned int N>
class ButtonGroup
{
  const int DEBOUNCE;
  int pins[N];
  bool state[N];
  bool volatileState[N];
  unsigned long debounceTime[N];

 public:
  ButtonGroup(int debounceDelay, const int (&_pins)[N])
    : DEBOUNCE(debounceDelay)
  {
    for (unsigned int i = 0; i < N; i++) {
      pins[i] = _pins[i];
      pinMode(pins[i], INPUT_PULLUP);
      state[i] = 0;
      volatileState[i] = 0;
      debounceTime[i] = 0;
    }
  }

  bool IsDown(int n) const
  {
    return state[n];
  }

  // Returns true when the given button transitions from pressed to released.
  bool PollForRelease(int n)
  {
    return PollButton(n) && !state[n];
  }

  bool PollForChange()
  {
    bool changed = false;
    for (int i = 0; i < N; i++) {
      changed |= PollButton(i);
    }
    return changed;
  }

  bool AnyButtonIsDown()
  {
    for (int i = 0; i < N; i++) {
      PollButton(i);
      if (state[i])
        return true;
    }
    return false;
  }

private:
  bool PollButton(int n)
  {
    bool value = digitalRead(pins[n]) == LOW;
    if (value != volatileState[n]) {
      volatileState[n] = value;
      debounceTime[n] = millis();
      return false;
    }
    else if (value != state[n] && (millis() - debounceTime[n]) > DEBOUNCE) {
      state[n] = value;
      return true;
    }
    return false;
  }
};

#endif
