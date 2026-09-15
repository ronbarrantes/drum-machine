#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define PPQN 96
#define MAX_NOTES_PER_MEASURE 32
#define MAX_MEASURES_PER_PATTERN 4
#define MAX_PATTERNS 4
#define MAX_ACTIVE_NOTES (MAX_NOTES_PER_MEASURE * MAX_MEASURES_PER_PATTERN)

typedef struct {
  uint8_t pitch;
  uint8_t velocity;
  uint16_t duration_ticks;
  uint32_t start_tick;
} Note;

typedef struct {
  Note notes[MAX_NOTES_PER_MEASURE];
  uint8_t note_count;
  uint8_t beats_per_measure;
  uint8_t beat_unit;
} Measure;

typedef struct {
  Measure measures[MAX_MEASURES_PER_PATTERN];
  uint8_t measure_count;
} Pattern;

typedef struct {
  Pattern patterns[MAX_PATTERNS];
  uint8_t pattern_count;
  uint16_t bpm;
} Sequence;

typedef struct {
  uint8_t pitch;
  uint32_t end_tick;
  bool active;
} ActiveNote;

typedef struct {
  const Sequence *sequence;
  uint8_t curr_pattern;
  uint8_t curr_measure;
  uint32_t curr_tick;
  uint32_t started_at_tick;
  uint32_t last_tick_at;
  ActiveNote active_notes[MAX_ACTIVE_NOTES];
  bool playing;
} Sequencer;

void sequencer_init(Sequencer *sequencer, const Sequence *sequence) {
  *sequencer = (Sequencer){
    .playing = false,
    .curr_measure = 0,
    .curr_pattern = 0,
    .curr_tick = 0,
    .started_at_tick = 0,
    .last_tick_at = 0,
    .sequence = sequence,
  };
}

void note_on(uint8_t pitch, uint8_t velocity, uint32_t tick) {
  printf("note_on pitch=%u velocity=%u tick=%lu\n",
         pitch,
         velocity,
         (unsigned long)tick);
}

void note_off(uint8_t pitch, uint32_t tick) {
  printf("note_off pitch=%u tick=%lu\n", pitch, (unsigned long)tick);
}

// Return the musical length of a measure, or zero for an unsupported signature.
static uint32_t measure_length_ticks(const Measure *measure) {
  if (measure == NULL || measure->beats_per_measure == 0) {
    return 0;
  }

  switch (measure->beat_unit) {
    case 1:
    case 2:
    case 4:
    case 8:
    case 16:
    case 32:
    case 64:
      break;
    default:
      return 0;
  }

  uint32_t ticks =
    (uint32_t)PPQN * measure->beats_per_measure * 4 / measure->beat_unit;
  return ticks == 0 ? 0 : ticks;
}

static void release_notes_due(Sequencer *sequencer, uint32_t tick) {
  for (size_t index = 0; index < MAX_ACTIVE_NOTES; index++) {
    ActiveNote *active_note = &sequencer->active_notes[index];
    if (active_note->active &&
        (int32_t)(tick - active_note->end_tick) >= 0) {
      note_off(active_note->pitch, tick);
      active_note->active = false;
    }
  }
}

static void release_all_notes(Sequencer *sequencer, uint32_t tick) {
  for (size_t index = 0; index < MAX_ACTIVE_NOTES; index++) {
    ActiveNote *active_note = &sequencer->active_notes[index];
    if (active_note->active) {
      note_off(active_note->pitch, tick);
      active_note->active = false;
    }
  }
}

static void emit_notes_at_position(Sequencer *sequencer, uint32_t tick) {
  const Pattern *pattern = &sequencer->sequence->patterns[sequencer->curr_pattern];
  const Measure *measure = &pattern->measures[sequencer->curr_measure];

  for (uint8_t index = 0; index < measure->note_count; index++) {
    const Note *note = &measure->notes[index];
    if (note->start_tick == sequencer->curr_tick) {
      note_on(note->pitch, note->velocity, tick);
      for (size_t active_index = 0; active_index < MAX_ACTIVE_NOTES;
           active_index++) {
        ActiveNote *active_note = &sequencer->active_notes[active_index];
        if (!active_note->active) {
          *active_note = (ActiveNote){
            .pitch = note->pitch,
            .end_tick = tick + note->duration_ticks,
            .active = true,
          };
          break;
        }
      }
    }
  }
}

