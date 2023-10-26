//////////////////////////////////////////////////////////////////////////////
/// @file main.cpp
/// @author Kai R. ()
/// @brief Kitchen clock control
///
/// @date 2023-06-10
/// @version 1.0.0
///
/// @copyright Copyright (c) 2023
///
//////////////////////////////////////////////////////////////////////////////
#include <avr/sleep.h>
#include <Arduino.h>
#include <U8g2lib.h>
#include <RotaryEncoder.h>
#include "Button_SL.hpp"
#include "KitchenTimer.hpp"
#include "AlarmTone.hpp"

// #define DISPLAY_Y32       // Remove the comment if the display has only 32 instead of 64 pixel lines
// #define MINUTES_DEFAULT   // Remove the comment if you want the time setting to start with the minutes.

//
// gobal constants
//

// If the time is running ahead or behind, the inaccuracy of the oscillator can be compensated
// somewhat via this "SECOND" value.
constexpr uint16_t SECOND {997};   // 1000ms = 1 Second
constexpr uint16_t TIMEOUT {10000};

constexpr uint8_t BUFFERLENGTH {6};   // 5 characters + end-of-string character '\0'.
constexpr uint8_t DISPLAY_MAX_X {128};
#ifndef DISPLAY_Y32
constexpr uint8_t DISPLAY_MAX_Y {64};
#else
constexpr uint8_t DISPLAY_MAX_Y {32};
#endif

// Font u8g2_font_freedoomr25_mn   // 19 Width 26 Height
constexpr uint8_t FONT_WIDTH {24};
constexpr uint8_t FONT_HIGHT {51};

// The following display values are calculated from the upper four values. No change necessary.
constexpr uint8_t DISPLAY_X {(DISPLAY_MAX_X - FONT_WIDTH * (BUFFERLENGTH - 1)) / 2};   // Column = X Coordinate
constexpr uint8_t DISPLAY_Y {(DISPLAY_MAX_Y + FONT_WIDTH) / 2};                        // Row = Y coordinate
constexpr uint8_t MINUTES_LINE_X {DISPLAY_X};   // LINE = Coordinates for the line under minute and second digits
constexpr uint8_t SECONDS_LINE_X {DISPLAY_X + FONT_WIDTH * 3};
constexpr uint8_t LINE_Y {DISPLAY_Y + 2};        // Line below the numbers
constexpr uint8_t LINE_WIDTH {FONT_WIDTH * 2};   // Line length = font width * 2

#if defined(__AVR_ATtiny1604__)
constexpr uint8_t PIN_BTN {0};     // SW on rotary encoder
constexpr uint8_t PIN_IN1 {1};     // DT   ---- " ----
constexpr uint8_t PIN_IN2 {2};     // CLK  ---- " ----
constexpr uint8_t PIN_ALARM {3};   // Buzzer
#else
constexpr uint8_t PIN_BTN {3};      // SW on rotary encoder
constexpr uint8_t PIN_IN1 {4};      // DT   ---- " ----
constexpr uint8_t PIN_IN2 {5};      // CLK  ---- " ----
constexpr uint8_t PIN_ALARM {13};   // Buzzer
#endif

constexpr uint16_t NOTE_F6 {1397};
constexpr uint16_t NOTE_A6 {1760};

//
// Global objects / variables
//

struct InputState {
  enum class state : uint8_t { seconds = 0, minutes };
  RotaryEncoder encoder {PIN_IN1, PIN_IN2, RotaryEncoder::LatchMode::FOUR3};
#ifndef MINUTES_DEFAULT
  const state defaultState {state::seconds};
  state lastState {state::minutes};
#else
  const state defaultState {state::minutes};
  state lastState {state::seconds};
#endif
  state currentState {defaultState};
} input;

#ifndef DISPLAY_Y32
U8G2_SSD1306_128X64_NONAME_2_HW_I2C u8g2(U8G2_R0);
#else
U8G2_SSD1306_128X32_UNIVISION_2_HW_I2C u8g2(U8G2_R0);
#endif

enum class Underline : byte { no, yes };

using namespace Btn;
ButtonSL btn {PIN_BTN};
TimerHelper wait;   // This class is defined in AlarmTone.hpp
KitchenTimer ktTimer;
AlarmTone alarm {PIN_ALARM};

//
// Forward declaration function(s).
//
void intWakeup();
void powerDown(uint8_t wakeupPin);
KitchenTimerState runTimer(KitchenTimer &);
bool askEncoder(RotaryEncoder &, KitchenTimer &);
bool processInput(KitchenTimer &, InputState &);
void displayTime(KitchenTimer &, Underline);
void setDisplayForInput(KitchenTimer &kT, InputState &iS);
void askRtButton(ButtonSL &, KitchenTimer &, InputState &);

