# Preset firmware hardware hookup

This guide is for the 28-pin DIP ATmega328P and the `atmega_preset.hex`
firmware. The firmware boots into a hard-coded pattern and outputs PWM audio.
It does not use buttons, LEDs, a screen, or the shift register yet.

## ATmega328P 28-pin DIP

| DIP pin | ATmega signal | Preset use |
| ---: | --- | --- |
| 1 | PC6 / RESET | ISP reset |
| 2 | PD0 / RXD | spare |
| 3 | PD1 / TXD | spare |
| 4 | PD2 / INT0 | spare |
| 5 | PD3 / OC2B | PWM audio output |
| 6 | PD4 | spare |
| 7 | VCC | +5 V supply |
| 8 | GND | ground |
| 9 | PB6 / XTAL1 | leave for internal oscillator |
| 10 | PB7 / XTAL2 | leave for internal oscillator |
| 11 | PD5 / OC0B | spare |
| 12 | PD6 / OC0A | spare |
| 13 | PD7 | spare |
| 14 | PB0 | spare |
| 15 | PB1 / OC1A | spare |
| 16 | PB2 / OC1B | spare |
| 17 | PB3 / MOSI | ISP MOSI |
| 18 | PB4 / MISO | ISP MISO |
| 19 | PB5 / SCK | ISP clock |
| 20 | AVCC | +5 V supply |
| 21 | AREF | 100 nF to ground if using ADC later |
| 22 | GND | ground |
| 23 | PC0 / ADC0 | spare |
| 24 | PC1 / ADC1 | spare |
| 25 | PC2 / ADC2 | spare |
| 26 | PC3 / ADC3 | spare |
| 27 | PC4 / SDA | spare |
| 28 | PC5 / SCL | spare |

Connect a 100 nF ceramic bypass capacitor between pins 7 and 8, and another
between pins 20 and 22. Tie AVCC to the positive supply even when the ADC is
not being used. Keep RESET pulled up to VCC with about 10 kΩ unless the ISP
programmer board already provides that resistor.

The firmware assumes the ATmega is running at 8 MHz from its internal
oscillator. Do not add an external crystal for this first test.

## Preset PWM audio path

The firmware outputs 8-bit PWM on `PD3/OC2B`, DIP pin 5. Use this path with a
small audio amplifier that accepts an analog or PWM-derived signal:

```text
ATmega pin 5 (PD3) → 1 kΩ → audio node → amplifier input
                                      ↓
                                100 nF → GND
amplifier output → speaker
ATmega GND      → amplifier GND
```

The RC values are a starting point, not a finished audio filter. Do not drive
a speaker directly from the ATmega pin.

## MAX98357A module

A MAX98357A breakout normally exposes these labels:

| Module label | Connect to |
| --- | --- |
| VIN/VCC | 2.5–5.5 V supply, according to the breakout documentation |
| GND | common ground |
| DIN | I²S serial data from an audio source |
| BCLK/BCK | I²S bit clock from the audio source |
| LRC/LRCLK/WS | I²S left/right or frame clock from the audio source |
| SD/SD_MODE | enable/shutdown and channel selection; follow the module documentation |
| GAIN/GAIN_SLOT | gain selection; leave or strap as the module documentation says |
| SPK+ / OUT+ | one speaker terminal |
| SPK- / OUT- | the other speaker terminal |

The MAX98357A is an I²S PCM amplifier. Its `SPK+` and `SPK-` outputs are a
bridge-tied pair. Connect the speaker between them and do not connect either
speaker output to ground. The ATmega preset firmware does not generate I²S,
so do not connect its PWM pin to `DIN` and expect sound from this amplifier.

For the first preset test, use the PWM path with a suitable analog-input
amplifier. Use the MAX98357A later with an I²S-capable controller or after we
add and test an I²S output implementation.

## SN74HC595N

`SN74HC595N` is an 8-bit serial-in, parallel-out shift register in a 16-pin
PDIP package. The `N` identifies the PDIP package. It provides eight outputs
for LEDs or other digital loads. It is not used by the preset firmware.

The usual control connections are:

```text
SER   ← ATmega data
SRCLK ← ATmega clock
RCLK  ← ATmega latch
OE    → GND to enable outputs
SRCLR → VCC to keep clear inactive
Q0–Q7 → LED circuits, each with its own resistor
VCC   → supply
GND   → common ground
```

Check the notch and the chip datasheet before wiring. Never leave the CMOS
control inputs floating.
