#include <stdint.h>

#include "board.h"

/*
 * 启动文件只做三件事：
 * 1. 把毫秒计数器固定在寄存器组 1，board.c 的定时器中断按这个地址累加
 * 2. 把 USB 中断转给 HID 处理函数
 * 3. 调用 init / setup / loop
 */

void USBInterrupt(void);
void setup(void);
void loop(void);

/* 中断向量表只在含有 main 的文件里生成。定时器函数在 board.c。 */
void Timer0Interrupt(void) __interrupt(INT_NO_TMR0);

__idata __at(0x08) volatile uint32_t timer0_overflow_count = 0;
__idata __at(0x0C) volatile uint8_t timer0_overflow_count_5th_byte = 0;

void DeviceUSBInterrupt(void) __interrupt(INT_NO_USB) { USBInterrupt(); }

void main(void) {
  init();
  setup();
  for (;;) {
    loop();
  }
}