//////////////////////////////////////////////////////////////////////////////
/// @brief Initialization part of the main program
///
//////////////////////////////////////////////////////////////////////////////
void setup(void) {
  // Serial.begin(115200);
#if defined(__AVR_ATtiny1604__)
  // Turn on all the pullups for minimal power in sleep
  PORTA.DIR = 0;   // All PORTA pins inputs
  for (uint8_t pin = 0; pin < 8; ++pin) { (&PORTA.PIN0CTRL)[pin] = PORT_PULLUPEN_bm; }
  PORTB.DIR = 0;   // All PORTB pins inputs
  for (uint8_t pin = 0; pin < 4; ++pin) { (&PORTB.PIN0CTRL)[pin] = PORT_PULLUPEN_bm; }

  // ADC is not required so switch it off
  ADC0.CTRLA &= ~ADC_ENABLE_bm;
#else
  bitClear(ADCSRA, ADEN);
#endif

  u8g2.begin();
  u8g2.setFont(u8g2_font_logisoso42_tn);   // 24 Width 51 Hight

  btn.begin();
  btn.releaseOn();
  btn.setDebounceTime_ms(100);

  set_sleep_mode(SLEEP_MODE_PWR_DOWN);   // Set sleep mode to POWER DOWN mode
  sleep_enable();                        // Enable sleep mode, but not yet
}

//////////////////////////////////////////////////////////////////////////////
/// @brief main program
///
//////////////////////////////////////////////////////////////////////////////
void loop() {
  KitchenTimerState ktState {ktTimer.getState()};
  switch (ktState) {
    case KitchenTimerState::active: runTimer(ktTimer); break;
    case KitchenTimerState::off:
      if (processInput(ktTimer, input)) {
        wait.start();
      } else {
        if (wait(TIMEOUT)) {
          pinMode(PIN_ALARM, OUTPUT);   // Saves power
          powerDown(PIN_BTN);
          wait.start();   // Start timer so that the display does not go off immediately after wake up
          delay(1000);    // A delay so that the minute/second changeover is not triggered immediately after waking up.
        }
      }
      break;
    case KitchenTimerState::alarm:
      alarm.playAlarm();
      if (btn.tick() != ButtonState::notPressed) {   // Switch alarm off with encoder button
        setDisplayForInput(ktTimer, input);
      }
      if (askEncoder(input.encoder, ktTimer)) {   // Switch alarm off with encoder rotation
        ktTimer.setSeconds(0);                    // Reset count from rotation
        setDisplayForInput(ktTimer, input);
      }
      wait.start();   // Start timer so that the display does not go off immediately after the alarm is turned off.
      break;
  }
  // If the alarm is active, only the encoder query in the switch instruction may be active.
  if (ktState != KitchenTimerState::alarm) { askRtButton(btn, ktTimer, input); }
}

//////////////////////////////////////////////////////////////////////////////
/// @brief Interrupt service routine for wake up
///
//////////////////////////////////////////////////////////////////////////////
void intWakeup() { detachInterrupt(PIN_BTN); }

//////////////////////////////////////////////////////////////////////////////
/// @brief  Start (and stop) the sleep mode
///
/// @param wakeupPin     Number of the wake up interrupt pin
//////////////////////////////////////////////////////////////////////////////
void powerDown(uint8_t wakeupPin) {
  // go into deep Sleep
#if defined(__AVR_ATtiny1604__)
  attachInterrupt(digitalPinToInterrupt(wakeupPin), intWakeup, LOW);
#else
  attachInterrupt(digitalPinToInterrupt(wakeupPin), intWakeup, FALLING);
#endif
  u8g2.setPowerSave(true);
  delay(20);
  sleep_cpu();   // sleep
  // switch anything on
  delay(20);
  u8g2.setPowerSave(false);
}

//////////////////////////////////////////////////////////////////////////////
/// @brief The set time is continuously counted down
///        by 1 per second until the value is 0.
///
/// @param kT Reference on kitchen timer object
/// @return KitchenTimerState
//////////////////////////////////////////////////////////////////////////////
KitchenTimerState runTimer(KitchenTimer &kT) {
  if (kT(SECOND)) {
    --kT;
    switch (kT.timeIsUp()) {
      case false: kT.start(); break;
      case true: kT.setState(KitchenTimerState::alarm); break;
    }
    displayTime(kT, Underline::no);
  }
  return kT.getState();
}

