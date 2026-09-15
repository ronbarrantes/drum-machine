#include "sequencer.h"

#include <stddef.h>

void sequencer_init(
  Sequencer *sequencer,
  Sequence *sequence,
  SequencerEventHandler event_handler,
  void *event_context
) {
  *sequencer = (Sequencer){
    .playing = false,
    .curr_measure = 0,
    .curr_pattern = 0,
    .requested_pattern = 0,
    .curr_tick = 0,
    .started_at_tick = 0,
    .last_tick_at = 0,
    .sequence = sequence,
    .pattern_change_pending = false,
    .event_handler = event_handler,
    .event_context = event_context,
  };
}

static void emit_event(
  Sequencer *sequencer,
  SequencerEventType type,
  uint8_t pitch,
  uint8_t velocity,
  uint32_t tick
) {
  if (sequencer->event_handler != NULL) {
    SequencerEvent event = {
      .type = type,
      .pitch = pitch,
      .velocity = velocity,
      .tick = tick,
    };
    sequencer->event_handler(&event, sequencer->event_context);
  }
}

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
      emit_event(sequencer, SEQUENCER_NOTE_OFF, active_note->pitch, 0, tick);
      active_note->active = false;
    }
  }
}

static void release_all_notes(Sequencer *sequencer, uint32_t tick) {
  for (size_t index = 0; index < MAX_ACTIVE_NOTES; index++) {
    ActiveNote *active_note = &sequencer->active_notes[index];
    if (active_note->active) {
      emit_event(sequencer, SEQUENCER_NOTE_OFF, active_note->pitch, 0, tick);
      active_note->active = false;
    }
  }
}

static ActiveNote *reserve_active_note(Sequencer *sequencer, uint8_t pitch) {
  ActiveNote *free_note = NULL;
  for (size_t index = 0; index < MAX_ACTIVE_NOTES; index++) {
    ActiveNote *active_note = &sequencer->active_notes[index];
    if (active_note->active && active_note->pitch == pitch) {
      return active_note;
    }
    if (!active_note->active && free_note == NULL) {
      free_note = active_note;
    }
  }
  return free_note;
}

static void emit_notes_at_position(Sequencer *sequencer, uint32_t tick) {
  const Pattern *pattern = &sequencer->sequence->patterns[sequencer->curr_pattern];
  const Measure *measure = &pattern->measures[sequencer->curr_measure];

  for (uint8_t index = 0; index < measure->note_count; index++) {
    const Note *note = &measure->notes[index];
    if (note->start_tick == sequencer->curr_tick) {
      ActiveNote *active_note = reserve_active_note(sequencer, note->pitch);
      if (active_note != NULL) {
        *active_note = (ActiveNote){
          .pitch = note->pitch,
          .end_tick = tick + note->duration_ticks,
          .active = true,
        };
        emit_event(sequencer, SEQUENCER_NOTE_ON, note->pitch,
                   note->velocity, tick);
      }
    }
  }
}

static bool measure_is_valid(const Measure *measure) {
  uint32_t measure_ticks = measure_length_ticks(measure);
  if (measure_ticks == 0 || measure->note_count > MAX_NOTES_PER_MEASURE) {
    return false;
  }
  for (uint8_t index = 0; index < measure->note_count; index++) {
    const Note *note = &measure->notes[index];
    if (note->pitch > 127 || note->velocity > 127 ||
        note->duration_ticks == 0 || note->start_tick >= measure_ticks) {
      return false;
    }
  }
  return true;
}

