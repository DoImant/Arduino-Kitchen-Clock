# Kitchen Timer

This program is a kitchen clock control. The time is displayed on an OLED display (128x64px or 128x32px). The time is set by a so-called rotary encoder.

Turning the encoder sets minutes and seconds. Switching between the two time units is done by a short press on the encoder button. A long press starts the countdown. 60 minutes is the maximum time span that can be set.

When the time has elapsed, an alarm sounds. By short pressure on the encoder button, or by turning, the alarm tone is switched off again. 

If no input is made at the clock, the circuit is put into a sleep mode to save power. The power consumption in sleep mode is about 7.6µA. To end this, a short press on the encoder button is also sufficient.

The program in principle runs on contoller boards with an ATMega328 chip and on ATtinys from the tinyAVR series with more than 14kb Flash and 800 bytes RAM.

This version is customized to an ATtiny 1604.

## Circuit diagram

[Sheet](https://github.com/DoImant/Arduino-Kitchen-Clock/blob/main/docu/kitchen_clock.pdf)

Breadboard view\
![breadboard circuit](https://github.com/DoImant/Arduino-Kitchen-Clock/blob/main/docu/kitchen_clock.jpg?raw=true)