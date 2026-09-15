# Drum machine TODO

Build a portable sequencer, then prove a small playable ATmega328P prototype.
Work in small, explained changes. Use this TODO as the current implementation
order; the README describes the broader project direction.

The next hardware milestone is one button triggering one synthesized voice,
followed by a repeating pattern with start/stop and tempo control. Display,
full recording controls, storage, and the final PCB can follow that milestone.

The current control plan is eight illuminated pads, four function buttons, and
two rotary encoders with push buttons. That is 14 button switches plus four
quadrature encoder signals. The earlier four-pad and potentiometer assumptions
are obsolete.

## Where we are

- [x] Define `Note`, `Measure`, `Pattern`, and `Sequence`.
- [x] Define `Sequencer` playback state.
- [x] Add `sequencer_init()` with stopped state. The sequence pointer is now
      mutable so recording can add notes to its patterns.
- [x] Complete and demonstrate the first note-on/note-off playback path.
      Full playback controls and desktop clock work remain below.
- [x] Split public types/API into `sequencer.h`, implementation into
      `sequencer.c`, desktop demo into `main.c`, and tests into
      `test_sequencer.c`.

The current tests cover basic playback state, selection, recording, invalid
input, event delivery, and active-note tracking. Event-stream coverage still
needs more cases for skipped ticks, simultaneous notes, and recording across
boundaries.

The first code review found several correctness issues. They are now recorded
below with their fixes checked off where complete:

- Playback previously emitted more notes than it could track, leaving notes
  without a matching `note_off`. The reproduction produced 160 note-ons and
  only 128 note-offs.
- Invalid note counts previously could read past the fixed note array. Invalid
  patterns were not consistently rejected before selection or playback.
- Notes could be inserted at or beyond the end of a measure and then never
  play.
- An earlier timestamp could cause an enormous catch-up loop because tick
  differences used unsigned arithmetic.
- The AVR compiler reports about 4,940 bytes of stack for `main()` with the
  current desktop-sized arrays, while the ATmega328P has 2,048 bytes of SRAM.

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
- [x] Connect one measure to a pattern and sequence in `main()`. Use distinct
      note times, two simultaneous notes, and MIDI-style pitches within 0–127.
- [x] Add a note-setting helper that validates MIDI pitch/velocity ranges,
      positive duration, and measure capacity.
- [x] Add a small note-on output function that initially prints pitch,
      velocity, and musical time.
- [x] Trigger every note due at the current position. Handle tick zero once,
      repeated updates at the same tick, and updates that skip over ticks.
- [x] Feed simulated ticks from `main()` and check printed events against
      the example data. No sleeps or AVR includes needed.
- [x] Record desktop compile/run commands in README and run them on macOS.
      Linux verification remains unconfirmed.

## 2. Make playback complete

- [x] Extract measure-length calculation when it helps readability. Check
      supported time signatures and invalid values before division.
- [x] Advance through measures, then loop the selected pattern. Arrangements
      remain deferred.
- [x] Add note-off events using `duration_ticks`. Track active notes so endings
      can survive measure boundaries. Boundary event tests remain below.
- [x] Add `sequencer_stop()`: release active notes, reset measure position,
      preserve the current pattern, and cancel any queued selection.
- [x] Select patterns immediately while stopped and queue them while playing.
- [x] Fix queued switching to happen at the next measure boundary and start
      the target at its first measure. Test a source pattern with multiple
      measures.
- [x] Cover queued-selection rules: the latest request wins, selecting the
      current pattern cancels the request, and stopping cancels it too.
- [x] Add `sequencer_record_note()` for playback: the caller supplies a clock
      tick and a known duration; the note is stored and triggered immediately.
      This is not yet press/release recording or a complete editing interface.
- [x] Move printing out of `sequencer.c` behind a small note-event output
      interface. Let the demo print events, tests capture them, and the synth
      consume them. Keep the interface small; add modules only when needed.
- [ ] Assert emitted pitch, velocity, and event order for simultaneous notes,
      skipped ticks, recording across a boundary, and replay on the next loop.
      Tick-zero, repeated-tick, looping, queued-switch, and stopping cases are
      covered. Extend the existing tests around observable behavior.
- [x] Validate target patterns before selection/playback and before recording
      indexes their arrays: measure counts, signatures, note counts, and note
      positions within the measure. Reject invalid input without partial edits.
- [x] Handle active-note capacity before emitting note-on. Playback previously
      emits even if no tracking slot is free, which can leave a note without
      a matching note-off. The current rule reuses an active slot for the same
      pitch and drops a new note when all slots are occupied.
