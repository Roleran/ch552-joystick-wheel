#include <stdint.h>

#include "board.h"
#include "include/ch5xx.h"
#include "ws2812.h"

/*
 * 时序只覆盖 CH55xduino 里这两档内部时钟：
 *   24 MHz（5V 板子的默认项）
 *   16 MHz
 * 位时间按 1T 8051 估算。灯不亮或发白时，先确认时钟菜单，再改下面的 nop 个数。
 * 发送全程关中断，大约几十微秒，USB 轮询是 10ms，不会把枚举打断。
 */

__data uint8_t wsBuf[3];

#if (F_CPU == 24000000L)

static void wsSendBits(void) __naked {
  __asm
    mov r0, #_wsBuf
    mov r2, #3
00001$:
    mov a, @r0
    mov r1, #8
00002$:
    rlc a
    jc 00003$
    setb 0x97
    nop
    nop
    nop
    nop
    nop
    clr 0x97
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    sjmp 00004$
00003$:
    setb 0x97
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    clr 0x97
    nop
    nop
00004$:
    djnz r1, 00002$
    inc r0
    djnz r2, 00001$
    ret
  __endasm;
}

#elif (F_CPU == 16000000L)

static void wsSendBits(void) __naked {
  __asm
    mov r0, #_wsBuf
    mov r2, #3
00001$:
    mov a, @r0
    mov r1, #8
00002$:
    rlc a
    jc 00003$
    setb 0x97
    nop
    nop
    nop
    clr 0x97
    nop
    nop
    nop
    nop
    nop
    sjmp 00004$
00003$:
    setb 0x97
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    clr 0x97
    nop
00004$:
    djnz r1, 00002$
    inc r0
    djnz r2, 00001$
    ret
  __endasm;
}

#else
#error "WS2812 timing supports only 16MHz or 24MHz"
#endif

void ws2812Show(uint8_t red, uint8_t green, uint8_t blue) {
  /* WS2812B 线上的顺序是绿、红、蓝。 */
  wsBuf[0] = green;
  wsBuf[1] = red;
  wsBuf[2] = blue;

  EA = 0;
  wsSendBits();
  EA = 1;

  /* 拉低超过 80us，灯珠才把这一帧锁存。 */
  P1_7 = 0;
  delayMicroseconds(80);
}