static bool pattern_is_valid(const Pattern *pattern) {
  if (pattern == NULL || pattern->measure_count == 0 ||
      pattern->measure_count > MAX_MEASURES_PER_PATTERN) {
    return false;
  }
  for (uint8_t index = 0; index < pattern->measure_count; index++) {
    if (!measure_is_valid(&pattern->measures[index])) {
      return false;
    }
  }
  return true;
}

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

  uint32_t measure_ticks = measure_length_ticks(measure);
  if (measure_ticks == 0 || start_tick >= measure_ticks) {
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
  if (!pattern_is_valid(pattern)) {
    return false;
  }

  if (sequencer->playing) {
    release_all_notes(sequencer, now_tick);
  }
  sequencer->curr_measure = 0;
  sequencer->curr_tick = 0;
  sequencer->requested_pattern = sequencer->curr_pattern;
  sequencer->pattern_change_pending = false;
  sequencer->started_at_tick = now_tick;
  sequencer->last_tick_at = now_tick;
  sequencer->playing = true;
  emit_notes_at_position(sequencer, now_tick);
  return true;
}

bool sequencer_select_pattern(Sequencer *sequencer, uint8_t pattern_index) {
  if (sequencer == NULL || sequencer->sequence == NULL) {
    return false;
  }

  const Sequence *sequence = sequencer->sequence;
  if (sequence->pattern_count == 0 ||
      sequence->pattern_count > MAX_PATTERNS ||
      pattern_index >= sequence->pattern_count) {
    return false;
  }
  if (!pattern_is_valid(&sequence->patterns[pattern_index])) {
    return false;
  }

  if (sequencer->playing) {
    sequencer->requested_pattern = pattern_index;
    sequencer->pattern_change_pending = pattern_index != sequencer->curr_pattern;
  } else {
    sequencer->curr_pattern = pattern_index;
    sequencer->requested_pattern = pattern_index;
    sequencer->pattern_change_pending = false;
  }

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

  if (now_tick < sequencer->last_tick_at) {
    return;
  }

  uint32_t ticks_to_advance = now_tick - sequencer->last_tick_at;
  sequencer->last_tick_at = now_tick;
  if (ticks_to_advance > MAX_TICKS_PER_UPDATE) {
    ticks_to_advance = MAX_TICKS_PER_UPDATE;
  }

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
      }
      if (sequencer->pattern_change_pending) {
        sequencer->curr_pattern = sequencer->requested_pattern;
        sequencer->curr_measure = 0;
        sequencer->pattern_change_pending = false;
      }
    }

    uint32_t event_tick = now_tick - ticks_to_advance;
    release_notes_due(sequencer, event_tick);
    emit_notes_at_position(sequencer, event_tick);
  }
}

bool sequencer_record_note(
  Sequencer *sequencer,
  uint32_t now_tick,
  uint8_t pitch,
  uint8_t velocity,
  uint16_t duration_ticks
) {
  if (sequencer == NULL || sequencer->sequence == NULL ||
      !sequencer->playing || pitch > 127 || velocity > 127 ||
      duration_ticks == 0) {
    return false;
  }

  sequencer_update(sequencer, now_tick);

  Pattern *pattern = &sequencer->sequence->patterns[sequencer->curr_pattern];
  Measure *measure = &pattern->measures[sequencer->curr_measure];
  ActiveNote *active_note = reserve_active_note(sequencer, pitch);
  if (active_note == NULL || measure->note_count >= MAX_NOTES_PER_MEASURE) {
    return false;
  }

  if (!measure_add_note(
        measure, pitch, velocity, duration_ticks, sequencer->curr_tick)) {
    return false;
  }

  *active_note = (ActiveNote){
    .pitch = pitch,
    .end_tick = now_tick + duration_ticks,
    .active = true,
  };
  emit_event(sequencer, SEQUENCER_NOTE_ON, pitch, velocity, now_tick);
  return true;
}

void sequencer_stop(Sequencer *sequencer, uint32_t now_tick) {
  if (sequencer == NULL) {
    return;
  }

  release_all_notes(sequencer, now_tick);
  sequencer->playing = false;
  sequencer->curr_measure = 0;
  sequencer->curr_tick = 0;
  sequencer->requested_pattern = sequencer->curr_pattern;
  sequencer->pattern_change_pending = false;
  sequencer->started_at_tick = now_tick;
  sequencer->last_tick_at = now_tick;
}
