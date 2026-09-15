#define _POSIX_C_SOURCE 200809L

#include "sequencer.h"

#include <stdio.h>
#include <time.h>

enum {
  STEP_TICKS = 24,
  FRAME_MILLISECONDS = 40,
};

typedef struct {
  uint8_t pitch;
  const char *name;
} VoiceDisplay;

static const VoiceDisplay voices[] = {
  {36, "Kick "},
  {38, "Snare"},
  {42, "Hat  "},
  {46, "Open "},
};

static void ignore_event(const SequencerEvent *event, void *context) {
  (void)event;
  (void)context;
}

static void add_step_note(
  Pattern *pattern,
  uint8_t pitch,
  uint8_t step,
  uint8_t velocity
) {
  uint8_t measure_index = step / 4;
  uint8_t step_in_measure = step % 4;
  measure_add_note(
    &pattern->measures[measure_index],
    pitch,
    velocity,
    STEP_TICKS,
    step_in_measure * STEP_TICKS
  );
}

static bool has_note_at_step(const Pattern *pattern, uint8_t pitch, uint8_t step) {
  const Measure *measure = &pattern->measures[step / 4];
  uint32_t start_tick = (step % 4) * STEP_TICKS;
  for (uint8_t index = 0; index < measure->note_count; index++) {
    if (measure->notes[index].pitch == pitch &&
        measure->notes[index].start_tick == start_tick) {
      return true;
    }
  }
  return false;
}

static void render(const Sequencer *sequencer) {
  const Pattern *pattern = &sequencer->sequence->patterns[sequencer->curr_pattern];
  uint8_t current_step = sequencer->curr_measure * 4 +
                         sequencer->curr_tick / STEP_TICKS;

  printf("\033[H\033[J");
  printf("Drum machine     %s     pattern %u     tempo %u BPM\n\n",
         sequencer->playing ? "PLAYING" : "STOPPED",
         (unsigned)sequencer->curr_pattern + 1,
         (unsigned)sequencer->sequence->bpm);

  printf("       ");
  for (uint8_t step = 0; step < 16; step++) {
    printf(" %2u", (unsigned)(step + 1));
  }
  printf("\n       ");
  for (uint8_t step = 0; step < 16; step++) {
    printf(" %2s", step == current_step ? "^" : " ");
  }
  printf("\n");

  for (size_t voice_index = 0;
       voice_index < sizeof(voices) / sizeof(voices[0]);
       voice_index++) {
    printf("%s  ", voices[voice_index].name);
    for (uint8_t step = 0; step < 16; step++) {
      printf(" %2c", has_note_at_step(pattern, voices[voice_index].pitch, step)
                        ? 'X'
                        : '.');
    }
    printf("\n");
  }

  printf("\nCtrl-C to quit. This view uses the same tick-based sequencer as the hardware target.\n");
  fflush(stdout);
}

static uint64_t monotonic_milliseconds(void) {
  struct timespec now;
  clock_gettime(CLOCK_MONOTONIC, &now);
  return (uint64_t)now.tv_sec * 1000 + (uint64_t)now.tv_nsec / 1000000;
}

int main(void) {
  Sequence sequence = {0};
  sequence.pattern_count = 1;
  sequence.bpm = 120;
  sequence.patterns[0].measure_count = 4;
  for (uint8_t measure = 0; measure < 4; measure++) {
    sequence.patterns[0].measures[measure].beats_per_measure = 4;
    sequence.patterns[0].measures[measure].beat_unit = 4;
  }

  Pattern *pattern = &sequence.patterns[0];
  for (uint8_t step = 0; step < 16; step += 4) {
    add_step_note(pattern, 36, step, 115);
    add_step_note(pattern, 42, step, 75);
  }
  for (uint8_t step = 2; step < 16; step += 4) {
    add_step_note(pattern, 42, step, 65);
  }
  for (uint8_t step = 4; step < 16; step += 8) {
    add_step_note(pattern, 38, step, 105);
  }
  add_step_note(pattern, 46, 14, 90);

  Sequencer sequencer;
  sequencer_init(&sequencer, &sequence, ignore_event, NULL);
  if (!sequencer_play(&sequencer, 0)) {
    return 1;
  }

  uint64_t started_at = monotonic_milliseconds();
  for (;;) {
    uint64_t elapsed = monotonic_milliseconds() - started_at;
    uint32_t now_tick = (uint32_t)(elapsed * sequence.bpm * PPQN / 60000);
    sequencer_update(&sequencer, now_tick);
    render(&sequencer);

    struct timespec delay = {
      .tv_sec = 0,
      .tv_nsec = FRAME_MILLISECONDS * 1000000L,
    };
    nanosleep(&delay, NULL);
  }
}
