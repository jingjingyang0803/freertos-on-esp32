#pragma once

enum ButtonEventType {
  BUTTON_EVENT_SHORT_PRESS,
  BUTTON_EVENT_LONG_PRESS,
  BUTTON_EVENT_DOUBLE_CLICK
};

struct ButtonEvent {
  ButtonEventType type;
};