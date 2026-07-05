#pragma once

enum InputEventType {
  INPUT_EVENT_BUTTON_SHORT,
  INPUT_EVENT_BUTTON_LONG,
  INPUT_EVENT_BUTTON_DOUBLE,
  INPUT_EVENT_ENCODER_CW,
  INPUT_EVENT_ENCODER_CCW,
};

struct InputEvent {
  InputEventType type;
};

enum DisplayPage {
  PAGE_HOME = 0,
  PAGE_HARDWARE,
  PAGE_APPS,
  PAGE_RTOS,
  PAGE_COUNT
};