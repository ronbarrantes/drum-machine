#ifndef F_CPU
#define F_CPU 8000000UL
#endif

#include <avr/interrupt.h>
#include <avr/io.h>

#include <stdint.h>

enum {
  TEST_PORTB_PINS = _BV(PB0) | _BV(PB3) | _BV(PB5),
  TEST_PORTD_PINS = _BV(PD2),
};

ISR(TIMER0_COMPA_vect) {
  static uint16_t milliseconds;

  milliseconds++;
  if (milliseconds >= 500) {
    PORTB ^= TEST_PORTB_PINS;
    PORTD ^= TEST_PORTD_PINS;
    milliseconds = 0;
  }
}

int main(void) {
  DDRB |= TEST_PORTB_PINS;
  DDRD |= TEST_PORTD_PINS;

  // Timer0: 1 ms interrupt at 8 MHz / 64 / 125.
  TCCR0A = _BV(WGM01);
  TCCR0B = _BV(CS01) | _BV(CS00);
  OCR0A = 124;
  TIMSK0 = _BV(OCIE0A);

  sei();
  for (;;) {
  }
}
