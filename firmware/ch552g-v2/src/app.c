#include <stdint.h>

#include "board.h"
#include "userUsbHidKeyboardMouse/USBHIDKeyboardMouse.h"

/*
 * CH552G 双摇杆。
 *
 * 摇杆 1：P1.4 上下，决定滚轮方向；P1.5 左右，只加快滚动。
 *         P1.6 切换摇杆 2 的按键。
 * 摇杆 2：P1.1 上下、P3.2 左右，移动鼠标。轴向和 v1 实机一致，读入时对调。
 *         P1.7 默认鼠标左键，切换后按住发送回车。
 * P3.0 上、P3.1 下、P3.3 左、P3.4 右，始终是方向键。
 *
 * 滚动间隔：刚出死区 180ms，P1.4 打满 72ms，两轴都打满压到 36ms。
 */

#define ADC_CENTER 128
#define DEAD_ZONE 20
#define SCROLL_STEP 1
#define SCROLL_Y_SIGN 1
#define MOUSE_X_SIGN 1
#define MOUSE_Y_SIGN -1

#define BIT_S1_X 0x10
#define BIT_S1_Y 0x20
#define BIT_S1_KEY 0x40
#define BIT_S2_X 0x02
#define BIT_S2_KEY 0x80
#define ADC_S1_X 1
#define ADC_S1_Y 2
#define ADC_S2_X 0
#define ADC_S2_Y 3

#define BIT_KEY_UP 0x01
#define BIT_KEY_DOWN 0x02
#define BIT_S2_Y 0x04
#define BIT_KEY_LEFT 0x08
#define BIT_KEY_RIGHT 0x10

#define SCROLL_BASE_MS 180
#define SCROLL_Y_GAIN 108
#define SCROLL_X_GAIN 36
#define SCROLL_MIN_MS 36

#define MOUSE_DIVISOR 8
#define MOUSE_INTERVAL_MS 16
#define BUTTON_DEBOUNCE_MS 30

#define KEY_MODE 0
#define KEY_STICK2 1
#define KEY_UP 2
#define KEY_DOWN 3
#define KEY_LEFT 4
#define KEY_RIGHT 5
#define KEY_COUNT 6

static __bit enterMode = 0;
static uint8_t keyStable[KEY_COUNT];
static uint8_t keyLast[KEY_COUNT];
static uint32_t keyChangeMs[KEY_COUNT];
static uint32_t scrollMs = 0;
static uint32_t mouseMs = 0;

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

/* 1 = 按下沿，2 = 松开沿，0 = 没有新动作。 */
static uint8_t pollKey(uint8_t index, uint8_t down) {
  uint32_t now = millis();

  if (down != keyLast[index]) {
    keyLast[index] = down;
    keyChangeMs[index] = now;
  }
  if ((uint32_t)(now - keyChangeMs[index]) < BUTTON_DEBOUNCE_MS ||
      down == keyStable[index]) {
    return 0;
  }

  keyStable[index] = down;
  return down ? 1 : 2;
}

static void releaseStick2Button(void) {
  Mouse_release(MOUSE_LEFT);
  Keyboard_release(KEY_RETURN);
}

static void onModePress(void) {
  enterMode = !enterMode;
  releaseStick2Button();
}

static void onStick2Edge(uint8_t edge) {
  if (edge == 0) {
    return;
  }
  if (enterMode) {
    if (edge == 1) {
      Keyboard_press(KEY_RETURN);
    } else {
      Keyboard_release(KEY_RETURN);
    }
    return;
  }
  if (edge == 1) {
    Mouse_press(MOUSE_LEFT);
  } else {
    Mouse_release(MOUSE_LEFT);
  }
}

static void onArrowEdge(uint8_t edge, uint8_t keyCode) {
  if (edge == 1) {
    Keyboard_press(keyCode);
  } else if (edge == 2) {
    Keyboard_release(keyCode);
  }
}

static void runScroll(int16_t vx, int16_t vy) {
  int8_t step;
  if (vy == 0 || (uint32_t)(millis() - scrollMs) < scrollInterval(vx, vy)) {
    return;
  }

  step = (int8_t)(vy > 0 ? SCROLL_STEP : -SCROLL_STEP);
  if (SCROLL_Y_SIGN < 0) {
    step = (int8_t)-step;
  }
  Mouse_scroll(step);
  scrollMs = millis();
}

static void runMouse(int16_t vx, int16_t vy) {
  int16_t x;
  int16_t y;
  if ((uint32_t)(millis() - mouseMs) < MOUSE_INTERVAL_MS) {
    return;
  }
  mouseMs = millis();

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
  if (x == 0 && y == 0) {
    return;
  }
  Mouse_move((int8_t)x, (int8_t)y);
}

void setup(void) {
  pinModeP1(BIT_S1_X, PIN_FLOAT);
  pinModeP1(BIT_S1_Y, PIN_FLOAT);
  pinModeP1(BIT_S2_X, PIN_FLOAT);
  pinModeP1(BIT_S1_KEY, PIN_PULLUP);
  pinModeP1(BIT_S2_KEY, PIN_PULLUP);

  pinModeP3(BIT_S2_Y, PIN_FLOAT);
  pinModeP3(BIT_KEY_UP, PIN_PULLUP);
  pinModeP3(BIT_KEY_DOWN, PIN_PULLUP);
  pinModeP3(BIT_KEY_LEFT, PIN_PULLUP);
  pinModeP3(BIT_KEY_RIGHT, PIN_PULLUP);

  USBInit();
}

void loop(void) {
  /* 和 v1 一样：标成 X 的脚是上下，标成 Y 的脚是左右。 */
  int16_t s1x = stickValue(ADC_S1_Y);
  int16_t s1y = stickValue(ADC_S1_X);
  int16_t s2x = stickValue(ADC_S2_Y);
  int16_t s2y = stickValue(ADC_S2_X);
  uint8_t edge;

  edge = pollKey(KEY_MODE, digitalReadP1(BIT_S1_KEY) == 0);
  if (edge == 1) {
    onModePress();
  }
  onStick2Edge(pollKey(KEY_STICK2, digitalReadP1(BIT_S2_KEY) == 0));
  onArrowEdge(pollKey(KEY_UP, digitalReadP3(BIT_KEY_UP) == 0), KEY_UP_ARROW);
  onArrowEdge(pollKey(KEY_DOWN, digitalReadP3(BIT_KEY_DOWN) == 0), KEY_DOWN_ARROW);
  onArrowEdge(pollKey(KEY_LEFT, digitalReadP3(BIT_KEY_LEFT) == 0), KEY_LEFT_ARROW);
  onArrowEdge(pollKey(KEY_RIGHT, digitalReadP3(BIT_KEY_RIGHT) == 0), KEY_RIGHT_ARROW);

  runScroll(s1x, s1y);
  runMouse(s2x, s2y);
}
