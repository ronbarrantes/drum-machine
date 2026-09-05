#include <stdint.h>

#define PPQN 96
#define MAX_NOTES_PER_MEASURE 32

typedef struct {
  uint8_t pitch;
  uint8_t velocity;
  uint16_t duration;
  uint32_t start_tick;
} Note;

typedef struct {
  Note notes[MAX_NOTES_PER_MEASURE];
  uint8_t note_count;
  uint8_t beats_per_measure;
  uint8_t beat_unit;
} Measure;

typedef struct {
} Pattern;
typedef struct {
} Song;

int main(void) { return 0; }
