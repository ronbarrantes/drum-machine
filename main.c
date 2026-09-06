#include <stdint.h>

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
  uint8_t curr_tick;
  bool playing;

} Sequencer;

int main(void) { return 0; }
