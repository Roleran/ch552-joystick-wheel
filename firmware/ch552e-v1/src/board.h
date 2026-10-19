#ifndef BOARD_H
#define BOARD_H

#include <stdint.h>

#include "include/ch5xx.h"

/* P1.4 / P1.5 是 ADC，必须高阻。P1.6 用内部上拉。 */
#define PIN_FLOAT 0
#define PIN_PULLUP 1

#define BIT_VX 0x10
#define BIT_VY 0x20
#define BIT_KEY 0x40
#define ADC_VX 1
#define ADC_VY 2

void init(void);
uint32_t millis(void);
void delayMicroseconds(__data uint16_t us);
void pinModeP1(uint8_t bit, uint8_t mode);
uint8_t digitalReadP1(uint8_t bit);
uint8_t analogReadChannel(uint8_t channel);

#endif