// Add one note to a measure without changing notes already stored there.
bool measure_add_note(
  Measure *measure,
  uint8_t pitch,
  uint8_t velocity,
  uint16_t duration_ticks,
  uint32_t start_tick
) {
  if (measure == NULL || measure->note_count >= MAX_NOTES_PER_MEASURE ||
      pitch > 127 || velocity > 127 || duration_ticks == 0) {
    return false;
  }

  measure->notes[measure->note_count++] = (Note){
    .pitch = pitch,
    .velocity = velocity,
    .duration_ticks = duration_ticks,
    .start_tick = start_tick,
  };
  return true;
}

// Restart the selected pattern. Invalid input leaves playback state unchanged.
bool sequencer_play(Sequencer *sequencer, uint32_t now_tick) {
  if (sequencer == NULL || sequencer->sequence == NULL) {
    return false;
  }

  const Sequence *sequence = sequencer->sequence;
  if (sequence->pattern_count == 0 || sequence->pattern_count > MAX_PATTERNS ||
      sequencer->curr_pattern >= sequence->pattern_count) {
    return false;
  }

  const Pattern *pattern = &sequence->patterns[sequencer->curr_pattern];
  if (pattern->measure_count == 0 ||
      pattern->measure_count > MAX_MEASURES_PER_PATTERN) {
    return false;
  }

  if (sequencer->playing) {
    release_all_notes(sequencer, now_tick);
  }
  sequencer->curr_measure = 0;
  sequencer->curr_tick = 0;
  sequencer->started_at_tick = now_tick;
  sequencer->last_tick_at = now_tick;
  sequencer->playing = true;
  emit_notes_at_position(sequencer, now_tick);
  return true;
}

void sequencer_update(Sequencer *sequencer, uint32_t now_tick) {
  if (sequencer == NULL || sequencer->sequence == NULL || !sequencer->playing) {
    return;
  }

  const Sequence *sequence = sequencer->sequence;

  if (sequence->pattern_count == 0 || sequence->pattern_count > MAX_PATTERNS ||
      sequencer->curr_pattern >= sequence->pattern_count) {
    return;
  }

  uint32_t ticks_to_advance = now_tick - sequencer->last_tick_at;
  sequencer->last_tick_at = now_tick;

  while (ticks_to_advance > 0) {
    const Pattern *pattern = &sequence->patterns[sequencer->curr_pattern];
    if (pattern->measure_count == 0 ||
        pattern->measure_count > MAX_MEASURES_PER_PATTERN ||
        sequencer->curr_measure >= pattern->measure_count) {
      return;
    }

    const Measure *measure = &pattern->measures[sequencer->curr_measure];
    uint32_t measure_ticks = measure_length_ticks(measure);
    if (measure_ticks == 0) {
      return;
    }

    sequencer->curr_tick++;
    ticks_to_advance--;
    if (sequencer->curr_tick >= measure_ticks) {
      sequencer->curr_tick = 0;
      sequencer->curr_measure++;
      if (sequencer->curr_measure >= pattern->measure_count) {
        sequencer->curr_measure = 0;
        sequencer->curr_pattern++;
        if (sequencer->curr_pattern >= sequence->pattern_count) {
          sequencer->curr_pattern = 0;
        }
      }
    }

    uint32_t event_tick = now_tick - ticks_to_advance;
    release_notes_due(sequencer, event_tick);
    emit_notes_at_position(sequencer, event_tick);
  }
}

void sequencer_stop(Sequencer *sequencer, uint32_t now_tick) {
  if (sequencer == NULL) {
    return;
  }

  release_all_notes(sequencer, now_tick);
  sequencer->playing = false;
  sequencer->curr_pattern = 0;
  sequencer->curr_measure = 0;
  sequencer->curr_tick = 0;
  sequencer->started_at_tick = now_tick;
  sequencer->last_tick_at = now_tick;
}

int main(void) {
  Sequence sequence = {0};
  sequence.pattern_count = 1;
  sequence.patterns[0].measure_count = 1;
  sequence.patterns[0].measures[0].beats_per_measure = 4;
  sequence.patterns[0].measures[0].beat_unit = 4;

  Measure *measure = &sequence.patterns[0].measures[0];
  measure_add_note(measure, 36, 100, 24, 0);
  measure_add_note(measure, 42, 80, 12, 24);
  measure_add_note(measure, 38, 110, 24, 48);
  measure_add_note(measure, 46, 90, 12, 48);

  Sequencer sequencer;
  sequencer_init(&sequencer, &sequence);
  if (!sequencer_play(&sequencer, 1000)) {
    return 1;
  }

  sequencer_update(&sequencer, 1000);
  sequencer_update(&sequencer, 1024);
  sequencer_update(&sequencer, 1048);
  sequencer_stop(&sequencer, 1050);

  return 0;
}