- [x] Reject backward clock timestamps before entering the update loop and cap
      catch-up work at `MAX_TICKS_PER_UPDATE` so a bad timestamp cannot freeze
      the application.
- [x] Choose repeated-pitch behavior for the first drum voice. Retriggering
      reuses that pitch's active slot so an older ending cannot cut it off.
      Velocity-zero behavior remains part of the event boundary design.

Pause/resume is separate from stop/restart. Add it when needed, with an explicit
decision about held notes and elapsed time while paused.

## 3. Check hardware fit and add the musical clock

Do the hardware budget and output-path checks alongside the playback fixes.
They determine which capacities and timing choices are practical.

- [ ] Measure target RAM/flash needs with the AVR toolchain. The current AVR
      compile reports about 4,940 bytes of stack for `main()` against 2,048
      bytes of ATmega328P SRAM. Budget patterns,
      active notes, stack, synth state, and display buffers together. Choose
      smaller capacities or a compact representation before instantiating the
      current desktop arrays on the ATmega328P.
- [ ] Verify a practical audio path for the ATmega328P against the component
      datasheets. Check PWM plus filter/amplifier versus the interface required
      by the available MAX98357 module; owning the module does not establish
      compatibility. Choose the first output before soldering its circuit.
- [ ] Assign pins and timers for programming, timekeeping, audio output,
      eight pads, four function buttons, two encoder switches, four encoder
      signals, LED shift register, and I2C display. Verify whether the
      ATmega328P needs an input expander or button matrix before committing to
      the board layout.

- [ ] Add clock code that converts elapsed time and BPM into 96-PPQN ticks.
      Preserve fractional time to avoid accumulating rounding drift.
- [ ] Supply time from a desktop monotonic clock and reuse the same sequencer.
- [ ] Decide how tempo changes, playback restart, and counter rollover work.
      Backward timestamps are rejected and catch-up work is capped at
      `MAX_TICKS_PER_UPDATE`; test the chosen policy with the real clock.
- [ ] Test fractional timing and tempo changes with simulated elapsed time;
      use a short real-time desktop demo to exercise the same clock.

## 4. First playable hardware prototype

- [ ] Bring up ATmega programming, power/decoupling, and a non-blocking LED
      heartbeat. Replace the inherited ATtiny85 timer with an ATmega328P timer.
- [ ] Generate one simple drum voice and prove one note event produces sound
      through the verified output circuit. Keep audio sample timing separate
      from musical ticks. Use a desktop audio experiment only if it helps here.
- [ ] Add one debounced button with press/hold/release events and use it to
      trigger that voice immediately, including while transport is stopped.
- [ ] Feed the hardware clock into the sequencer and play one repeating
      pattern. Add start/stop and tempo control; check responsiveness and sound
      while controls are being read.

This milestone is enough to start playing with the hardware. The full editor
and recording modes do not need to be finished first.

## 5. Editing, recording, and the remaining controls

- [ ] Add the remaining pads, function buttons, encoder switches, encoder
      signals, shift-register LEDs, and temporary display.
- [ ] Settle the function-button assignments, beat/page selection, and encoder
      roles. PLAY/SOUND/PATTERN/WRITE and dedicated volume are proposals until
      confirmed through use of the prototype.
- [ ] Add step add/remove/edit operations while stopped and playing. Keep the
      edit page distinct from the playback position. Define when an edit is
      first heard and keep pattern edits in the main loop.
- [ ] Separate live triggering from recording being armed. For initial drums,
      use a chosen duration per hit with the existing recording API. Measure
      duration from press/release only when a voice needs held notes.
- [ ] Add optional quantization for live recording while retaining tick-based
      timing for unquantized notes. Define recording behavior on pattern switch
      and stop, and when the measure is full.
- [ ] Add pattern saving after choosing a representation that fits EEPROM;
      save deliberately rather than on every control event.
- [ ] Finish speaker/headphone routing, battery power, and enclosure choices;
      verify the circuit and resource budget before making the final PCB.

Hardware parts, power, and output circuitry remain provisional. Verify them
when selecting and connecting hardware.

## Later, only as needed

- [ ] Swing and optional pattern arrangements.
- [ ] Tracks/routing when instrument selection needs more than pitch mapping.
- [ ] MIDI output, melodic voices, samples, and external storage.
- [ ] Parameter automation and more controls.

The eight-pad grid is an editing interface. Preserve tick-based note timing
so the sequencer can also represent performances between grid positions.
