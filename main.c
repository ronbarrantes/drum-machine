#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#define PPQN 96
#define MAX_NOTES_PER_MEASURE 32
#define MAX_MEASURES_PER_PATTERN 4
#define MAX_PATTERNS 4

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
  const Sequence *sequence;
  uint8_t curr_pattern;
  uint8_t curr_measure;
  uint32_t curr_tick;
  uint32_t started_at_tick;
  uint32_t last_tick_at;
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

  sequencer->curr_measure = 0;
  sequencer->curr_tick = 0;
  sequencer->started_at_tick = now_tick;
  sequencer->last_tick_at = now_tick;
  sequencer->playing = true;
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
    if (measure->beats_per_measure == 0 || measure->beat_unit == 0) {
      return;
    }

    uint32_t measure_ticks =
      (uint32_t)PPQN * measure->beats_per_measure * 4 / measure->beat_unit;
    if (measure_ticks == 0) {
      return;
    }

    uint32_t remaining_ticks = measure_ticks - sequencer->curr_tick;
    if (ticks_to_advance < remaining_ticks) {
      sequencer->curr_tick += ticks_to_advance;
      break;
    }

    ticks_to_advance -= remaining_ticks;
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
}

int main(void) {
  Note note1 =
    (Note){.pitch = 232, .duration_ticks = 92, .velocity = 80, .start_tick = 0};

  Note notes[] = {note1, note1, note1};
  uint8_t note_count = sizeof(notes) / sizeof(notes[0]);

  Measure measure1 =
    (Measure){.note_count = note_count, .beats_per_measure = 4, .beat_unit = 4};

  memcpy(measure1.notes, notes, sizeof(notes));

  return 0;
}
