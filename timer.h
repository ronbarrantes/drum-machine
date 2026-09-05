#ifndef TIMER_H
#define TIMER_H

#include <stdbool.h>
#include <stdint.h>

void timer_init(void);
uint32_t timer_now(void);

static inline bool timer_has_elapsed(uint32_t started_at, uint32_t now,
                                     uint16_t duration_ms) {
  return (uint32_t)(now - started_at) >= duration_ms;
}
#endif
