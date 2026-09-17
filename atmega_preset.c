#ifndef F_CPU
#define F_CPU 8000000UL
#endif

#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/pgmspace.h>

#include <stdint.h>

enum {
  SAMPLE_RATE = 15625,
  BPM = 120,
  STEP_COUNT = 16,
  VOICE_COUNT = 4,
};

typedef enum {
  KICK,
  SNARE,
  CLOSED_HAT,
  OPEN_HAT,
} VoiceKind;

typedef struct {
  VoiceKind kind;
  uint32_t phase;
  uint32_t phase_increment;
  uint16_t envelope;
  uint16_t envelope_step;
  uint32_t random_state;
  int16_t previous_noise;
  uint8_t active;
} Voice;

static const uint16_t pattern[VOICE_COUNT] PROGMEM = {
  0b0001000100010001,  // kick
  0b0001000000010000,  // snare
  0b0101010101010101,  // closed hat
  0b0100000000000000,  // open hat
};

static const int16_t sine_table[32] PROGMEM = {
  0, 49, 97, 142, 180, 213, 236, 249,
  255, 249, 236, 213, 180, 142, 97, 49,
  0, -49, -97, -142, -180, -213, -236, -249,
  -255, -249, -236, -213, -180, -142, -97, -49,
};

static volatile Voice voices[VOICE_COUNT] = {
  {.kind = KICK},
  {.kind = SNARE},
  {.kind = CLOSED_HAT},
  {.kind = OPEN_HAT},
};

static volatile uint8_t current_step;
static volatile uint32_t step_phase;

static uint32_t phase_increment(uint16_t frequency) {
  return (uint32_t)(((uint64_t)frequency << 32) / SAMPLE_RATE);
}

static void trigger_voice(Voice *voice) {
  voice->phase = 0;
  voice->active = 1;
  voice->envelope = 65535;
  voice->previous_noise = 0;
  switch (voice->kind) {
    case KICK:
      voice->phase_increment = phase_increment(150);
      voice->envelope_step = 16;
      break;
    case SNARE:
      voice->phase_increment = phase_increment(190);
      voice->envelope_step = 35;
      break;
    case CLOSED_HAT:
      voice->phase_increment = 0;
      voice->envelope_step = 52;
      break;
    case OPEN_HAT:
      voice->phase_increment = 0;
      voice->envelope_step = 8;
      break;
  }
}

static int16_t next_noise(Voice *voice) {
  uint32_t value = voice->random_state;
  if (value == 0) value = 0x13579bdf;
  value ^= value << 13;
  value ^= value >> 17;
  value ^= value << 5;
  voice->random_state = value;
  return (int16_t)(value >> 16);
}

static int16_t render_voice(Voice *voice) {
  if (!voice->active) return 0;

  int32_t sample = 0;
  switch (voice->kind) {
    case KICK: {
      uint8_t index = voice->phase >> 27;
      int16_t wave = pgm_read_word(&sine_table[index]) * 100;
      sample = (int32_t)wave * voice->envelope / 65535;
      if (voice->phase_increment > phase_increment(48)) {
        voice->phase_increment -= 2;
      }
      voice->phase += voice->phase_increment;
      break;
    }
    case SNARE: {
      int16_t tone = (voice->phase >> 31) ? 7000 : -7000;
      sample = ((int32_t)next_noise(voice) * 75 + tone) * voice->envelope / 65535;
      voice->phase += voice->phase_increment;
      break;
    }
    case CLOSED_HAT:
    case OPEN_HAT: {
      int16_t noise = next_noise(voice);
      sample = ((int32_t)noise - voice->previous_noise) * voice->envelope / 2;
      voice->previous_noise = noise;
      break;
    }
  }

  if (voice->envelope > voice->envelope_step) {
    voice->envelope -= voice->envelope_step;
  } else {
    voice->envelope = 0;
    voice->active = 0;
  }
  return (int16_t)sample;
}

static void render_sample(void) {
  int32_t mixed = 0;
  for (uint8_t index = 0; index < VOICE_COUNT; index++) {
    mixed += render_voice((Voice *)&voices[index]);
  }
  mixed = mixed / 3 + 128;
  if (mixed < 0) mixed = 0;
  if (mixed > 255) mixed = 255;
  OCR2B = (uint8_t)mixed;
}

ISR(TIMER1_COMPA_vect) {
  static const uint32_t step_period =
    ((uint32_t)SAMPLE_RATE * 60UL << 16) / (BPM * 4UL);

  step_phase += 65536UL;
  if (step_phase >= step_period) {
    step_phase -= step_period;
    for (uint8_t index = 0; index < VOICE_COUNT; index++) {
      if (pgm_read_word(&pattern[index]) & (1U << current_step)) {
        trigger_voice((Voice *)&voices[index]);
      }
    }
    current_step = (current_step + 1) % STEP_COUNT;
  }
  render_sample();
}

int main(void) {
  // PD3/OC2B is the PWM audio output. Feed it through a resistor/capacitor
  // and an amplifier before connecting a speaker.
  DDRD |= _BV(DDD3);
  TCCR2A = _BV(COM2B1) | _BV(WGM21) | _BV(WGM20);
  TCCR2B = _BV(CS20);
  OCR2B = 128;

  // Timer1 drives the audio sample rate at 8 MHz / 8 / 64 = 15,625 Hz.
  TCCR1A = 0;
  TCCR1B = _BV(WGM12) | _BV(CS11);
  OCR1A = 63;
  TIMSK1 = _BV(OCIE1A);

  sei();
  for (;;) {
  }
}
