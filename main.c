#include "sequencer.h"

#include <stdio.h>

static void print_event(const SequencerEvent *event, void *context) {
  (void)context;
  if (event->type == SEQUENCER_NOTE_ON) {
    printf("note_on pitch=%u velocity=%u tick=%lu\n",
           event->pitch, event->velocity, (unsigned long)event->tick);
  } else {
    printf("note_off pitch=%u tick=%lu\n",
           event->pitch, (unsigned long)event->tick);
  }
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
  sequencer_init(&sequencer, &sequence, print_event, NULL);
  if (!sequencer_play(&sequencer, 1000)) {
    return 1;
  }

  sequencer_update(&sequencer, 1000);
  sequencer_update(&sequencer, 1024);
  sequencer_update(&sequencer, 1048);
  sequencer_stop(&sequencer, 1050);

  return 0;
}
