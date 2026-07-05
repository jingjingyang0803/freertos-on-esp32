#pragma once

enum ButtonEventType {
  BUTTON_EVENT_SHORT_PRESS,
  BUTTON_EVENT_LONG_PRESS,
  BUTTON_EVENT_DOUBLE_CLICK
};

struct ButtonEvent {
  ButtonEventType type;
};

enum DisplayPage {
  PAGE_HOME = 0,
  PAGE_HARDWARE,
  PAGE_APPS,
  PAGE_RTOS,
  PAGE_COUNT
};