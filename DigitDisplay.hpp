#ifndef __DIGITDISPLAY_H
#define __DIGITDISPLAY_H

#include "Arduino.h"

class DigitDisplay
{
  const int clockPin;
  const int latchPin;
  const int dataPin;

public:
  DigitDisplay(int _clockPin, int _latchPin, int _dataPin)
    : clockPin(_clockPin), latchPin(_latchPin), dataPin(_dataPin)
  {
    pinMode(clockPin, OUTPUT);
    pinMode(latchPin, OUTPUT);
    pinMode(dataPin, OUTPUT);
  }
  void WriteDigit(int digit) const
  {
    if (digit < 0 || digit > 0xf)
      WriteByte(0);
    else
      WriteByte(digitValues[digit]);
  }
  void Blank() const
  {
    WriteByte(0);
  }

private:
  void WriteByte(byte value) const
  {
    digitalWrite(latchPin, LOW);
    shiftOut(dataPin, clockPin, MSBFIRST, value);
    digitalWrite(latchPin, HIGH);
  }

  static const byte digitValues[16];
};

const byte DigitDisplay::digitValues[16] = {
  252, 96, 218, 242,
  102, 182, 190, 224,
  254, 246, 238, 62,
  156, 122, 158, 142 };

#endif
