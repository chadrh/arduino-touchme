#ifndef __BUTTONGROUP_H
#define __BUTTONGROUP_H

#include "Arduino.h"

template <unsigned short N>
class ButtonGroup
{
  const unsigned long DEBOUNCE;
  int pins[N];
  bool state[N];
  bool volatileState[N];
  unsigned long debounceTime[N];

 public:
  ButtonGroup(int debounceDelay, const int (&_pins)[N])
    : DEBOUNCE(debounceDelay)
  {
    for (unsigned short i = 0; i < N; i++) {
      pins[i] = _pins[i];
      pinMode(pins[i], INPUT_PULLUP);
      state[i] = false;
      volatileState[i] = false;
      debounceTime[i] = 0;
    }
  }

  void WaitForButtons()
  {
    PollForChange();
    do {
      delay(DEBOUNCE + 1);
    } while (AnyButtonIsDown());
  }

  bool IsDown(unsigned short n) const
  {
    return state[n];
  }

  // Returns true when the given button transitions from pressed to released.
  bool PollForRelease(unsigned short n)
  {
    return PollButton(n) && !state[n];
  }

  bool PollForChange()
  {
    bool changed = false;
    for (unsigned short i = 0; i < N; i++) {
      changed |= PollButton(i);
    }
    return changed;
  }

  bool AnyButtonIsDown()
  {
    for (unsigned short i = 0; i < N; i++) {
      PollButton(i);
      if (state[i])
        return true;
    }
    return false;
  }

private:
  bool PollButton(unsigned short n)
  {
    if (n == N - 1)
      return false; // WTF evil hack
    bool value = digitalRead(pins[n]) == LOW;
    if (value != volatileState[n]) {
      volatileState[n] = value;
      debounceTime[n] = millis();
    }
    else if (value != state[n] && (millis() - debounceTime[n]) > DEBOUNCE) {
      state[n] = value;
      return true;
    }
    return false;
  }
};

#endif
