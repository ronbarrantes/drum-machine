#ifndef F_CPU
#define F_CPU 8000000UL
#endif

#include <avr/interrupt.h>
#include <avr/io.h>

#include <stdint.h>

enum {
  STATUS_LED = PB0, // DIP pin 14
  I2S_LRCLK = PB1, // DIP pin 15, Timer1 OC1A
  I2S_DATA = PD1,  // DIP pin 3, USART TXD
  I2S_BCLK = PD4,  // DIP pin 6, USART XCK

  // The internal oscillator is calibrated for roughly 8 MHz at the factory.
  // A small increase puts the 15.625 kHz frame clock closer to the MAX98357A's
  // supported 16 kHz rate. The exact result varies slightly between chips.
  OSC_CAL_OFFSET = 3,
};

static volatile uint8_t next_byte;
static volatile uint8_t words_until_polarity_change = 32;
static volatile uint8_t positive_half_cycle = 1;

// Feed the USART transmit buffer before the current byte finishes. In Master
// SPI mode this keeps BCLK running continuously, unlike the ordinary SPI port
// on the ATmega328P, which pauses while software loads each new byte.
ISR(USART_UDRE_vect) {
  UDR0 = next_byte;

  // Repeat a 16-bit positive or negative sample in both stereo slots. Changing
  // polarity every 64 bytes produces a quiet tone near 500 Hz.
  static uint8_t high_byte_is_next = 1;
  if (high_byte_is_next) {
    next_byte = positive_half_cycle ? 0x00 : 0x78;
  } else {
    next_byte = positive_half_cycle ? 0x08 : 0x00;

    words_until_polarity_change--;
    if (words_until_polarity_change == 0) {
      positive_half_cycle ^= 1;
      words_until_polarity_change = 32;
    }
  }
  high_byte_is_next ^= 1;
}

int main(void) {
  // A solid LED means the test firmware is running. It never changes while
  // audio is streaming, so it cannot disturb the clock timing.
  DDRB |= _BV(STATUS_LED) | _BV(I2S_LRCLK);
  PORTB |= _BV(STATUS_LED);
  DDRD |= _BV(I2S_DATA) | _BV(I2S_BCLK);

  OSCCAL = (uint8_t)(OSCCAL + OSC_CAL_OFFSET);

  // Timer1 toggles OC1A every 256 CPU clocks. At approximately 8.192 MHz this
  // produces a 16 kHz LRCLK with a 50% duty cycle.
  OCR1A = 255;
  TCNT1 = 0;
  TCCR1A = _BV(COM1A0);

  // USART Master SPI mode. UBRR=7 divides the CPU clock by 16, producing a
  // roughly 512 kHz continuous BCLK. MSB-first and clock mode 0 match the
  // MAX98357A's rising-edge I2S input.
  UBRR0 = 0;
  UCSR0C = _BV(UMSEL01) | _BV(UMSEL00);
  UCSR0B = _BV(TXEN0);
  UBRR0 = 7;

  next_byte = 0x00;
  UDR0 = 0x08;

  // Start LRCLK and the transmit-buffer interrupt only after the first audio
  // byte is queued. Both clocks then remain continuous.
  TCCR1B = _BV(WGM12) | _BV(CS10);
  UCSR0B |= _BV(UDRIE0);
  sei();

  for (;;) {
  }
}
