#include <Arduino.h>

#include "app_config.h"
#include "app_types.h"
#include "button_task.h"
#include "logger_task.h"

QueueHandle_t g_button_queue = NULL;

static void button_task(void *pvParameters) {
  // INPUT_PULLUP:
  // HIGH = not pressed
  // LOW  = pressed
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  // Debounce state.
  bool last_stable_state = HIGH;   // Last accepted button state.
  bool last_raw_state = HIGH;      // Last sampled GPIO state.
  uint32_t last_debounce_time = 0; // Time of the last raw state change.

  // Press timing state.
  uint32_t press_start_time = 0;

  // Double-click state.
  // A short press is delayed briefly to see if a second click follows.
  bool waiting_for_second_click = false;
  uint32_t first_click_time = 0;

  while (1) {
    bool raw_state = digitalRead(BUTTON_PIN);
    uint32_t now = millis();

    // Detect any change and restart the debounce timer.
    if (raw_state != last_raw_state) {
      last_debounce_time = now;
      last_raw_state = raw_state;
    }

    // Accept the new state only after it has been stable long enough.
    if ((now - last_debounce_time) > BUTTON_DEBOUNCE_MS) {

      // Has the stable state actually changed?
      if (raw_state != last_stable_state) {
        last_stable_state = raw_state;

        ButtonEvent event;
        bool send_event = false;

        if (last_stable_state == LOW) {
          // Button pressed: remember when the press started.
          press_start_time = now;
        } else {
          // Button released: classify the completed press.
          uint32_t press_duration = now - press_start_time;

          if (press_duration >= BUTTON_LONG_PRESS_MS) {
            // Long press is sent immediately.
            event.type = BUTTON_EVENT_LONG_PRESS;
            send_event = true;
            waiting_for_second_click = false;
          } else {
            // Short press: wait briefly to see if it becomes a double click.
            if (waiting_for_second_click &&
                (now - first_click_time <= BUTTON_DOUBLE_CLICK_MS)) {
              event.type = BUTTON_EVENT_DOUBLE_CLICK;
              send_event = true;
              waiting_for_second_click = false;
            } else {
              // First short click detected. Do not send SHORT_PRESS yet.
              waiting_for_second_click = true;
              first_click_time = now;
            }
          }
        }

        if (send_event) {
          xQueueSend(g_button_queue, &event, 0);
        }
      }
    }

    // If no second click arrives in time, confirm it as a single short press.
    if (waiting_for_second_click &&
        (now - first_click_time > BUTTON_DOUBLE_CLICK_MS)) {
      ButtonEvent event;
      event.type = BUTTON_EVENT_SHORT_PRESS;

      xQueueSend(g_button_queue, &event, 0);
      waiting_for_second_click = false;
    }

    vTaskDelay(pdMS_TO_TICKS(BUTTON_SCAN_MS));
  }
}

void button_task_start() {
  g_button_queue = xQueueCreate(BUTTON_QUEUE_LENGTH, sizeof(ButtonEvent));

  if (g_button_queue == NULL) {
    logger_log("ERROR: Failed to create button queue");
    return;
  }

  xTaskCreatePinnedToCore(button_task, "ButtonTask", BUTTON_TASK_STACK_SIZE,
                          NULL, BUTTON_TASK_PRIORITY, NULL, CORE_UI);

  logger_log("ButtonTask started");
}