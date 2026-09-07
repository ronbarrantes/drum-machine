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
  Sequence *sequence;
  uint8_t curr_pattern;
  uint8_t curr_measure;
  uint16_t curr_tick;
  uint32_t last_tick_at;
  bool playing;
} Sequencer;

void sequencer_start(Sequencer *sequencer) {
  *sequencer = (Sequencer){
    .playing = false,
    .curr_measure = 0,
    .curr_pattern = 0,
    .curr_tick = 0,
    .last_tick_at = 0,
    .sequence = NULL,
  };
}

void sequencer_update(Sequencer *sequencer, uint32_t now_us) {
  if (sequencer->sequence == NULL || !sequencer->playing) {
    return;
  }

  Sequence *sequence = sequencer->sequence;

  // Advance tick
  uint32_t tick_us = 60000000UL / sequence->bpm / PPQN;

  if (now_us - sequencer->last_tick_at >= tick_us) {
    sequencer->curr_tick++;
    sequencer->last_tick_at += tick_us;
  }

  // Current pattern + measure
  Pattern *pattern = &sequence->patterns[sequencer->curr_pattern];
  Measure *measure = &pattern->measures[sequencer->curr_measure];

  // How many ticks are in this measure?
  uint16_t measure_ticks =
    PPQN * measure->beats_per_measure * 4 / measure->beat_unit;

  // Next measure
  if (sequencer->curr_tick >= measure_ticks) {
    sequencer->curr_tick = 0;
    sequencer->curr_measure++;

    // Next pattern
    if (sequencer->curr_measure >= pattern->measure_count) {
      sequencer->curr_measure = 0;
      sequencer->curr_pattern++;

      // End of sequence -> loop back
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
