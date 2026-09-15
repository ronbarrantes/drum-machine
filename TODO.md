# Drum machine TODO

Build a portable sequencer on macOS/Linux, then connect it to the ATmega328P.
Work through one function at a time. Keep code in `main.c` until splitting it
makes the project easier to understand. Explain each change before moving on.

## Where we are

- [x] Define `Note`, `Measure`, `Pattern`, and `Sequence`.
- [x] Define `Sequencer` playback state.
- [x] Add `sequencer_init()` with a read-only sequence pointer and stopped state.
- [ ] Complete and demonstrate playback. Current `sequencer_update()` is a draft;
      `main()` only constructs example notes and a measure.

Current capacities are 32 notes per measure, 4 measures per pattern, and
4 patterns per sequence. These are desktop development limits, not a settled
ATmega storage design. `timer.c` is inherited ATtiny85 code, not a working
ATmega328P timer implementation.

## 1. First milestone: print notes at the correct ticks

Finish when one sequence containing one pattern and one 4/4 measure prints
each expected note exactly once, including simultaneous notes and tick zero.

- [x] Agree on the time contract: `now_tick` is the total musical ticks since
      the external clock started. It continues across measure boundaries;
      the sequencer tracks its measure position separately. `Note.start_tick`
      is relative to its measure. At 96 PPQN, a 4/4 measure lasts 384 ticks.
      Supply simulated tick values on desktop first; a real clock will supply
      the same input later.
- [x] Add `sequencer_play(sequencer, now_tick)`. Start the selected pattern
      from its beginning and record the clock origin. Return false for null
      pointers, invalid pattern selection, or empty/out-of-capacity pattern
      and measure counts, leaving playback state unchanged. Measures with
      no notes are allowed as rests.
- [x] Change `sequencer_update()` to consume musical ticks. Remove its
      microsecond/BPM conversion. Keep elapsed clock time and measure position
      clearly distinguished in names and types.
- [ ] Connect one measure to a pattern and sequence in `main()`. Use distinct
      note times, two simultaneous notes, and MIDI-style pitches within 0–127.
- [x] Add a note-setting helper that validates MIDI pitch/velocity ranges,
      positive duration, and measure capacity.
- [ ] Add a small note-on output function that initially prints pitch,
      velocity, and musical time.
- [ ] Trigger every note due at the current position. Handle tick zero once,
      repeated updates at the same tick, and updates that skip over ticks.
- [ ] Feed simulated ticks from `main()` and check printed events against
      the example data. No sleeps or AVR includes needed.
- [ ] Record a simple desktop compile/run command and use it on macOS/Linux
      as those environments become available.

## 2. Make playback complete

- [ ] Extract measure-length calculation when it helps readability. Check
      supported time signatures and invalid values before division.
- [ ] Advance through measures, then loop the selected pattern. The current
      draft advances through every pattern automatically; the writeup calls
      for selected-pattern looping, with arrangements deferred.
- [ ] Add note-off events using `duration_ticks`. Track active notes so endings
      survive measure boundaries. Decide how repeated overlapping pitches work.
- [ ] Add `sequencer_stop()`: stop playback, release active notes, reset position.
- [ ] Add pattern selection while stopped. Leave switching during playback
      until we choose when the switch takes effect.
- [ ] Add focused assertions for event timing, simultaneous notes, looping,
      skipped/repeated ticks, and stopping. Check behavior, not every field.

Pause/resume is separate from stop/restart. Add it when needed, with an explicit
decision about held notes and elapsed time while paused.

## 3. Run with real desktop time

- [ ] Add clock code that converts elapsed time and BPM into 96-PPQN ticks.
      Preserve fractional time to avoid accumulating rounding drift.
- [ ] Supply time from a desktop monotonic clock and reuse the same sequencer.
- [ ] Decide how tempo changes, playback restart, and counter rollover work.
- [ ] Try a minimal desktop sound output if useful. Keep audio sample timing
      separate from musical ticks, and keep output details outside sequencing.

## 4. Pivot to hardware when ready

- [ ] Measure RAM/flash needs and choose smaller capacities or storage before
      instantiating the current nested arrays on the ATmega328P.
- [ ] Implement the ATmega328P timer and feed its time into the musical clock.
- [ ] Prove one note event can produce sound with the chosen output hardware.
- [ ] Add button press/hold/release events, then LEDs and the display.
- [ ] Implement the proposed PLAY/SOUND/PATTERN/WRITE controls, four-step paging,
      dedicated volume, and context-dependent control knob/encoder.
- [ ] Add pattern saving after choosing a representation that fits storage.

Hardware parts, power, and output circuitry remain provisional. Verify them
when selecting and connecting hardware.

## Later, only as needed

- [ ] Step editing and live recording, then quantization and swing.
- [ ] Pattern switching during playback and optional pattern arrangements.
- [ ] Tracks/routing when instrument selection needs more than pitch mapping.
- [ ] MIDI output, melodic voices, samples, and external storage.
- [ ] Parameter automation and more controls.

The four-button grid is an editing interface. Preserve tick-based note timing
so the sequencer can also represent performances between grid positions.
