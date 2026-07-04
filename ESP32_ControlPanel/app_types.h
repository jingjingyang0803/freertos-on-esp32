#pragma once

enum ButtonEventType { BUTTON_EVENT_PRESSED, BUTTON_EVENT_RELEASED };

struct ButtonEvent {
  ButtonEventType type;
};