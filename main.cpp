#include "Arduino.h"

#include "DigitDisplay.hpp"
#include "ButtonGroup.hpp"

// Timing constants
constexpr int DEBOUNCE = 40;
constexpr int TIMEOUT = 4000;
constexpr int BLINK_TIME = 200; // when displaying sequence
constexpr int NEXT_LEVEL_DELAY = 800; // when adding to the sequence

constexpr int BUTTONCOUNT = 4;
constexpr int MAX_SEQ_LEN = 10;

// Pin assignments
constexpr int CLOCK_PIN = A0;
constexpr int LATCH_PIN = A1;
constexpr int DATA_PIN = A2;
constexpr int MY_TURN = A3;
constexpr int YOUR_TURN = A4;
constexpr int UNUSED_ANALOG = A5;
constexpr int ledPins[BUTTONCOUNT] = { 2, 3, 4, 5 };
constexpr int buttonPins[BUTTONCOUNT + 2] = { 6, 7, 8, 9, 11, 12 }; // 12 is actually unused
constexpr int STATUS_LED = 13;
constexpr int BUZZER = 10;


// R: 2376?
// Y:
// O:
// ?: 2148
// G: 2865
constexpr const int frequencies[5] = { 121, 1000, 2376, 4000, 5000 };

class IO
{
  DigitDisplay digit;

public:
  ButtonGroup<BUTTONCOUNT + 2> buttons;

  IO()
    : digit(CLOCK_PIN, LATCH_PIN, DATA_PIN)
    , buttons(DEBOUNCE, buttonPins)
  {
    pinMode(STATUS_LED, OUTPUT);
    pinMode(MY_TURN, OUTPUT);
    pinMode(YOUR_TURN, OUTPUT);
    pinMode(BUZZER, INPUT);
    for (int i = 0; i < BUTTONCOUNT; i++) {
      pinMode(ledPins[i], OUTPUT);
    }
    SeedRandom();
    digit.Blank();
  }
  void SeedRandom()
  {
    // TODO: reads are 10 bits of resolution, so gather at least 3.
    randomSeed(analogRead(A5));
  }
  void WriteDigit(int val) const
  {
    digit.WriteDigit(val);
  }
  void WritePin(int pinNum, bool value) const
  {
    digitalWrite(pinNum, value ? HIGH : LOW);
  }
  void SetLED(int num, bool on = true) const
  {
    WritePin(ledPins[num], on);
  }
  void Buzzer(int num) const
  {
    pinMode(BUZZER, OUTPUT);
    tone(BUZZER, frequencies[num]);
  }
  void BuzzerOff() const
  {
    noTone(BUZZER);
    pinMode(BUZZER, INPUT);
  }
  void MyTurn(bool isMyTurn = true)
  {
    WritePin(MY_TURN, isMyTurn);
    WritePin(YOUR_TURN, !isMyTurn);
  }
  void Blink(int num, int time, bool buzz = true) const
  {
    SetLED(num);
    if (buzz) {
      Buzzer(num);
    }
    delay(time);
    if (buzz) {
      BuzzerOff();
    }
    SetLED(num, false);
  }
  void Demo()
  {
    unsigned long lastTime = 0;
    int light = -1;
    int c = 0;

    // Make sure all buttons are released.
    while (buttons.AnyButtonIsDown()) { delay(42); }
    do { delay(42); } while (buttons.AnyButtonIsDown());

    while (!buttons.PollForRelease(BUTTONCOUNT)) {
      unsigned long time = millis();
      if (time - lastTime > 1000) {
        int prev = light;
        do {
          light = random(BUTTONCOUNT);
        } while (light == prev);
        lastTime = time;

        WriteDigit((c++) % 10);
        MyTurn(c % 2);
        SetLED(prev, false);
        SetLED(light, true);
      }
    }
    // Make sure all other buttons are released as well.
    while (buttons.AnyButtonIsDown()) { delay(42); }
    do { delay(42); } while (buttons.AnyButtonIsDown());

    MyTurn();
    if (light >= 0)
      SetLED(light, false);
    WriteDigit(0);
  }
} io;

