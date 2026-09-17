#define _DEFAULT_SOURCE

#include "synth.h"

#include <math.h>

static float midi_frequency(uint8_t pitch) {
  return 440.0f * powf(2.0f, ((float)pitch - 69.0f) / 12.0f);
}

static uint32_t next_random(Synth *synth) {
  uint32_t value = synth->random_state;
  value ^= value << 13;
  value ^= value >> 17;
  value ^= value << 5;
  synth->random_state = value;
  return value;
}

static float noise(Synth *synth) {
  return ((float)(next_random(synth) & 0xffff) / 32767.5f) - 1.0f;
}

static SynthVoiceKind kind_for_pitch(uint8_t pitch) {
  switch (pitch) {
    case 38:
      return SYNTH_SNARE;
    case 42:
      return SYNTH_CLOSED_HAT;
    case 46:
      return SYNTH_OPEN_HAT;
    default:
      return SYNTH_KICK;
  }
}

static SynthVoice *find_voice(Synth *synth, uint8_t pitch) {
  SynthVoice *free_voice = NULL;
  SynthVoice *oldest_voice = &synth->voices[0];
  for (size_t index = 0; index < SYNTH_MAX_VOICES; index++) {
    SynthVoice *voice = &synth->voices[index];
    if (voice->active && voice->pitch == pitch) {
      return voice;
    }
    if (!voice->active && free_voice == NULL) {
      free_voice = voice;
    }
    if (voice->age < oldest_voice->age) {
      oldest_voice = voice;
    }
  }
  return free_voice != NULL ? free_voice : oldest_voice;
}

void synth_init(Synth *synth, uint32_t sample_rate) {
  *synth = (Synth){
    .sample_rate = sample_rate,
    .random_state = 0x12345678,
  };
}

void synth_handle_event(const SequencerEvent *event, void *context) {
  Synth *synth = context;
  SynthVoice *voice = find_voice(synth, event->pitch);
  if (event->type == SEQUENCER_NOTE_OFF) {
    if (voice->active && voice->pitch == event->pitch) {
      voice->releasing = true;
      voice->release_step = voice->envelope_level /
                            (0.08f * (float)synth->sample_rate);
    }
    return;
  }

  *voice = (SynthVoice){
    .kind = kind_for_pitch(event->pitch),
    .pitch = event->pitch,
    .frequency = midi_frequency(event->pitch),
    .envelope_level = 0.0f,
    .age = ++synth->age,
    .velocity = event->velocity,
    .active = true,
  };
}

static float envelope(Synth *synth, SynthVoice *voice) {
  float attack_step = 1.0f / (0.002f * (float)synth->sample_rate);
  float decay_step = 0.8f / (0.08f * (float)synth->sample_rate);
  if (voice->releasing) {
    voice->envelope_level -= voice->release_step;
    if (voice->envelope_level <= 0.0f) {
      voice->active = false;
      voice->envelope_level = 0.0f;
    }
  } else if (voice->envelope_level < 1.0f) {
    voice->envelope_level += attack_step;
    if (voice->envelope_level > 1.0f) {
      voice->envelope_level = 1.0f;
    }
  } else {
    voice->envelope_level -= decay_step;
    if (voice->envelope_level < 0.2f) {
      voice->envelope_level = 0.2f;
    }
  }
  return voice->envelope_level;
}

static float voice_sample(Synth *synth, SynthVoice *voice) {
  float sample_rate = (float)synth->sample_rate;
  float time = voice->age / sample_rate;
  float value;
  switch (voice->kind) {
    case SYNTH_KICK: {
      float frequency = 48.0f + 130.0f * expf(-time * 32.0f);
      voice->phase += frequency / sample_rate;
      value = sinf(voice->phase * 6.2831853f);
      break;
    }
    case SYNTH_SNARE: {
      voice->phase += voice->frequency / sample_rate;
      value = 0.25f * sinf(voice->phase * 6.2831853f) + 0.75f * noise(synth);
      break;
    }
    case SYNTH_CLOSED_HAT:
    case SYNTH_OPEN_HAT: {
      float current = noise(synth);
      value = current - 0.92f * voice->phase;
      voice->phase = current;
      break;
    }
  }
  voice->age++;
  return value * envelope(synth, voice) * ((float)voice->velocity / 127.0f);
}

void synth_render(Synth *synth, int16_t *samples, size_t sample_count) {
  for (size_t sample_index = 0; sample_index < sample_count; sample_index++) {
    float mixed = 0.0f;
    for (size_t voice_index = 0; voice_index < SYNTH_MAX_VOICES; voice_index++) {
      SynthVoice *voice = &synth->voices[voice_index];
      if (voice->active) {
        mixed += voice_sample(synth, voice);
      }
    }
    if (mixed > 1.0f) mixed = 1.0f;
    if (mixed < -1.0f) mixed = -1.0f;
    samples[sample_index] = (int16_t)(mixed * 28000.0f);
  }
}
