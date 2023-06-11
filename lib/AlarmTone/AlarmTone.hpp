//////////////////////////////////////////////////////////////////////////////
/// @file AlarmTone.hpp
/// @author Kai R.
/// @brief Class for playing a two-tone alarm tone (active buzzer).
///        The class works with millis() and is non-blocking.
///
/// @date 2022-08-06
/// @version 0.1
///
/// @copyright Copyright (c) 2022
///
//////////////////////////////////////////////////////////////////////////////

#ifndef _ALARM_TONE_
#define _ALARM_TONE_

#include <Arduino.h>

class TimerHelper
{
  public:
    void start() {timeStamp = millis();}
    boolean operator() (const uint32_t duration) {
      return (millis() - timeStamp >= duration) ?  true : false;
    }
  private:
    uint32_t timeStamp {0};
};

constexpr uint16_t ALARM_DELAY_MS{1500};
constexpr uint16_t NOTE_F5{698};
constexpr uint16_t NOTE_A5{880};
constexpr uint8_t MAX_TONES{2};

class AlarmTone {
public:
  AlarmTone(uint8_t pin_, uint16_t toneA = NOTE_F5, uint16_t toneB = NOTE_A5) : pin(pin_), tones{toneA, toneB} {}
  void playAlarm(void);

private:
  void play(void);
  void reset(void);

  uint8_t pin;
  uint8_t pauseBetweenNotes{10};
  uint16_t toneDuration{150};
  uint16_t tones[MAX_TONES];
  uint8_t tonesIndex{0};
  TimerHelper toneTimer;
  TimerHelper delayTimer;
  bool outputTone{false};
  bool alarmFinished{false};
};

//----------------- private methods -----------------

//////////////////////////////////////////////////////////////////////////////
/// @brief Manages the playback of the two-tone sequence
///
//////////////////////////////////////////////////////////////////////////////

void AlarmTone::play() {
  switch (alarmFinished) {
    case true: break;
    case false:
      switch (outputTone) {
        case true:
          if (toneTimer(toneDuration)) {
            noTone(pin);
            outputTone = false;
          }
          break;
        case false:
          if (toneTimer(pauseBetweenNotes)) {
            tone(pin, tones[tonesIndex], toneDuration);
            outputTone = true;
            ++tonesIndex;
            if (tonesIndex > (MAX_TONES - 1)) {
              alarmFinished = true;
              tonesIndex = 0;
            }
            toneTimer.start();   // <- Start timer
          }
      }   // inner switch
      break;
  }   // outer switch
}

//////////////////////////////////////////////////////////////////////////////
/// @brief Necessary call if the alarm tone sequence is to be played
///        more than once.
///
//////////////////////////////////////////////////////////////////////////////
void AlarmTone::reset() { outputTone = alarmFinished = false; }

//----------------- public methods -----------------

//////////////////////////////////////////////////////////////////////////////
/// @brief Completes the playback every "ALARM_DELAY_MS" seconds.
///
//////////////////////////////////////////////////////////////////////////////

void AlarmTone::playAlarm() {
  if (delayTimer(ALARM_DELAY_MS)) {
    reset();
    delayTimer.start();
  }
  play();
}

#endif