//////////////////////////////////////////////////////////////////////////////
/// @brief The encoder signals are evaluated
///
/// @param enc Reference on encoder object
/// @param kT  Reference on kitchen timer object
/// @return true  if the an encoder signal was evaluated
/// @return false if no encoder signal was evaluated
//////////////////////////////////////////////////////////////////////////////
bool askEncoder(RotaryEncoder &enc, KitchenTimer &kT) {
  uint8_t flag {true};
  enc.tick();
  switch (enc.getDirection()) {
    case RotaryEncoder::Direction::NOROTATION: flag = false; break;
    case RotaryEncoder::Direction::CLOCKWISE: ++kT; break;
    case RotaryEncoder::Direction::COUNTERCLOCKWISE: --kT; break;
  }
  return flag;
}

//////////////////////////////////////////////////////////////////////////////
/// @brief Control the inputs and set the input states
///
/// @param kT Reference on kitchen timer object
/// @param iS Reference on input state structure
/// @return true when the encoder has been actuated
/// @return false if no encoder operation has occurred
//////////////////////////////////////////////////////////////////////////////
bool processInput(KitchenTimer &kT, InputState &iS) {
  bool encoderActuated {true};
  if (iS.lastState != iS.currentState) {
    switch (iS.currentState) {
      case InputState::state::seconds: kT.setUnitSeconds(); break;
      case InputState::state::minutes: kT.setUnitMinutes(); break;
    }
    iS.lastState = iS.currentState;
    displayTime(kT, Underline::yes);
  } else if (askEncoder(iS.encoder, ktTimer)) {
    switch (ktTimer.getState()) {
      case KitchenTimerState::alarm: ktTimer.setState(KitchenTimerState::off); break;
      default: displayTime(kT, Underline::yes); break;
    }
  } else {
    encoderActuated = false;
  }
  return encoderActuated;
}

//////////////////////////////////////////////////////////////////////////////
/// @brief Set the correct input status for the display indication
///
/// @param kT Reference on kitchen timer object
/// @param iS Reference on input state structure
//////////////////////////////////////////////////////////////////////////////
void setDisplayForInput(KitchenTimer &kT, InputState &iS) {
  ktTimer.setState(KitchenTimerState::off);
  iS.lastState =
      (iS.defaultState == InputState::state::seconds) ? InputState::state::minutes : InputState::state::seconds;
  iS.currentState = iS.defaultState;
}

//////////////////////////////////////////////////////////////////////////////
/// @brief Write the two time units into a string and output the string
///        on the display.
///
/// @param kT Reference on kitchen timer object
/// @param underline If Underline::yes, a line will be displayed under the digits 
///                  active for the input. If "no", then no line is displayed.
//////////////////////////////////////////////////////////////////////////////
void displayTime(KitchenTimer &kT, Underline underline) {
  char charBuffer[BUFFERLENGTH];
  sprintf(charBuffer, "%02d:%02d", kT.getMinutes(), kT.getSeconds());
  u8g2.firstPage();
  do {
    u8g2.drawStr(DISPLAY_X, DISPLAY_Y, charBuffer);   // Output string on the display.
    if (underline == Underline::yes) {
      if (kT.getActiveUnit() == ActiveUnit::seconds) {
        u8g2.drawHLine(SECONDS_LINE_X, LINE_Y, LINE_WIDTH);
      } else {
        u8g2.drawHLine(MINUTES_LINE_X, LINE_Y, LINE_WIDTH);
      }
    }
  } while (u8g2.nextPage());
}

//////////////////////////////////////////////////////////////////////////////
/// @brief Query of the encoder's wakeupPin function
///
/// @param b Reference on wakeupPin object
/// @param kT Reference on kitchen timer object
/// @param iS Reference on input state structure
//////////////////////////////////////////////////////////////////////////////
void askRtButton(ButtonSL &b, KitchenTimer &kT, InputState &iS) {
  switch (b.tick()) {
    // If the wakeupPin is pressed for a long time, it switches between timer active and timer off.
    case ButtonState::notPressed: break;
    case ButtonState::longPressed:
      if (!kT.timeIsUp()) {   // Switch on timer only if a time iS set.
        tone(PIN_ALARM, NOTE_A6, 30);
        switch (kT.getState()) {
          case KitchenTimerState::active: setDisplayForInput(ktTimer, input); break;
          case KitchenTimerState::off:
            kT.setState(KitchenTimerState::active);
            kT.setUnitSeconds();
            displayTime(kT, Underline::no);   // Delete underline
            kT.start();                       // Start the countdown
            break;
          case KitchenTimerState::alarm: break;
        }
      }
      break;
    case ButtonState::shortPressed:
      if (kT.getState() == KitchenTimerState::active) { break; }
      switch (iS.currentState) {
        case InputState::state::seconds:
          tone(PIN_ALARM, NOTE_F6, 30);
          iS.currentState = InputState::state::minutes;
          break;
        case InputState::state::minutes:
          tone(PIN_ALARM, NOTE_F6, 30);
          iS.currentState = InputState::state::seconds;
          break;
        default: break;
      }   // inner switch
      break;
    default: break;
  }
}
