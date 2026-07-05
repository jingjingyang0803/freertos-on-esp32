#include "encoder_task.h"
#include "app_config.h"
#include "app_types.h"
#include <Arduino.h>

static QueueHandle_t encoder_queue;

static volatile uint8_t last_state = 0;
static volatile int8_t encoder_accum = 0;

static void IRAM_ATTR encoder_isr() {
  uint8_t a = digitalRead(ENCODER_PIN_A);
  uint8_t b = digitalRead(ENCODER_PIN_B);

  uint8_t current_state = (a << 1) | b;

  InputEvent event;
  bool valid_event = false;

  /*
    Encoder state transitions for clockwise (CW) and counter-clockwise (CCW)
    rotation: Last State | Current State | Direction
    ----------------------------------------
    00         | 01            | CW
    01         | 11            | CW
    11         | 10            | CW
    10         | 00            | CW
    00         | 10            | CCW
    10         | 11            | CCW
    11         | 01            | CCW
    01         | 00            | CCW
  */

  if ((last_state == 0b00 && current_state == 0b01) ||
      (last_state == 0b01 && current_state == 0b11) ||
      (last_state == 0b11 && current_state == 0b10) ||
      (last_state == 0b10 && current_state == 0b00)) {
    event.type = INPUT_EVENT_ENCODER_CW;
    valid_event = true;
  } else if ((last_state == 0b00 && current_state == 0b10) ||
             (last_state == 0b10 && current_state == 0b11) ||
             (last_state == 0b11 && current_state == 0b01) ||
             (last_state == 0b01 && current_state == 0b00)) {
    event.type = INPUT_EVENT_ENCODER_CCW;
    valid_event = true;
  }

  last_state = current_state;

  if (valid_event) {
    if (event.type == INPUT_EVENT_ENCODER_CW) {
      encoder_accum++;
    } else {
      encoder_accum--;
    }

    if (encoder_accum >= 4) {
      event.type = INPUT_EVENT_ENCODER_CW;
      encoder_accum = 0;
    } else if (encoder_accum <= -4) {
      event.type = INPUT_EVENT_ENCODER_CCW;
      encoder_accum = 0;
    } else {
      return;
    }

    BaseType_t higher_priority_task_woken = pdFALSE;
    xQueueSendFromISR(encoder_queue, &event, &higher_priority_task_woken);

    if (higher_priority_task_woken == pdTRUE) {
      portYIELD_FROM_ISR();
    }
  }
}

void encoder_task_start() {
  pinMode(ENCODER_PIN_A, INPUT_PULLUP);
  pinMode(ENCODER_PIN_B, INPUT_PULLUP);

  encoder_queue = xQueueCreate(16, sizeof(InputEvent));

  uint8_t a = digitalRead(ENCODER_PIN_A);
  uint8_t b = digitalRead(ENCODER_PIN_B);
  last_state = (a << 1) | b;

  attachInterrupt(digitalPinToInterrupt(ENCODER_PIN_A), encoder_isr, CHANGE);

  attachInterrupt(digitalPinToInterrupt(ENCODER_PIN_B), encoder_isr, CHANGE);
}

QueueHandle_t encoder_get_queue() { return encoder_queue; }
