#ifndef KITCHENTIMER_HPP
#define KITCHENTIMER_HPP

#include <Arduino.h>
constexpr uint8_t TIMEUNIT_SEC{1};
constexpr uint8_t TIMEUNIT_MIN{60};
constexpr uint8_t MAX_SECONDS{59};
constexpr uint8_t MAX_MINUTES{60};
constexpr int MAX_TOTALSECONDS{MAX_MINUTES * TIMEUNIT_MIN};

enum class KitchenTimerState : uint8_t { off = 0, active, alarm };
enum class ActiveUnit : uint8_t { seconds, minutes };

class KitchenTimer {
public:
  KitchenTimer(size_t m = 0, size_t s = 0) {
    setMinutes(m);
    setSeconds(s);
  }

  KitchenTimer &operator--();
  KitchenTimer const operator--(int);
  KitchenTimer &operator++();
  KitchenTimer const operator++(int);
  boolean operator()(const uint32_t duration) { return (millis() - timeStamp >= duration) ? true : false; }

  void start() { timeStamp = millis(); }
  size_t getMinutes() const { return totalSeconds / TIMEUNIT_MIN; }
  size_t getSeconds() const { return totalSeconds % TIMEUNIT_MIN; }
  void setMinutes(size_t m);
  void setSeconds(size_t s);
  void setUnitSeconds() {
    multiplier = TIMEUNIT_SEC;
    activeUnit = ActiveUnit::seconds;
  }
  void setUnitMinutes() {
    multiplier = TIMEUNIT_MIN;
    activeUnit = ActiveUnit::minutes;
  }
  bool timeIsUp() const { return !(totalSeconds); }

  void setState(KitchenTimerState s) { state = s; }
  KitchenTimerState getState() const { return state; }
  ActiveUnit getActiveUnit() const { return activeUnit; }

private:
  uint32_t totalSeconds{0};
  uint32_t timeStamp{0};
  size_t multiplier{TIMEUNIT_SEC};
  KitchenTimerState state{KitchenTimerState::off};
  ActiveUnit activeUnit{ActiveUnit::seconds};
};

void KitchenTimer::setMinutes(size_t m) {
  totalSeconds = totalSeconds % TIMEUNIT_MIN;

  uint16_t minutes = ((m > MAX_MINUTES) ? MAX_MINUTES : TIMEUNIT_MIN) * m;
  totalSeconds = totalSeconds + minutes;
  if (totalSeconds > MAX_TOTALSECONDS) { totalSeconds = MAX_TOTALSECONDS; }
}

void KitchenTimer::setSeconds(size_t s) {
  totalSeconds = (totalSeconds / TIMEUNIT_MIN) * TIMEUNIT_MIN;
  totalSeconds = totalSeconds + ((s > MAX_SECONDS) ? MAX_SECONDS : (totalSeconds == MAX_TOTALSECONDS) ? 0 : s);
}

// pre decrement (--x)
KitchenTimer &KitchenTimer::operator--() {
  auto save = totalSeconds;
  totalSeconds = totalSeconds - (1 * multiplier);
  // Unsigned overflow if negative! (thats why ">" and not "<")
  if (totalSeconds > MAX_TOTALSECONDS) { totalSeconds = (multiplier == TIMEUNIT_SEC) ? 0 : save % TIMEUNIT_MIN; }
  return *this;
}

// post decrement (x--)
KitchenTimer const KitchenTimer::operator--(int) {
  KitchenTimer temp{*this};
  operator--();
  return temp;
}

// pre increment (++x)
KitchenTimer &KitchenTimer::operator++() {
  totalSeconds = totalSeconds + (1 * multiplier);
  if (totalSeconds > MAX_TOTALSECONDS) { totalSeconds = MAX_TOTALSECONDS; }
  return *this;
}

// post increment (x++)
KitchenTimer const KitchenTimer::operator++(int) {
  KitchenTimer temp{*this};
  operator++();
  return temp;
}
#endif