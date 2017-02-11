#include "Arduino.h"

#include "DigitDisplay.hpp"

constexpr int BUTTONCOUNT = 4;
constexpr int MAX_SEQ_LEN = 10;
constexpr int TIMEOUT = 4000;
constexpr int DEBOUNCE = 40;
constexpr int BLINK_TIME = 200; // when displaying sequence
constexpr int NEXT_LEVEL_DELAY = 800; // when adding to the sequence
constexpr int STATUS_LED = 13;
constexpr int BUZZER = 10;
constexpr int MY_TURN = A3;
constexpr int YOUR_TURN = A4;
constexpr int DATA_PIN = A2;
constexpr int LATCH_PIN = A1;
constexpr int CLOCK_PIN = A0;
constexpr int START_BUTTON = 11;

constexpr int ledPins[BUTTONCOUNT] = { 2, 3, 4, 5 };
constexpr int buttonPins[BUTTONCOUNT] = { 6, 7, 8, 9 };
constexpr const int frequencies[5] = { 121, 1000, 2376, 4000, 5000 };

class IO
{
  bool buttonState[BUTTONCOUNT];
  bool volatileState[BUTTONCOUNT];
  unsigned long debounceTime[BUTTONCOUNT];
  DigitDisplay digit;

public:
  IO()
    : digit(CLOCK_PIN, LATCH_PIN, DATA_PIN)
  {
    pinMode(STATUS_LED, OUTPUT);
    pinMode(MY_TURN, OUTPUT);
    pinMode(YOUR_TURN, OUTPUT);
    pinMode(BUZZER, INPUT);
    pinMode(START_BUTTON, INPUT_PULLUP);
    for (int i = 0; i < BUTTONCOUNT; i++) {
      pinMode(ledPins[i], OUTPUT);
      pinMode(buttonPins[i], INPUT_PULLUP);
      buttonState[i] = 0;
      volatileState[i] = 0;
      debounceTime[i] = 0;
    }
    SeedRandom();
    digit.Blank();
  }
  void SeedRandom()
  {
    // TODO: reads are 10 bits of resolution, so gather at least 3.
    randomSeed(analogRead(A5));
  }
  bool ButtonState(int num) const
  {
    return buttonState[num];
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
  void Buzzer(int num, bool on = true) const
  {
    if (on) {
      pinMode(BUZZER, OUTPUT);
      tone(BUZZER, frequencies[num]);
    } else {
      noTone(BUZZER);
      pinMode(BUZZER, INPUT);
    }
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
      Buzzer(num, false);
    }
    SetLED(num, false);
  }
  void Demo()
  {
    unsigned int c = 0;
    int light = -1;
    do {
      int prev = light;
      do {
        light = random(BUTTONCOUNT);
      } while (light == prev);
      WriteDigit((c++) % 10);
      MyTurn(c % 2);
      SetLED(light);
      delay(1000);
      SetLED(light, false);
    } while (digitalRead(START_BUTTON) != LOW);
  }
  int PollButtons()
  {
    int toggledButton = -1;
    bool value;
    for (int i = 0; i < BUTTONCOUNT; i++) {
      value = digitalRead(buttonPins[i]) == LOW;
      if (value != volatileState[i]) {
        volatileState[i] = value;
        debounceTime[i] = millis();
      } else if (value != buttonState[i] && (millis() - debounceTime[i]) > DEBOUNCE) {
        buttonState[i] = value;
        toggledButton = i;
      }
    }
    return toggledButton;
  }
} io;

class State
{
  byte sequenceLen = 0;
  byte inputPosition = 0;
  int sequence[MAX_SEQ_LEN];
  unsigned long lastInputTime;

  void youWin()
  {
    for (int i = 0; i < 3; i++) {
      for (int i = 0; i < BUTTONCOUNT; i++)
        io.SetLED(i);
      delay(800);
      for (int i = 0; i < BUTTONCOUNT; i++)
        io.SetLED(i, false);
      delay(200);
    }
    restart();
  }
  void youLose()
  {
    io.Buzzer(BUTTONCOUNT);
    delay(800);
    io.Buzzer(BUTTONCOUNT, false);
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
    inputPosition = 0;
    lastInputTime = millis();
    io.MyTurn(false);
  }
public:
  void NewGame()
  {
    io.Demo();
    io.WriteDigit(0);
    sequenceLen = 0;
    addToSequence();
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
  void Process()
  {
    // FIXME: PollButtons could theoretically fail to report a simulataneous button press
    int buttonNum = io.PollButtons();
    if (buttonNum < 0) {
      // nothing pressed: do we need to timeout?
      unsigned long currentTime = millis();
      if (currentTime - lastInputTime >= TIMEOUT)
        youLose();
    } else {
      // a button was pressed or released
      bool value = io.ButtonState(buttonNum);
      if (value) {
        // button pressed down, read the input.
        // FIXME: if a button is pressed while one is still held down, that
        //        should lose the game.
        int correctButton = sequence[inputPosition++];
        if (buttonNum != correctButton)
          youLose();
        else {
          // display confirmation
          io.SetLED(buttonNum);
          io.Buzzer(buttonNum);
          lastInputTime = millis();
        }
      } else {
        // button released, move on.
        io.SetLED(buttonNum, false);
        io.Buzzer(buttonNum, false);
        if (inputPosition == sequenceLen) {
          // passed the test! make the sequence longer
          io.WriteDigit(sequenceLen);
          delay(NEXT_LEVEL_DELAY);
          addToSequence();
        } else {
          lastInputTime = millis();
        }
      }
    }
  }
} state;

void setup() {
  state.NewGame();
}

void loop() {
  state.Process();
}
