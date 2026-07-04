#include <Arduino.h>

#include "app_config.h"
#include "app_types.h"
#include "button_task.h"
#include "logger_task.h"

QueueHandle_t g_button_queue = NULL;

static void button_task(void *pvParameters) {
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  // INPUT_PULLUP:
  // HIGH = not pressed
  // LOW  = pressed
  bool last_stable_state = HIGH;
  bool last_raw_state = HIGH;
  uint32_t last_debounce_time = 0;
  uint32_t press_start_time = 0;

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
          press_start_time = now;
        } else {
          uint32_t press_duration = now - press_start_time;

          if (press_duration >= BUTTON_LONG_PRESS_MS) {
            event.type = BUTTON_EVENT_LONG_PRESS;
          } else {
            event.type = BUTTON_EVENT_SHORT_PRESS;
          }

          send_event = true;
        }

        if (send_event) {
          xQueueSend(g_button_queue, &event, 0);
        }
      }
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