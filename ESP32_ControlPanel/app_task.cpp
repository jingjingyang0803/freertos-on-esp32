#include <Arduino.h>

#include "app_config.h"
#include "app_task.h"
#include "app_types.h"
#include "button_task.h"
#include "logger_task.h"

static void app_task(void *pvParameters) {
  ButtonEvent event;

  while (1) {
    if (xQueueReceive(g_button_queue, &event, portMAX_DELAY) == pdTRUE) {
      if (event.type == BUTTON_EVENT_PRESSED) {
        logger_log("AppTask received: BUTTON_EVENT_PRESSED");
      } else if (event.type == BUTTON_EVENT_RELEASED) {
        logger_log("AppTask received: BUTTON_EVENT_RELEASED");
      }
    }
  }
}

void app_task_start() {
  xTaskCreatePinnedToCore(app_task, "AppTask", APP_TASK_STACK_SIZE, NULL,
                          APP_TASK_PRIORITY, NULL, CORE_UI);

  logger_log("AppTask started");
}