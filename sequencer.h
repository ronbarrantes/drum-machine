#ifndef SEQUENCER_H
#define SEQUENCER_H

#include <stdbool.h>
#include <stdint.h>

#define PPQN 96
#define MAX_NOTES_PER_MEASURE 32
#define MAX_MEASURES_PER_PATTERN 4
#define MAX_PATTERNS 4
#define MAX_ACTIVE_NOTES \
  (MAX_NOTES_PER_MEASURE * MAX_MEASURES_PER_PATTERN)
#define MAX_TICKS_PER_UPDATE 1024

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

typedef enum {
  SEQUENCER_NOTE_ON,
  SEQUENCER_NOTE_OFF,
} SequencerEventType;

typedef struct {
  SequencerEventType type;
  uint8_t pitch;
  uint8_t velocity;
  uint32_t tick;
} SequencerEvent;

typedef void (*SequencerEventHandler)(
  const SequencerEvent *event,
  void *context
);

typedef struct {
  Sequence *sequence;
  uint8_t curr_pattern;
  uint8_t requested_pattern;
  uint8_t curr_measure;
  uint32_t curr_tick;
  uint32_t started_at_tick;
  uint32_t last_tick_at;
  ActiveNote active_notes[MAX_ACTIVE_NOTES];
  bool pattern_change_pending;
  bool playing;
  SequencerEventHandler event_handler;
  void *event_context;
} Sequencer;

void sequencer_init(
  Sequencer *sequencer,
  Sequence *sequence,
  SequencerEventHandler event_handler,
  void *event_context
);
bool measure_add_note(
  Measure *measure,
  uint8_t pitch,
  uint8_t velocity,
  uint16_t duration_ticks,
  uint32_t start_tick
);
bool sequencer_play(Sequencer *sequencer, uint32_t now_tick);
bool sequencer_select_pattern(Sequencer *sequencer, uint8_t pattern_index);
void sequencer_update(Sequencer *sequencer, uint32_t now_tick);
bool sequencer_record_note(
  Sequencer *sequencer,
  uint32_t now_tick,
  uint8_t pitch,
  uint8_t velocity,
  uint16_t duration_ticks
);
void sequencer_stop(Sequencer *sequencer, uint32_t now_tick);

#endif
