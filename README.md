# Drum Machine

A small, battery-powered drum machine for learning embedded C, audio, sequencing, and PCB design. The first prototype should be compact enough to finish while preserving the interaction model of a larger machine.

## Current prototype direction

- ATmega328P in a DIP socket
- Internal oscillator initially
- Eight illuminated buttons
  - Four reusable pad/step buttons
  - Four function buttons whose assignments are intentionally undecided
- Two knobs/potentiometers
- Small 128-pixel I2C screen already on hand
- One shift register controlling eight LEDs
- Synthesized sounds initially; no sampling or microphone yet
- Tiny built-in speaker
- 3.5 mm headphone/audio jack
- Two AAA batteries as the likely normal supply
- A coin cell may be tested as a deliberately tiny, low-volume experiment

Every button must support press, hold, and release events. Button meanings can therefore change by mode, and any button may later act as a modifier.

## Sequencer interface

The sequencer has 16 steps organized as four beats with four sixteenth-note subdivisions per beat. The four pad buttons edit one beat at a time:

```text
Beat 1:  * _ * _
Beat 2:  * * _ _
Beat 3:  _ _ * _
Beat 4:  * _ _ *
```

Together they form the measure:

```text
*_*_  **__  __*_  *__*
```

The screen shows the current beat, voice, tempo, mode, and settings. During playback it can count beats 1 through 4 while the pad LEDs show the subdivisions and playhead.

The same four pad buttons may become voice selectors, live triggers, pattern slots, or parameter controls in other modes. The four function buttons decide what the pad grid currently means.

## Event and sound model

The sequencer should produce MIDI-like note events internally even before physical MIDI exists:

```c
note_on(channel, note, velocity);
note_off(channel, note);
```

Initially these events drive the internal drum synthesizer. Later, the same events could also be sent over USB-C MIDI without redesigning the sequencer.

Planned recording modes include:

- Step entry
- Quantized live recording
- Free recording that preserves timing offsets

Patterns can be stored in the ATmega328P's internal EEPROM. External sample memory is deferred until sampling becomes part of the project.

## Audio direction

The audio path needs to support both a real miniature speaker and headphones:

```text
digital sound engine
        |
        +--> speaker amplifier --> tiny speaker
        |
        +--> headphone amplifier --> switched 3.5 mm jack
```

A line-level output can omit the headphone amplifier, but ordinary headphones need a headphone driver. Bridge-tied speaker-amplifier outputs must not be connected to a common-ground headphone jack.

MAX98357 modules are already on hand. They combine an I2S DAC and Class-D speaker amplifier and can be used for experimentation, although they do not provide headphone output. A final PCB may instead use a bare combined speaker/headphone amplifier IC to minimize size.

Candidate speakers:

- 11 x 15 mm, 8-ohm, 1 W sugar-cube speaker with a small sealed chamber
- 16 mm, 8-ohm, 1 W cavity speaker with its enclosure already attached

Raw speakers should be mounted by their rigid outer frame, never by the cone. A sealed rear chamber or integrated cavity greatly improves their sound.

## Parts already available

- ATmega328P
- DIP sockets
- Buttons
- Diodes
- Two or three potentiometers
- Small I2C screens
- Two shift registers
- Through-hole LEDs
- Perfboard
- Raw 28 mm speakers
- MAX98357 amplifier modules
- ISP programming setup

Only one shift register is required for the eight prototype LEDs. The second remains available for expansion.

## Parts still needed or undecided

- Tiny enclosed/cavity speaker
- Switched 3.5 mm audio jack
- Headphone-amplifier solution
- Two-AAA holder or rechargeable power system
- Power switch
- LED current-limiting resistors
- Decoupling and audio-filter capacitors
- Final display
- Final amplifier IC
- External memory and microphone if sampling is added
- USB-C MIDI hardware if MIDI output is added

## Software structure

The pattern-game repository is a useful reference for non-blocking timers, state machines, input events, and player/update APIs. The drum machine should follow the same general style while keeping hardware drivers separate from musical state.

Likely modules:

```text
main.c
timer.c
buttons.c
leds.c
controls.c
display.c
sequencer.c
midi.c
synth.c
audio.c
storage.c
app.c
```

The audio sample generation may run from a timer interrupt while controls, display updates, and application modes remain non-blocking in the main loop.

The button state should preserve:

- Current pressed mask
- New press events
- Release events
- Hold events
- Press timestamp or duration for every button

## Build order

1. Implement and test the 16-step sequencer model on the computer.
2. Bring up the ATmega timer and non-blocking main loop.
3. Read all eight buttons and distinguish press, hold, and release.
4. Drive the eight LEDs through one shift register.
5. Display beat, mode, and tempo on the temporary I2C screen.
6. Generate MIDI-like events and route them to a simple synthesized voice.
7. Test speaker audio using an amplifier already on hand.
8. Add pattern storage in EEPROM.
9. Test two-AAA and experimental coin-cell power.
10. Choose the final speaker, jack, amplifier, display, and PCB layout.

## Open interface decisions

- Exact jobs of the four function buttons
- Whether either knob is permanently assigned to volume or tempo
- How beat/page selection works
- Voice count and voice-selection workflow
- Pattern save/load workflow
- Quantized versus free-record behavior
- Speaker mute and headphone insertion behavior
- Final battery chemistry
- Whether the final version includes samples, MIDI, or both