class State
{
  byte sequenceLen = 0;
  byte inputPosition = 0;
  int currentButton = -1;
  int sequence[MAX_SEQ_LEN];
  unsigned long lastInputTime;

  void youWin()
  {
    // chase 5 times
    for (int i = 0; i < 5; i++) {
      // 0.5s for 1 ltr chase
      for (int i = 0; i < BUTTONCOUNT; i++) {
        if (i > 0)
          io.SetLED(i-1, false);
        io.SetLED(i);
        delay(100);
      }
      io.SetLED(BUTTONCOUNT-1, false);
      delay(100);
    }
    restart();
  }
  void youLose()
  {
    io.Buzzer(BUTTONCOUNT);
    delay(800);
    io.BuzzerOff();
    restart();
  }
  void restart()
  {
    delay(2000);
    NewGame();
  }
  void addToSequence()
  {
    io.MyTurn();
    if (sequenceLen == MAX_SEQ_LEN) {
      youWin();
      return;
    }

    sequence[sequenceLen++] = random(BUTTONCOUNT);
    DisplaySequence();

    currentButton = -1;
    inputPosition = 0;
    lastInputTime = millis();
    io.MyTurn(false);
  }
  void DisplaySequence()
  {
    for (byte i = 0; i < sequenceLen; i++) {
      int num = sequence[i];
      io.Blink(num, BLINK_TIME);
      if (i < sequenceLen - 1)
        delay(BLINK_TIME);
    }
  }
public:
  void NewGame()
  {
    io.Demo();
    delay(1000);
    io.WriteDigit(0);
    sequenceLen = 0;
    addToSequence();
  }
  void ProcessInput()
  {
    if (currentButton < 0) {
      // Currently waiting for a button to be pressed.

      if (io.buttons.PollForChange()) {
        // make sure exactly one was pressed.
        for (int i = 0; i < BUTTONCOUNT; i++) {
          if (io.buttons.IsDown(i)) {
            if (currentButton < 0)
              currentButton = i;
            else
              currentButton = BUTTONCOUNT + 1; // cause failure
          }
        }

        // False alarm; start button.
        if (currentButton == -1)
          return;

        // verify that it's the correct button
        int correctButton = sequence[inputPosition++];
        if (currentButton != correctButton)
          youLose();
        else {
          // display confirmation
          io.SetLED(currentButton);
          io.Buzzer(currentButton);
          lastInputTime = millis();
        }
      } else {
        // nothing pressed: do we need to timeout?
        unsigned long currentTime = millis();
        if (currentTime - lastInputTime >= TIMEOUT)
          youLose();
      }
      return;
    }

    // Currently waiting for button to be released.
    if (!io.buttons.PollForChange()) {
      // check the timeout here as well, in case some joker is holding down the button
      unsigned long currentTime = millis();
      if (currentTime - lastInputTime >= TIMEOUT) {
        io.SetLED(currentButton, false);
        io.BuzzerOff();
        youLose();
      }
      return;
    }

    for (int i = 0; i < BUTTONCOUNT; i++) {
      bool down = io.buttons.IsDown(i);
      if (down && (i != currentButton)) {
        // some other button was pressed!
        io.SetLED(currentButton, false);
        io.BuzzerOff();
        youLose();
        return;
      }
    }
    if (io.buttons.IsDown(currentButton))
      return;

    // button released, move on.
    io.SetLED(currentButton, false);
    io.BuzzerOff();
    if (inputPosition == sequenceLen) {
      // passed the test! make the sequence longer
      io.WriteDigit(sequenceLen);
      delay(NEXT_LEVEL_DELAY);
      addToSequence();
    } else {
      lastInputTime = millis();
    }
    currentButton = -1;
  }
} state;

void setup() {
  state.NewGame();
}

void loop() {
  state.ProcessInput();
}
