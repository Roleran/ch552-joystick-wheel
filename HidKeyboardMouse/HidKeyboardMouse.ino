/*
  cli board options: usb_settings=user148
*/

#ifndef USER_USB_RAM
#error "This example needs to be compiled with a USER USB setting"
#endif

#include "src/userUsbHidKeyboardMouse/USBHIDKeyboardMouse.h"

#define BUTTON_PIN 16
#define ADC_X_PIN 15
#define ADC_Y_PIN 14

// 滚轮控制参数
#define DEAD_ZONE 20          // 死区阈值
#define MIN_INTERVAL 80       // 最快30ms
#define MAX_INTERVAL 200      // 最慢100ms
#define SCROLL_STEP_SIZE 1    // 每次滚动1个单位

// 结构体存储每个轴的滚动状态
typedef struct {
    uint32_t lastScrollTime;  // 上次滚动时间
    int8_t scrollDirection;   // 滚动方向 (1 或 -1)
    bool isActive;            // 是否处于活动状态
    int16_t lastValue;        // 存储上次的ADC值
} ScrollAxisState;

ScrollAxisState xScrollState = {0, 0, false, 0};
ScrollAxisState yScrollState = {0, 0, false, 0};

bool enable_double_click = true;

// 函数声明
void handle_single_click();
void handle_double_click();
void handle_key_state(bool key_state, bool is_double_click);
void updateScrollState(ScrollAxisState* state, int16_t value);
void handleActiveScroll(ScrollAxisState* state);

void setup()
{
  USBInit();
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(ADC_X_PIN, INPUT);
  pinMode(ADC_Y_PIN, INPUT);
}

void loop()
{
  // 按键处理
  if (enable_double_click) {
    handle_double_click();
  } else {
    handle_single_click();
  }

  // 每60ms更新一次ADC值并更新滚动状态
  static uint32_t lastAdcUpdate = 0;
  const uint32_t adcUpdateInterval = 60;
  
  if (millis() - lastAdcUpdate >= adcUpdateInterval) {
    lastAdcUpdate = millis();
    
    // 读取并更新X轴状态
    int16_t xValue = analogRead(ADC_X_PIN) ; // 转换为0-255范围
    int16_t xMapped = map(xValue, 0, 255, -128, 128);
    updateScrollState(&xScrollState, xMapped);
    
    // 读取并更新Y轴状态
    int16_t yValue = analogRead(ADC_Y_PIN) ; // 转换为0-255范围
    int16_t yMapped = map(yValue, 0, 255, -128, 128);
    updateScrollState(&yScrollState, yMapped);
  }

  // 处理X轴滚动
  handleActiveScroll(&xScrollState);
  
  // 处理Y轴滚动
  handleActiveScroll(&yScrollState);
}

/**
 * 更新轴的滚动状态
 * @param state 要更新的轴状态
 * @param value 当前轴的ADC映射值 (-128 到 128)
 */
void updateScrollState(ScrollAxisState* state, int16_t value) {
    state->lastValue = value;
    int16_t absValue = abs(value);
    
    if (absValue < DEAD_ZONE) {
        // 在死区内，禁用滚动
        state->isActive = false;
    } else {
        // 在死区外，激活滚动
        state->isActive = true;
        state->scrollDirection = (value > 0) ? 1 : -1;
    }
}

/**
 * 处理活动轴的滚动事件
 * @param state 要处理的轴状态
 */
void handleActiveScroll(ScrollAxisState* state) {
    if (!state->isActive) return;
    
    // 获取实际偏移量（减去死区）
    int16_t effectiveOffset = abs(state->lastValue) - DEAD_ZONE;
    
    // 计算滚动间隔：偏移越大，间隔越小（滚动越快）
    int16_t scrollInterval = MAX_INTERVAL - effectiveOffset;
    
    // 确保间隔在设定范围内
    if (scrollInterval < MIN_INTERVAL) scrollInterval = MIN_INTERVAL;
    else if (scrollInterval > MAX_INTERVAL) scrollInterval = MAX_INTERVAL;
    
    // 检查是否到达滚动时间
    if (millis() - state->lastScrollTime >= scrollInterval) {
        Mouse_scroll(state->scrollDirection * SCROLL_STEP_SIZE);
        state->lastScrollTime = millis();
    }
}


void handle_single_click()
{
  static bool buttonPressPrev = false;
  bool buttonPress = !digitalRead(BUTTON_PIN);
  unsigned long currentTime = millis();
  static unsigned long debounceTime = 0;

  if (buttonPress != buttonPressPrev && currentTime - debounceTime > 10)
  {
    debounceTime = currentTime;
    buttonPressPrev = buttonPress;
    handle_key_state(buttonPress, false); // 按键处理函数
  }
}

void handle_double_click()
{
  // 使用第一次按下与第二次按下的时间差区分单击和双击
  static uint16_t timeDiff = 200;
  const uint16_t timeDiffMin = 200;
  const uint16_t timeDiffMax = 350;

  // 单击按下事件定时
  static unsigned long startTimer1 = 0;
  // 双击释放事件定时
  static unsigned long startTimer2 = 0;

  static bool buttonPressPrev = false;
  bool buttonPress = !digitalRead(BUTTON_PIN);
  unsigned long currentTime = millis();
  static unsigned long debounceTime = 0;

  if (buttonPress != buttonPressPrev && currentTime - debounceTime > 10)
  {
    buttonPressPrev = buttonPress;
    debounceTime = currentTime;

    if (buttonPress)
    {
      if (startTimer1 == 0 && startTimer2 == 0)
      {
        // 开始延迟触发单击按下和释放事件
        startTimer1 = currentTime + timeDiff;
        startTimer2 = currentTime + timeDiff;
      }
      else if (startTimer1 != 0 && startTimer2 != 0)
      {
        // 取消单击定时器，触发双击按下事件
        startTimer1 = 0;
        startTimer2 = 0;
        handle_key_state(true, true);
      }
    }
    else
    {
      if (startTimer2 != 0)
      {
        startTimer2 = currentTime;
      }
      else
      {
        handle_key_state(false, true);
        timeDiff = timeDiffMax;
      }
    }
  }
  else
  {
    if (buttonPress && startTimer2 != 0)
    {
      startTimer2 = currentTime + 1;
    }
  }

  // 单击按下计时器
  if (startTimer1 != 0 && currentTime >= startTimer1)
  {
    startTimer1 = 0;
    handle_key_state(true, false);
  }
  // 单击释放计时器
  if (startTimer1 == 0 && startTimer2 != 0 && currentTime >= startTimer2)
  {
    startTimer2 = 0;
    handle_key_state(false, false);
    timeDiff = timeDiffMin;
  }
}

void handle_key_state(bool key_state, bool is_double_click)
{
  if (!key_state)
  {
    Keyboard_releaseAll();
    return;
  }

  if (is_double_click)
  {
    Keyboard_press(KEY_END);
    delayMicroseconds(10000);
    Keyboard_release(KEY_END);
  }
  else
  {
    Keyboard_press(KEY_HOME);
    delayMicroseconds(10000);
    Keyboard_release(KEY_HOME);
  }
}