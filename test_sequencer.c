#include <assert.h>
#include <stddef.h>

#include "sequencer.h"

static void ignore_event(const SequencerEvent *event, void *context) {
  (void)event;
  (void)context;
}

static SequencerEvent events[256];
static size_t event_count;

static void capture_event(const SequencerEvent *event, void *context) {
  (void)context;
  if (event_count < sizeof(events) / sizeof(events[0])) {
    events[event_count++] = *event;
  }
}

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
  sequencer_init(&sequencer, &sequence, ignore_event, NULL);
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
  sequencer_init(&sequencer, &sequence, ignore_event, NULL);

  assert(sequencer_select_pattern(&sequencer, 1));
  assert(sequencer.curr_pattern == 1);
  assert(sequencer_play(&sequencer, 0));
  assert(sequencer.curr_pattern == 1);
}

static void test_pattern_selection_while_playing_waits_for_measure(void) {
  Sequence sequence = make_sequence();
  Sequencer sequencer;
  sequencer_init(&sequencer, &sequence, ignore_event, NULL);

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
  sequencer_init(&sequencer, &sequence, ignore_event, NULL);

  assert(sequencer_select_pattern(&sequencer, 1));
  assert(sequencer_play(&sequencer, 0));
  sequencer_stop(&sequencer, 10);

  assert(!sequencer.playing);
  assert(sequencer.curr_pattern == 1);
}

static void test_record_note_while_playing(void) {
  Sequence sequence = make_sequence();
  Sequencer sequencer;
  sequencer_init(&sequencer, &sequence, capture_event, NULL);
  event_count = 0;

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
  sequencer_init(&sequencer, &sequence, capture_event, NULL);
  event_count = 0;

  assert(sequencer_play(&sequencer, 1000));
  assert(sequencer_record_note(&sequencer, 1000, 64, 90, 96));
  sequencer_stop(&sequencer, 1010);

  assert(!sequencer.playing);
  assert(!sequencer.active_notes[0].active);
}

static void test_invalid_notes_are_rejected(void) {
  Sequence sequence = make_sequence();
  Sequencer sequencer;
  sequencer_init(&sequencer, &sequence, ignore_event, NULL);

  assert(!measure_add_note(&sequence.patterns[0].measures[0], 60, 100, 24,
                           PPQN * 4));
  sequence.patterns[0].measures[0].note_count = MAX_NOTES_PER_MEASURE + 1;
  assert(!sequencer_play(&sequencer, 0));
}

static void test_pattern_switch_happens_at_next_measure(void) {
  Sequence sequence = make_sequence();
  sequence.patterns[0].measure_count = 2;
  sequence.patterns[0].measures[1] = make_measure();
  Sequencer sequencer;
  sequencer_init(&sequencer, &sequence, ignore_event, NULL);

  assert(sequencer_play(&sequencer, 0));
  assert(sequencer_select_pattern(&sequencer, 1));
  sequencer_update(&sequencer, PPQN * 4);
  assert(sequencer.curr_pattern == 1);
  assert(sequencer.curr_measure == 0);
}

static void test_backward_clock_is_ignored(void) {
  Sequence sequence = make_sequence();
  Sequencer sequencer;
  sequencer_init(&sequencer, &sequence, ignore_event, NULL);

  assert(sequencer_play(&sequencer, 100));
  sequencer_update(&sequencer, 110);
  sequencer_update(&sequencer, 90);
  assert(sequencer.curr_tick == 10);
  assert(sequencer.last_tick_at == 110);
}

static void test_event_handler_receives_events(void) {
  Sequence sequence = make_sequence();
  assert(measure_add_note(&sequence.patterns[0].measures[0], 60, 100, 24, 0));
  Sequencer sequencer;
  event_count = 0;
  sequencer_init(&sequencer, &sequence, capture_event, NULL);

  assert(sequencer_play(&sequencer, 1000));
  sequencer_update(&sequencer, 1024);
  assert(event_count == 2);
  assert(events[0].type == SEQUENCER_NOTE_ON);
  assert(events[0].pitch == 60);
  assert(events[0].velocity == 100);
  assert(events[0].tick == 1000);
  assert(events[1].type == SEQUENCER_NOTE_OFF);
  assert(events[1].tick == 1024);
}

static void test_note_capacity_does_not_emit_untracked_notes(void) {
  Sequence sequence = make_sequence();
  Measure *measure = &sequence.patterns[0].measures[0];
  for (uint8_t index = 0; index < MAX_NOTES_PER_MEASURE; index++) {
    assert(measure_add_note(measure, index, 100, 65535, 0));
  }
  Sequencer sequencer;
  event_count = 0;
  sequencer_init(&sequencer, &sequence, capture_event, NULL);
  assert(sequencer_play(&sequencer, 0));
  assert(event_count == MAX_NOTES_PER_MEASURE);
  sequencer_update(&sequencer, PPQN * 4 * 4);
  assert(event_count == MAX_NOTES_PER_MEASURE * 3);
  sequencer_stop(&sequencer, 1);
  assert(event_count == MAX_NOTES_PER_MEASURE * 4);
}

static void test_latest_pattern_request_wins(void) {
  Sequence sequence = make_sequence();
  Sequencer sequencer;
  sequencer_init(&sequencer, &sequence, ignore_event, NULL);

  assert(sequencer_play(&sequencer, 0));
  assert(sequencer_select_pattern(&sequencer, 1));
  assert(sequencer_select_pattern(&sequencer, 0));
  sequencer_update(&sequencer, PPQN * 4);
  assert(sequencer.curr_pattern == 0);
}

static void test_large_clock_gap_is_capped(void) {
  Sequence sequence = make_sequence();
  Sequencer sequencer;
  sequencer_init(&sequencer, &sequence, ignore_event, NULL);

  assert(sequencer_play(&sequencer, 0));
  sequencer_update(&sequencer, MAX_TICKS_PER_UPDATE + 1);
  assert(sequencer.last_tick_at == MAX_TICKS_PER_UPDATE + 1);
}

int main(void) {
  test_selected_pattern_loops();
  test_pattern_selection_while_stopped();
  test_pattern_selection_while_playing_waits_for_measure();
  test_stop_preserves_selected_pattern();
  test_record_note_while_playing();
  test_stop_releases_recorded_note();
  test_invalid_notes_are_rejected();
  test_pattern_switch_happens_at_next_measure();
  test_backward_clock_is_ignored();
  test_event_handler_receives_events();
  test_note_capacity_does_not_emit_untracked_notes();
  test_latest_pattern_request_wins();
  test_large_clock_gap_is_capped();
  return 0;
}
