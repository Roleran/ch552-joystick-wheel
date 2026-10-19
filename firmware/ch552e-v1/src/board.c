#include "board.h"

/*
 * 只保留这颗板子用到的底层：
 * 内部 24MHz、Timer0 毫秒、P1 口输入、两路 ADC。
 * 定时器每 125us 溢出一次，8 次是 1ms。
 */

extern __idata volatile uint32_t timer0_overflow_count;
extern __idata volatile uint8_t timer0_overflow_count_5th_byte;

void Timer0Interrupt(void) __interrupt(INT_NO_TMR0) __using(1) {
  __asm__("    inc r0                                      \n"
          "    cjne r0,#0,boardTimerOver$                  \n"
          "    inc r1                                      \n"
          "    cjne r1,#0,boardTimerOver$                  \n"
          "    inc r2                                      \n"
          "    cjne r2,#0,boardTimerOver$                  \n"
          "    inc r3                                      \n"
          "    cjne r3,#0,boardTimerOver$                  \n"
          "    inc r4                                      \n"
          "boardTimerOver$:                                \n");
}

uint32_t millis(void) {
  uint8_t saved = EA;
  uint32_t count;
  uint8_t high;

  EA = 0;
  count = timer0_overflow_count;
  high = timer0_overflow_count_5th_byte;
  EA = saved;
  return (count >> 3) | ((uint32_t)high << 29);
}

void delayMicroseconds(__data uint16_t us) {
  us;
  __asm__(".even                                    \n"
          "    mov  r6, dpl                         \n"
          "    mov  r7, dph                         \n"
          "    clr  c                               \n"
          "    mov  a,#0x01                         \n"
          "    subb a, r6                           \n"
          "    clr  a                               \n"
          "    subb a, r7                           \n"
          "    jc boardSkip0$                       \n"
          "    ret                                  \n"
          "    nop                                  \n"
          "boardSkip0$:                             \n"
          "    clr  c                               \n"
          "    mov  a, #0x02                        \n"
          "    subb a, r6                           \n"
          "    mov  r6, a                           \n"
          "    mov  a, #0x00                        \n"
          "    subb a, r7                           \n"
          "    mov  r7, a                           \n"
          "    nop                                  \n"
          "    cjne r6,#0x00,boardLoop$             \n"
          "    cjne r7,#0x00,boardLoop$             \n"
          "    nop                                  \n"
          "    ret                                  \n"
          "boardLoop$:                              \n"
          "    nop \n nop \n nop \n nop \n nop \n    "
          "    nop \n "
          "boardLoop2$:                             \n"
          "    nop \n nop \n nop \n nop \n nop \n    "
          "    nop \n nop \n nop \n nop \n nop \n    "
          "    nop \n                                "
          "    inc  r6                              \n"
          "    cjne r6, #0,boardLoop$               \n"
          "    inc  r7                              \n"
          "    cjne r7, #0,boardLoop2$              \n"
          "    nop                                  \n");
}

void pinModeP1(uint8_t bit, uint8_t mode) {
  if (mode == PIN_PULLUP) {
    P1_MOD_OC |= bit;
    P1_DIR_PU |= bit;
    return;
  }
  P1_MOD_OC &= ~bit;
  P1_DIR_PU &= ~bit;
}

uint8_t digitalReadP1(uint8_t bit) { return (P1 & bit) ? 1 : 0; }

uint8_t analogReadChannel(uint8_t channel) {
  ADC_CFG = bADC_EN | bADC_CLK;
  ADC_CTRL = (ADC_CTRL & ~0x03) | (channel & 0x03);
  ADC_START = 1;
  while (ADC_START) {
  }
  return ADC_DATA;
}

void init(void) {
  SAFE_MOD = 0x55;
  SAFE_MOD = 0xAA;
  CLOCK_CFG = (CLOCK_CFG & ~MASK_SYS_CK_SEL) | 0x06;
  SAFE_MOD = 0x00;
  delayMicroseconds(5000);

  TMOD = (TMOD & ~0x0F) | bT0_M1;
  T2MOD &= ~bT0_CLK;
  TH0 = 6;
  TF0 = 0;
  ET0 = 1;
  TR0 = 1;
  EA = 1;
}
