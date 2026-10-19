#ifndef BOARD_H
#define BOARD_H

#include <stdint.h>

#include "include/ch5xx.h"

/* ADC 脚用 PIN_FLOAT。按键用 PIN_PULLUP，另一端接地。 */
#define PIN_FLOAT 0
#define PIN_PULLUP 1

void init(void);
uint32_t millis(void);
void delayMicroseconds(__data uint16_t us);
void pinModeP1(uint8_t bit, uint8_t mode);
void pinModeP3(uint8_t bit, uint8_t mode);
uint8_t digitalReadP1(uint8_t bit);
uint8_t digitalReadP3(uint8_t bit);
uint8_t analogReadChannel(uint8_t channel);

#endif
