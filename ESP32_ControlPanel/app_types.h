#pragma once

enum ButtonEventType { BUTTON_EVENT_PRESSED };

struct ButtonEvent {
  ButtonEventType type;
};