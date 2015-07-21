#ifndef VIBRATOR_H
#define VIBRATOR_H

#include <stdint.h>
#include <stdbool.h>

void vibrator_init(void);
void vibrator_open(void);
void vibrator_close(void);
void vibrator_tick(uint8_t nums);

#endif
