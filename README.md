# Zoom MultiStomp Patch Changer

Change the current patch of your Zoom MultiStomp pedal using an Arduino Pro Mini, a USB Host Shield module, and your feet :)

![Demo](img/demo.jpg)

### Usage
- Click on buttons to decrement / increment patch value
- Longpress on buttons to decrement / increment patch value
- Longpress on both buttons to enable / disable tuner


### Bill of materials
- [Arduino Pro Mini (3.3v version)](img/APMv33.jpg)
- [USB Host Shield v2.0](img/UHSv2.jpg)
- [128x32 OLED screen](img/OLED.jpg)
- 2x push buttons or momentary footswitches
- Dual 9v power supply
- USB-A to USB-Mini cable
- FTDI breakout board (configured for 3.3v) for code upload


### Wiring
- The USB Host Shield should be soldered under (or above) the Arduino board; USB-A port facing one side, Arduino programming header facing the other (see pic)
- A4 and A5 pins should be wired to SDA / SCL on the OLED board; power is supplied using Arduino's VCC pin.
- Button pins are wired to analog A1 and A2, thus using internal pullup resistors.
- 9v power is supplied to the Arduino (through the RAW pin), and to the Zoom device.

