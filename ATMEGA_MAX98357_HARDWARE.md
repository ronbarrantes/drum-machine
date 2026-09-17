# ATmega328P to MAX98357A preset hookup

This guide is for `atmega_max_preset.c`. It synthesizes a four-voice,
funky-drummer-inspired pattern at about 10% digital full scale and blinks a
status LED every 500 ms.

The firmware uses the USART in Master SPI mode for uninterrupted serial data
and bit clock output. Timer1 produces the left/right clock in hardware. This
keeps exactly 32 BCLK cycles in each stereo frame while the main loop renders
the next sample.

## Verified clock measurements

The breadboard prototype produced sound with these measured values:

- BCLK: approximately 506 kHz
- LRC: approximately 15.72 kHz
- BCLK-to-LRC ratio: 32:1

The earlier ordinary-SPI experiment paused BCLK between software writes and
produced LRC near 14.5 kHz. The MAX98357A did not reliably accept that stream.

## ATmega328P 28-pin DIP

| DIP pin | ATmega signal | Connect to |
| ---: | --- | --- |
| 1 | PC6 / RESET | ISP reset; 10 kΩ pull-up to +5 V |
| 2 | PD0 / RXD | spare |
| 3 | PD1 / TXD | MAX98357A `DIN` |
| 4 | PD2 / INT0 | spare |
| 5 | PD3 / OC2B | spare |
| 6 | PD4 / XCK | MAX98357A `BCLK` |
| 7 | VCC | +5 V |
| 8 | GND | common ground |
| 9 | PB6 / XTAL1 | leave open for the internal oscillator |
| 10 | PB7 / XTAL2 | leave open for the internal oscillator |
| 11 | PD5 / OC0B | spare |
| 12 | PD6 / OC0A | spare |
| 13 | PD7 | spare |
| 14 | PB0 | LED anode through 220–1,000 Ω; cathode to GND |
| 15 | PB1 / OC1A | MAX98357A `LRC`, `LRCLK`, or `WS` |
| 16 | PB2 / SS | spare |
| 17 | PB3 / MOSI | ISP MOSI; spare after programming |
| 18 | PB4 / MISO | ISP MISO |
| 19 | PB5 / SCK | ISP clock; spare after programming |
| 20 | AVCC | +5 V |
| 21 | AREF | leave open for this firmware |
| 22 | GND | common ground |
| 23–26 | PC0–PC3 | spare |
| 27 | PC4 / SDA | spare |
| 28 | PC5 / SCL | spare |

Connect a 100 nF ceramic capacitor between pins 7 and 8 and another between
pins 20 and 22. Tie AVCC to +5 V even though this firmware does not use the
ADC. Both ATmega ground pins and the MAX module must share the same ground.

## MAX98357A breakout module

| Module label | Connect to |
| --- | --- |
| `VIN` or `VCC` | +5 V |
| `GND` | common ground |
| `DIN` | ATmega DIP pin 3, `PD1/TXD` |
| `BCLK` or `BCK` | ATmega DIP pin 6, `PD4/XCK` |
| `LRC`, `LRCLK`, or `WS` | ATmega DIP pin 15, `PB1/OC1A` |
| `SD`, `SD_MODE`, or `EN` | +5 V for always enabled, left-channel mode |
| `GAIN` or `GAIN_SLOT` | leave floating for the module's default gain |
| `SPK+` or `OUT+` | one speaker terminal |
| `SPK-` or `OUT-` | the other speaker terminal |

Connect the speaker only between `SPK+` and `SPK-`. Neither speaker terminal
connects to ground. Reversing the two speaker wires only reverses polarity and
does not hurt a single-speaker prototype.

Module pin order varies. Follow the labels printed on the board rather than
assuming a physical header order.

## Flashing

With the UNO R4 running ArduinoISP:

```sh
./flash.sh atmega_max_preset.c
```

The script detects the serial port, compiles the source, writes the program,
and verifies flash contents. If multiple serial ports are present:

```sh
ATMEGA_PORT=/dev/cu.usbmodem1401 ./flash.sh atmega_max_preset.c
```

The ATmega must use its internal oscillator without the factory divide-by-8
fuse. Set the low fuse once on a fresh chip:

```sh
avrdude -p atmega328p -c stk500v1 -P /dev/cu.usbmodem1401 -b 19200 \
  -U lfuse:w:0xE2:m
```

`flash.sh` does not alter fuses. The firmware makes a small runtime `OSCCAL`
adjustment so the internal oscillator produces clocks close to the supported
16 kHz MAX98357A sample rate.

## Scope safety

For DIN, BCLK, and LRC, connect the probe ground clip to circuit ground. For
the differential speaker output, connect both channel ground clips to circuit
ground, probe `SPK+` and `SPK-` separately, and use channel subtraction if
needed. Never connect a probe ground clip to either speaker output.

## SN74HC595N

The shift register is not used by this preset. Leave it disconnected until
the LED and control hardware is added.
