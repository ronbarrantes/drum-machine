#ifndef SYNTH_H
#define SYNTH_H

#include "sequencer.h"

#include <stddef.h>
#include <stdint.h>

#define SYNTH_MAX_VOICES 8

typedef enum {
  SYNTH_KICK,
  SYNTH_SNARE,
  SYNTH_CLOSED_HAT,
  SYNTH_OPEN_HAT,
} SynthVoiceKind;

typedef struct {
  SynthVoiceKind kind;
  uint8_t pitch;
  float phase;
  float frequency;
  float envelope_level;
  float release_step;
  uint32_t age;
  uint8_t velocity;
  bool active;
  bool releasing;
} SynthVoice;

typedef struct {
  SynthVoice voices[SYNTH_MAX_VOICES];
  uint32_t sample_rate;
  uint32_t random_state;
  uint32_t age;
} Synth;

void synth_init(Synth *synth, uint32_t sample_rate);
void synth_handle_event(const SequencerEvent *event, void *context);
void synth_render(Synth *synth, int16_t *samples, size_t sample_count);

#endif
