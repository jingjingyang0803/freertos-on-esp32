#include <Arduino.h>

#include "app_config.h"
#include "app_types.h"
#include "button_task.h"
#include "logger_task.h"

QueueHandle_t g_button_queue = NULL;

static void button_task(void *pvParameters) {
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  bool last_state = HIGH;

  while (1) {
    bool current_state = digitalRead(BUTTON_PIN);

    // INPUT_PULLUP:
    // HIGH = not pressed
    // LOW  = pressed
    if (last_state == HIGH && current_state == LOW) {
      ButtonEvent event;
      event.type = BUTTON_EVENT_PRESSED;

      xQueueSend(g_button_queue, &event, 0);
    }

    last_state = current_state;

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