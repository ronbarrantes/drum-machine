#include <assert.h>

#define main drum_machine_demo_main
#include "main.c"
#undef main

static Measure make_measure(void) {
  return (Measure){
    .beats_per_measure = 4,
    .beat_unit = 4,
  };
}

static Sequence make_sequence(void) {
  Sequence sequence = {0};
  sequence.pattern_count = 2;
  sequence.patterns[0].measure_count = 1;
  sequence.patterns[0].measures[0] = make_measure();
  sequence.patterns[1].measure_count = 1;
  sequence.patterns[1].measures[0] = make_measure();
  return sequence;
}

static void test_selected_pattern_loops(void) {
  Sequence sequence = make_sequence();

  Sequencer sequencer;
  sequencer_init(&sequencer, &sequence);
  sequencer.curr_pattern = 1;

  assert(sequencer_play(&sequencer, 0));
  sequencer_update(&sequencer, PPQN * 4);

  assert(sequencer.curr_pattern == 1);
  assert(sequencer.curr_measure == 0);
  assert(sequencer.curr_tick == 0);
}

static void test_pattern_selection_while_stopped(void) {
  Sequence sequence = make_sequence();
  Sequencer sequencer;
  sequencer_init(&sequencer, &sequence);

  assert(sequencer_select_pattern(&sequencer, 1));
  assert(sequencer.curr_pattern == 1);
  assert(sequencer_play(&sequencer, 0));
  assert(sequencer.curr_pattern == 1);
}

static void test_pattern_selection_while_playing_waits_for_measure(void) {
  Sequence sequence = make_sequence();
  Sequencer sequencer;
  sequencer_init(&sequencer, &sequence);

  assert(sequencer_play(&sequencer, 0));
  assert(sequencer_select_pattern(&sequencer, 1));
  assert(sequencer.curr_pattern == 0);

  sequencer_update(&sequencer, PPQN * 4 - 1);
  assert(sequencer.curr_pattern == 0);
  sequencer_update(&sequencer, PPQN * 4);
  assert(sequencer.curr_pattern == 1);
  assert(sequencer.curr_tick == 0);
}

static void test_stop_preserves_selected_pattern(void) {
  Sequence sequence = make_sequence();
  Sequencer sequencer;
  sequencer_init(&sequencer, &sequence);

  assert(sequencer_select_pattern(&sequencer, 1));
  assert(sequencer_play(&sequencer, 0));
  sequencer_stop(&sequencer, 10);

  assert(!sequencer.playing);
  assert(sequencer.curr_pattern == 1);
}

static void test_record_note_while_playing(void) {
  Sequence sequence = make_sequence();
  Sequencer sequencer;
  sequencer_init(&sequencer, &sequence);

  assert(sequencer_play(&sequencer, 1000));
  assert(sequencer_record_note(&sequencer, 1000, 60, 100, 24));

  assert(sequence.patterns[0].measures[0].note_count == 1);
  assert(sequence.patterns[0].measures[0].notes[0].pitch == 60);
  assert(sequence.patterns[0].measures[0].notes[0].start_tick == 0);
  assert(sequencer.active_notes[0].active);

  sequencer_update(&sequencer, 1024);
  assert(!sequencer.active_notes[0].active);
}

static void test_stop_releases_recorded_note(void) {
  Sequence sequence = make_sequence();
  Sequencer sequencer;
  sequencer_init(&sequencer, &sequence);

  assert(sequencer_play(&sequencer, 1000));
  assert(sequencer_record_note(&sequencer, 1000, 64, 90, 96));
  sequencer_stop(&sequencer, 1010);

  assert(!sequencer.playing);
  assert(!sequencer.active_notes[0].active);
}

int main(void) {
  test_selected_pattern_loops();
  test_pattern_selection_while_stopped();
  test_pattern_selection_while_playing_waits_for_measure();
  test_stop_preserves_selected_pattern();
  test_record_note_while_playing();
  test_stop_releases_recorded_note();
  return 0;
}
