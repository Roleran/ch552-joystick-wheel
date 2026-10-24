#include <stdint.h>

#include "board.h"
#include "userUsbHidKeyboardMouse/USBHIDKeyboardMouse.h"
#include "ws2812.h"

/*
 * CH552E 单摇杆固件
 *
 *   P1.4  上下。滚动方向，鼠标模式里是上下
 *   P1.5  左右。只加快滚动，单独扳它不滚动；鼠标模式里是左右
 *   P1.6  按下切换模式，只在按下沿切换一次
 *   P1.7  WS2812，滚动蓝色，鼠标琥珀色
 *
 * 滚动间隔：刚出死区 180ms，VY 打满 72ms，VY 和 VX 都打满压到 36ms。
 */

#define ADC_CENTER 128
#define DEAD_ZONE 20
#define SCROLL_STEP 1
#define SCROLL_Y_SIGN 1
#define MOUSE_X_SIGN 1
#define MOUSE_Y_SIGN -1

#define SCROLL_BASE_MS 180
#define SCROLL_Y_GAIN 108
#define SCROLL_X_GAIN 36
#define SCROLL_MIN_MS 36

#define MOUSE_DIVISOR 8
#define MOUSE_INTERVAL_MS 16
#define BUTTON_DEBOUNCE_MS 30

/* 灯色分量范围 0–255。上电默认是滚动蓝，这颗灯在个位数也会很亮。 */
#define LED_SCROLL_R 0
#define LED_SCROLL_G 0
#define LED_SCROLL_B 3
#define LED_MOUSE_R 4
#define LED_MOUSE_G 2
#define LED_MOUSE_B 0

static __bit mouseMode = 0;
static __bit keyStable = 1;
static __bit keyLast = 1;
static uint32_t keyChangeMs = 0;
static uint32_t actionMs = 0;

static void showMode(void) {
  if (mouseMode) {
    ws2812Show(LED_MOUSE_R, LED_MOUSE_G, LED_MOUSE_B);
  } else {
    ws2812Show(LED_SCROLL_R, LED_SCROLL_G, LED_SCROLL_B);
  }
}

static void clearPointer(void) {
  Mouse_move(0, 0);
  Mouse_scroll(0);
}

static int16_t stickValue(uint8_t channel) {
  int16_t value = (int16_t)analogReadChannel(channel) - ADC_CENTER;
  if (value > -DEAD_ZONE && value < DEAD_ZONE) {
    return 0;
  }
  return value;
}

static uint16_t scrollInterval(int16_t vx, int16_t vy) {
  int16_t magY = vy < 0 ? (int16_t)-vy : vy;
  int16_t magX = vx < 0 ? (int16_t)-vx : vx;
  int16_t interval = SCROLL_BASE_MS;

  interval -= (int16_t)(((int32_t)(magY - DEAD_ZONE) * SCROLL_Y_GAIN) / 108);
  if (magX > DEAD_ZONE) {
    interval -= (int16_t)(((int32_t)(magX - DEAD_ZONE) * SCROLL_X_GAIN) / 108);
  }
  if (interval < SCROLL_MIN_MS) {
    interval = SCROLL_MIN_MS;
  }
  return (uint16_t)interval;
}

static void pollButton(void) {
  __bit pressed = digitalReadP1(BIT_KEY) == 0;
  uint32_t now = millis();

  if (pressed != keyLast) {
    keyLast = pressed;
    keyChangeMs = now;
  }
  if ((uint32_t)(now - keyChangeMs) < BUTTON_DEBOUNCE_MS || pressed == keyStable) {
    return;
  }

  keyStable = pressed;
  if (!pressed) {
    return;
  }

  mouseMode = !mouseMode;
  clearPointer();
  showMode();
  actionMs = now;
}

static void runScroll(int16_t vx, int16_t vy) {
  int8_t step;
  if (vy == 0 || (uint32_t)(millis() - actionMs) < scrollInterval(vx, vy)) {
    return;
  }

  step = (int8_t)(vy > 0 ? SCROLL_STEP : -SCROLL_STEP);
  if (SCROLL_Y_SIGN < 0) {
    step = (int8_t)-step;
  }
  Mouse_scroll(step);
  actionMs = millis();
}

static void runMouse(int16_t vx, int16_t vy) {
  int16_t x;
  int16_t y;
  if ((uint32_t)(millis() - actionMs) < MOUSE_INTERVAL_MS) {
    return;
  }

  x = (int16_t)((vx * MOUSE_X_SIGN) / MOUSE_DIVISOR);
  y = (int16_t)((vy * MOUSE_Y_SIGN) / MOUSE_DIVISOR);
  if (x > 127) {
    x = 127;
  }
  if (x < -127) {
    x = -127;
  }
  if (y > 127) {
    y = 127;
  }
  if (y < -127) {
    y = -127;
  }

  Mouse_move((int8_t)x, (int8_t)y);
  actionMs = millis();
}

void setup(void) {
  /* 先拉低并写入暗色，避免数据线在 USB 初始化前抖动，灯珠锁存成高亮。 */
  P1_7 = 0;
  delayMicroseconds(80);
  showMode();

  pinModeP1(BIT_VX, PIN_FLOAT);
  pinModeP1(BIT_VY, PIN_FLOAT);
  pinModeP1(BIT_KEY, PIN_PULLUP);

  USBInit();
}

void loop(void) {
  /* 实机左右在 P1.5、上下在 P1.4，读入时对调。 */
  int16_t vx = stickValue(ADC_VY);
  int16_t vy = stickValue(ADC_VX);

  pollButton();
  if (mouseMode) {
    runMouse(vx, vy);
  } else {
    runScroll(vx, vy);
  }
}
