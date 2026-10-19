#ifndef WS2812_H
#define WS2812_H

#include <stdint.h>

/* 单颗 WS2812B，数据线在 P1.7。颜色按 RGB 传入，发送时改成灯珠要的 GRB。 */
void ws2812Show(uint8_t red, uint8_t green, uint8_t blue);

#endif
