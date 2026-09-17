#ifndef F_CPU
#define F_CPU 8000000UL
#endif

#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/pgmspace.h>

#include <stdint.h>

enum {
  SAMPLE_RATE = 16000,
  BPM = 101,
  SAMPLES_PER_STEP = SAMPLE_RATE * 60UL / (BPM * 4UL),
  STEP_COUNT = 16,
  VOICE_COUNT = 4,

  STATUS_LED = PB0, // DIP pin 14
  I2S_LRCLK = PB1, // DIP pin 15, Timer1 OC1A
  I2S_DATA = PD1,  // DIP pin 3, USART TXD
  I2S_BCLK = PD4,  // DIP pin 6, USART XCK
  OSC_CAL_OFFSET = 3,
  LED_TOGGLE_FRAMES = SAMPLE_RATE / 2,
  OUTPUT_LIMIT = 3200, // About 10% of signed 16-bit full scale.
};

typedef enum {
  KICK,
  SNARE,
  CLOSED_HAT,
  OPEN_HAT,
} VoiceKind;

// A compact, funky-drummer-inspired 16-step groove. This stores triggers,
// not recorded audio; every voice is synthesized by the ATmega in real time.
static const uint16_t pattern[VOICE_COUNT] PROGMEM = {
  0b0100010010001001, // kick
  0b0001000000010000, // snare
  0b0101010101010101, // closed hat
  0b1000000010000000, // open hat
};

static volatile uint8_t active_high_byte;
static volatile uint8_t active_low_byte;
static volatile uint8_t pending_high_byte;
static volatile uint8_t pending_low_byte;
static volatile uint8_t pending_sample_ready;

static uint16_t kick_phase;
static uint16_t kick_phase_step;
static uint16_t kick_envelope;
static uint16_t snare_envelope;
static uint16_t closed_hat_envelope;
static uint16_t open_hat_envelope;
static uint16_t noise_state = 0xace1;
static int8_t previous_noise;
static uint16_t samples_until_step;
static uint16_t led_frames_remaining = LED_TOGGLE_FRAMES;
static uint8_t current_step;

static int8_t next_noise(void) {
  uint16_t lsb = noise_state & 1U;
  noise_state >>= 1;
  if (lsb) {
    noise_state ^= 0xb400U;
  }
  return (int8_t)(noise_state >> 8);
}

static void trigger_step(void) {
  for (uint8_t voice = 0; voice < VOICE_COUNT; voice++) {
    if (!(pgm_read_word(&pattern[voice]) & (1U << current_step))) {
      continue;
    }

    switch ((VoiceKind)voice) {
      case KICK:
        kick_phase = 0;
        kick_phase_step = 760;
        kick_envelope = 30000;
        break;
      case SNARE:
        snare_envelope = 28000;
        break;
      case CLOSED_HAT:
        closed_hat_envelope = 22000;
        break;
      case OPEN_HAT:
        open_hat_envelope = 26000;
        break;
    }
  }

  current_step = (current_step + 1U) & (STEP_COUNT - 1U);
}

