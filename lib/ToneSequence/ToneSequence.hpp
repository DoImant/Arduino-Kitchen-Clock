//////////////////////////////////////////////////////////////////////////////
/// \file ToneSequence.hpp
/// \author Kai R. ()
/// \brief Class for non-blocking playback of a sound sequence
///
/// \date 2025-05-17
/// \version 1.0
///
//////////////////////////////////////////////////////////////////////////////

#pragma once

//
// Frequencies of the notes
//
namespace note {
constexpr unsigned int b0 {31};
constexpr unsigned int c1 {33};
constexpr unsigned int cs1 {35};
constexpr unsigned int d1 {37};
constexpr unsigned int ds1 {39};
constexpr unsigned int e1 {41};
constexpr unsigned int f1 {44};
constexpr unsigned int fs1 {46};
constexpr unsigned int g1 {49};
constexpr unsigned int gs1 {52};
constexpr unsigned int a1 {55};
constexpr unsigned int as1 {58};
constexpr unsigned int b1 {62};
constexpr unsigned int c2 {65};
constexpr unsigned int cs2 {69};
constexpr unsigned int d2 {73};
constexpr unsigned int ds2 {78};
constexpr unsigned int e2 {82};
constexpr unsigned int f2 {87};
constexpr unsigned int fs2 {93};
constexpr unsigned int g2 {98};
constexpr unsigned int gs2 {104};
constexpr unsigned int a2 {110};
constexpr unsigned int as2 {117};
constexpr unsigned int b2 {123};
constexpr unsigned int c3 {131};
constexpr unsigned int cs3 {139};
constexpr unsigned int d3 {147};
constexpr unsigned int ds3 {156};
constexpr unsigned int e3 {165};
constexpr unsigned int f3 {175};
constexpr unsigned int fs3 {185};
constexpr unsigned int g3 {196};
constexpr unsigned int gs3 {208};
constexpr unsigned int a3 {220};
constexpr unsigned int as3 {233};
constexpr unsigned int b3 {247};
constexpr unsigned int c4 {262};
constexpr unsigned int cs4 {277};
constexpr unsigned int d4 {294};
constexpr unsigned int ds4 {311};
constexpr unsigned int e4 {330};
constexpr unsigned int f4 {349};
constexpr unsigned int fs4 {370};
constexpr unsigned int g4 {392};
constexpr unsigned int gs4 {415};
constexpr unsigned int a4 {440};
constexpr unsigned int as4 {466};
constexpr unsigned int b4 {494};
constexpr unsigned int c5 {523};
constexpr unsigned int cs5 {554};
constexpr unsigned int d5 {587};
constexpr unsigned int ds5 {622};
constexpr unsigned int e5 {659};
constexpr unsigned int f5 {698};
constexpr unsigned int fs5 {740};
constexpr unsigned int g5 {784};
constexpr unsigned int gs5 {831};
constexpr unsigned int a5 {880};
constexpr unsigned int as5 {932};
constexpr unsigned int b5 {988};
constexpr unsigned int c6 {1047};
constexpr unsigned int cs6 {1109};
constexpr unsigned int d6 {1175};
constexpr unsigned int ds6 {1245};
constexpr unsigned int e6 {1319};
constexpr unsigned int f6 {1397};
constexpr unsigned int fs6 {1480};
constexpr unsigned int g6 {1568};
constexpr unsigned int gs6 {1661};
constexpr unsigned int a6 {1760};
constexpr unsigned int as6 {1865};
constexpr unsigned int b6 {1976};
constexpr unsigned int c7 {2093};
constexpr unsigned int cs7 {2217};
constexpr unsigned int d7 {2349};
constexpr unsigned int ds7 {2489};
constexpr unsigned int e7 {2637};
constexpr unsigned int f7 {2794};
constexpr unsigned int fs7 {2960};
constexpr unsigned int g7 {3136};
constexpr unsigned int gs7 {3322};
constexpr unsigned int a7 {3520};
constexpr unsigned int as7 {3729};
constexpr unsigned int b7 {3951};
constexpr unsigned int c8 {4186};
constexpr unsigned int cs8 {4435};
constexpr unsigned int d8 {4699};
constexpr unsigned int ds8 {4978};
}   // namespace note


using MillisType = decltype(millis());

//////////////////////////////////////////////////////////////////////////////
/// \brief Structure for storing a note value (pitch + duration)
///        To calculate the note duration, take one second divided by the note type.
///        e.g. quarter note = 1000ms / 4, eighth note = 1000/8, etc.
///
//////////////////////////////////////////////////////////////////////////////
struct Note {
  const unsigned int pitch;
  const MillisType duration;
};

//////////////////////////////////////////////////////////////////////////////
/// \brief Class for non-blocking playback of a sound sequence
/// 
/// \tparam pin  where the buzzer is connected to.
//////////////////////////////////////////////////////////////////////////////
template <byte pin> class ToneSequence {
public:
  template <size_t N> bool operator()(const Note (&m)[N]) {
    switch (is_tone_on) {
      case false:
        if (has_finnished) { return false; }
        if (idx >= N) {           // All notes of the sound sequence have been played.
          has_finnished = true;   // Marker indicating that a sound sequence has been played.
          return false;
        }
        timestamp = millis();
        play_duration = m[idx].duration * 1.30;
        tone(pin, m[idx].pitch, m[idx].duration);
        is_tone_on = true;
        [[fallthrough]];
      case true:
        if (millis() - timestamp > play_duration) {
          // noTone(pin);  necessary?
          ++idx;
          is_tone_on = false;
        }
        break;
    }
    return true;
  }
  // If a sound sequence has already been processed, reset() must be called so that another sequence can be played.
  bool reset() {
    idx = 0;
    return !(has_finnished = false);   // returns true
  }
  bool stop() {
    idx = 0;
    return !(has_finnished = true);   // returns false
  }

private:
  bool is_tone_on {false};
  bool has_finnished {false};
  size_t idx {0};
  MillisType play_duration;  // max. 65535
  MillisType timestamp;
};
