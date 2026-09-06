# Drum Machine UI Ideas

Potential control scheme for the first card-sized drum machine. This is a working design, not a fixed specification.

The current physical concept is:

- 4 musical buttons with LEDs
- 4 function buttons with LEDs
- 1 dedicated master-volume knob
- 1 general-purpose control knob/encoder
- small OLED display

The main UI idea is borrowed from the way the Teenage Engineering PO-12 gets many functions from a small number of controls: function buttons can act as modifiers that temporarily change what the musical buttons and control knob do.

## Proposed function buttons

| Button | Tap | Hold + musical buttons | Hold + control knob |
| --- | --- | --- | --- |
| PLAY | Play / stop | Performance functions later | Tempo or playback parameter |
| SOUND | Enter/select sound mode | Select instrument 1–4 | Edit/select sound parameter |
| PATTERN | Enter/select pattern mode | Select pattern 1–4 | Browse patterns/banks |
| WRITE | Toggle step editing | Play/live-record instruments | Edit or record a parameter |

These meanings are intentionally provisional. Frequently used operations should be directly accessible; uncommon operations can live on the OLED instead of requiring obscure button combinations.

## Musical buttons

The four musical buttons change meaning based on context.

### Sound selection

Hold `SOUND`:

```text
[1] Kick
[2] Snare
[3] Hi-hat
[4] Percussion
```

Additional sounds could later be exposed through banks/pages without adding more physical buttons.

### Step editing

During step editing, the four buttons represent sequencer steps:

```text
[1] [2] [3] [4]
```

Paging can expose additional steps, for example a conventional 16-step pattern as four pages of four steps.

### Live playing / recording

While playing, holding `WRITE` could turn the four musical buttons into instrument pads:

```text
WRITE + [1] = Kick
WRITE + [2] = Snare
WRITE + [3] = Hi-hat
WRITE + [4] = Percussion
```

This avoids needing separate physical controls for step entry and live instrument playing.

## Pattern controls

Hold `PATTERN` to select patterns:

```text
[1] Pattern A
[2] Pattern B
[3] Pattern C
[4] Pattern D
```

Later, entering multiple pattern buttons while holding `PATTERN` could create an arrangement/chain:

```text
PATTERN + 1, 1, 2, 4

A -> A -> B -> D -> repeat
```

This maps naturally to the software model of reusable Patterns inside a Sequence.

## Knobs

### Master volume

The volume knob should probably remain dedicated to volume. Its behavior should not depend on the current UI mode.

This could potentially be implemented as an analog audio control rather than an MCU input, depending on the final audio circuit.

### Control knob

The second knob/encoder is context-sensitive. Possible mappings:

```text
CONTROL                  selected parameter
PLAY + CONTROL           tempo
SOUND + CONTROL          sound parameter
PATTERN + CONTROL        pattern/bank selection
WRITE + CONTROL          edit/record parameter
```

If the control is a rotary encoder with a push switch, pressing it can also provide a natural select/confirm action for menus.

## OLED responsibilities

The OLED lets us avoid turning every uncommon operation into a button chord.

For example, pattern management could be exposed as:

```text
Pattern
> Copy
  Clear
  Chain
  Length
```

General rule:

> Use modifiers for operations performed frequently. Use the OLED/menu for operations performed occasionally.

This should keep the physical interface small without making normal operation dependent on memorizing complicated combinations.

## Current front-panel concept

```text
[ PLAY ] [ SOUND ] [ PATTERN ] [ WRITE ]

        [1] [2] [3] [4]

          VOL    CONTROL
```

The exact arrangement can change with the enclosure/PCB. The software should not depend on a particular physical pin or button layout.

## Software implications

The UI should remain separate from the musical data model. A physical button press should become an input/action that operates on the sequencer rather than having hardware assumptions embedded in Pattern, Measure, Note, etc.

Current conceptual musical hierarchy:

```text
Sequence
  Tracks (when needed)
    Patterns
      Measures
        Notes
```

The same sequencer/data model should eventually be usable with different sound engines:

- synthesized drums
- sampled drums from external storage
- melodic synthesizer voices
- MIDI output

This first hardware version is therefore one interface for the software engine, rather than defining the engine itself.

## Reference

The modifier-oriented interaction ideas were inspired by the Teenage Engineering PO-12 Rhythm workflow:

https://teenage.engineering/guides/po-12/en
