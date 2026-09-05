#include <util/atomic.h>

#include <avr/interrupt.h>
#include <avr/io.h>

#include "timer.h"

volatile uint32_t system_ticks = 0;

void timer_init(void) {
  TCNT1 = 0;

  // CTC mode
  TCCR1 = (1 << CTC1);

  // 1 MHz / 8 = 125 kHz
  // 125 timer counts = 1 ms
  OCR1A = 124;
  OCR1C = 124;

  TIMSK |= (1 << OCIE1A);

  TCCR1 |= (1 << CS12); // Start Timer1, prescaler /8

  sei(); // Global interrupts on
}

ISR(TIMER1_COMPA_vect) { system_ticks++; }

uint32_t timer_now(void) {
  uint32_t ticks;

  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) { ticks = system_ticks; }

  return ticks;
}
