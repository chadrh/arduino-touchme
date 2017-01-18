#include "Arduino.h"

constexpr int BUTTONCOUNT = 4;
constexpr int MAX_SEQ_LEN = 5;
constexpr int TIMEOUT = 4000;
constexpr int DEBOUNCE = 40;
constexpr int STATUS_LED = 13;
constexpr int BUZZER = 10;

constexpr int ledPins[] = { 2, 3, 4, 5 };
constexpr int buttonPins[] = { 6, 7, 8, 9 };
constexpr int frequencies[] = { 121, 1000, 2376, 10000 };

class IO
{
  bool buttonState[BUTTONCOUNT];
  bool volatileState[BUTTONCOUNT];
  unsigned long debounceTime[BUTTONCOUNT];
public:
  void Setup()
  {
    pinMode(STATUS_LED, OUTPUT);
    pinMode(BUZZER, INPUT);
    for (int i = 0; i < BUTTONCOUNT; i++) {
      pinMode(ledPins[i], OUTPUT);
      pinMode(buttonPins[i], INPUT_PULLUP);
      buttonState[i] = 0;
      volatileState[i] = 0;
      debounceTime[i] = 0;
    }
    SeedRandom();
  }
  void SeedRandom()
  {
    // TODO: reads are 10 bits of resolution, so gather at least 3.
    randomSeed(analogRead(A0));
  }
  bool ButtonState(int num) const
  {
    return buttonState[num];
  }
  void SetLED(int num, bool on = true) const
  {
    digitalWrite(ledPins[num], on ? HIGH : LOW);
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
    // TODO: blink & buzz every light
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
    // FIXME: this should be a different tone.
    io.Buzzer(3);
    delay(1000);
    io.Buzzer(3, false);
    restart();
  }
  void restart()
  {
    delay(2000);
    NewGame();
  }
  void addToSequence()
  {
    if (sequenceLen == MAX_SEQ_LEN) {
      youWin();
      return;
    }

    sequence[sequenceLen++] = random(BUTTONCOUNT);
    DisplaySequence();
    inputPosition = 0;
    lastInputTime = millis();
  }
public:
  void NewGame()
  {
    sequenceLen = 0;
    addToSequence();
  }
  void DisplaySequence()
  {
    for (byte i = 0; i < sequenceLen; i++) {
      int num = sequence[i];
      io.Blink(num, 500);
      if (i < sequenceLen - 1)
        delay(500);
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
          delay(1000);
          addToSequence();
        } else {
          lastInputTime = millis();
        }
      }
    }
  }
} state;

void setup() {
  io.Setup();
  state.NewGame();
}

void loop() {
  state.Process();
}