static int16_t render_audio_sample(void) {
  if (samples_until_step == 0) {
    trigger_step();
    samples_until_step = SAMPLES_PER_STEP;
  }
  samples_until_step--;

  int8_t noise = next_noise();
  int16_t mixed = 0;

  if (kick_envelope) {
    uint8_t phase = (uint8_t)(kick_phase >> 8);
    int16_t triangle = phase < 128 ? (int16_t)phase * 2 - 127
                                   : 383 - (int16_t)phase * 2;
    mixed += (triangle * (int16_t)(kick_envelope >> 8)) >> 4;
    kick_phase += kick_phase_step;
    if (kick_phase_step > 230) {
      kick_phase_step--;
    }
    kick_envelope = kick_envelope > 12 ? kick_envelope - 12 : 0;
  }

  if (snare_envelope) {
    int16_t snare = (int16_t)noise * (int16_t)(snare_envelope >> 8);
    int16_t tone = (kick_phase & 0x4000U) ? 2200 : -2200;
    mixed += (snare + tone) >> 4;
    snare_envelope = snare_envelope > 34 ? snare_envelope - 34 : 0;
  }

  int16_t bright_noise = (int16_t)noise - previous_noise;
  previous_noise = noise;

  if (closed_hat_envelope) {
    mixed += (bright_noise * (int16_t)(closed_hat_envelope >> 8)) >> 5;
    closed_hat_envelope = closed_hat_envelope > 120
                              ? closed_hat_envelope - 120
                              : 0;
  }

  if (open_hat_envelope) {
    mixed += (bright_noise * (int16_t)(open_hat_envelope >> 8)) >> 6;
    open_hat_envelope = open_hat_envelope > 18
                            ? open_hat_envelope - 18
                            : 0;
  }

  if (mixed > OUTPUT_LIMIT) {
    return OUTPUT_LIMIT;
  }
  if (mixed < -OUTPUT_LIMIT) {
    return -OUTPUT_LIMIT;
  }
  return mixed;
}

static void queue_audio_sample(int16_t sample) {
  // The leading zero is the one-bit delay required by standard I2S. Dropping
  // the quietest sample bit leaves 15 useful audio bits in each 16-bit slot.
  uint16_t i2s_word = (uint16_t)sample >> 1;
  pending_high_byte = (uint8_t)(i2s_word >> 8);
  pending_low_byte = (uint8_t)i2s_word;
  pending_sample_ready = 1;
}

// USART Master SPI mode has a transmit buffer. Filling it from this short ISR
// keeps BCLK continuous while synthesis runs in the main loop.
ISR(USART_UDRE_vect) {
  static uint8_t next_slot = 1;

  if (next_slot == 0 && pending_sample_ready) {
    active_high_byte = pending_high_byte;
    active_low_byte = pending_low_byte;
    pending_sample_ready = 0;
  }

  UDR0 = (next_slot & 1U) ? active_low_byte : active_high_byte;
  next_slot = (next_slot + 1U) & 3U;
}

int main(void) {
  DDRB |= _BV(STATUS_LED) | _BV(I2S_LRCLK);
  DDRD |= _BV(I2S_DATA) | _BV(I2S_BCLK);

  // The factory-calibrated internal oscillator is close to 8 MHz. This small
  // adjustment measured about 506 kHz BCLK and 15.72 kHz LRCLK on the test
  // ATmega, close enough to the MAX98357A's supported 16 kHz mode.
  OSCCAL = (uint8_t)(OSCCAL + OSC_CAL_OFFSET);

  // Hardware LRCLK: CPU clock / 512. Hardware BCLK below is CPU clock / 16,
  // keeping the required 32 BCLK cycles per stereo frame with no clock gaps.
  OCR1A = 255;
  TCNT1 = 0;
  TCCR1A = _BV(COM1A0);

  UBRR0 = 0;
  UCSR0C = _BV(UMSEL01) | _BV(UMSEL00);
  UCSR0B = _BV(TXEN0);
  UBRR0 = 7;

  int16_t first_sample = render_audio_sample();
  uint16_t first_word = (uint16_t)first_sample >> 1;
  active_high_byte = (uint8_t)(first_word >> 8);
  active_low_byte = (uint8_t)first_word;
  UDR0 = active_high_byte;

  TCCR1B = _BV(WGM12) | _BV(CS10);
  UCSR0B |= _BV(UDRIE0);
  sei();

  for (;;) {
    if (!pending_sample_ready) {
      queue_audio_sample(render_audio_sample());

      led_frames_remaining--;
      if (led_frames_remaining == 0) {
        PORTB ^= _BV(STATUS_LED);
        led_frames_remaining = LED_TOGGLE_FRAMES;
      }
    }
  }
}
