#ifndef F_CPU
#define F_CPU 8000000UL
#endif

#include <avr/io.h>
#include <avr/pgmspace.h>

#include <stdint.h>

enum {
  STATUS_LED = PB0,
  SPI_SS = PB2,
  I2S_DATA = PB3,
  I2S_BCLK = PB5,
  I2S_LRCLK = PD2,
  TONE_SAMPLE_COUNT = 32,
  LED_TOGGLE_FRAMES = 7812,
};

// Approximately 20% of full-scale. Each word includes the leading zero bit
// required by I2S; the MAX98357A reads the signed sample starting one BCLK
// after LRCLK changes.
#define I2S_WORD(sample) ((uint16_t)(int16_t)(sample) >> 1)
static const uint16_t tone_words[TONE_SAMPLE_COUNT] PROGMEM = {
  I2S_WORD(0),     I2S_WORD(1225),  I2S_WORD(2425),  I2S_WORD(3550),
  I2S_WORD(4500),  I2S_WORD(5325),  I2S_WORD(5900),  I2S_WORD(6225),
  I2S_WORD(6375),  I2S_WORD(6225),  I2S_WORD(5900),  I2S_WORD(5325),
  I2S_WORD(4500),  I2S_WORD(3550),  I2S_WORD(2425),  I2S_WORD(1225),
  I2S_WORD(0),     I2S_WORD(-1225), I2S_WORD(-2425), I2S_WORD(-3550),
  I2S_WORD(-4500), I2S_WORD(-5325), I2S_WORD(-5900), I2S_WORD(-6225),
  I2S_WORD(-6375), I2S_WORD(-6225), I2S_WORD(-5900), I2S_WORD(-5325),
  I2S_WORD(-4500), I2S_WORD(-3550), I2S_WORD(-2425), I2S_WORD(-1225),
};
#undef I2S_WORD

static inline void spi_send(uint8_t value) {
  SPDR = value;
  while (!(SPSR & _BV(SPIF))) {
  }
}

int main(void) {
  DDRB |= _BV(STATUS_LED) | _BV(SPI_SS) | _BV(I2S_DATA) | _BV(I2S_BCLK);
  PORTB |= _BV(SPI_SS);
  DDRD |= _BV(I2S_LRCLK);

  // SPI master at 500 kHz: 8 MHz / 16.
  SPCR = _BV(SPE) | _BV(MSTR) | _BV(SPR0);

  uint8_t sample_index = 0;
  uint16_t led_frames = 0;
  uint16_t word = pgm_read_word(&tone_words[0]);

  for (;;) {
    uint8_t high_byte = (uint8_t)(word >> 8);
    uint8_t low_byte = (uint8_t)word;

    PORTD &= (uint8_t)~_BV(I2S_LRCLK);
    spi_send(high_byte);
    spi_send(low_byte);

    PORTD |= _BV(I2S_LRCLK);
    spi_send(high_byte);

    // Load the next sample while the final byte is being shifted out. This
    // keeps sound generation from creating long gaps in BCLK.
    SPDR = low_byte;
    sample_index = (sample_index + 1) & (TONE_SAMPLE_COUNT - 1);
    word = pgm_read_word(&tone_words[sample_index]);

    led_frames++;
    if (led_frames >= LED_TOGGLE_FRAMES) {
      PORTB ^= _BV(STATUS_LED);
      led_frames = 0;
    }

    while (!(SPSR & _BV(SPIF))) {
    }
  }
}
