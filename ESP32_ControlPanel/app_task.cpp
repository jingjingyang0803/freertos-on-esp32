#include <Arduino.h>

#include "app_config.h"
#include "app_task.h"
#include "app_types.h"
#include "button_task.h"
#include "display_task.h"
#include "logger_task.h"

static void app_task(void *pvParameters) {
  ButtonEvent event;
  uint8_t current_page = PAGE_HOME;

  while (1) {
    if (xQueueReceive(g_button_queue, &event, portMAX_DELAY) == pdTRUE) {
      if (event.type == BUTTON_EVENT_LONG_PRESS) {
        logger_log("AppTask received: BUTTON_EVENT_LONG_PRESS");

        current_page = PAGE_HOME;
        display_show_page(current_page);
      } else if (event.type == BUTTON_EVENT_SHORT_PRESS) {
        logger_log("AppTask received: BUTTON_EVENT_SHORT_PRESS");

        current_page++;
        if (current_page >= PAGE_COUNT) {
          current_page = PAGE_HOME;
        }
        display_show_page(current_page);
      } else if (event.type == BUTTON_EVENT_DOUBLE_CLICK) {
        logger_log("AppTask received: BUTTON_EVENT_DOUBLE_CLICK");
      }
    }
  }
}

void app_task_start() {
  xTaskCreatePinnedToCore(app_task, "AppTask", APP_TASK_STACK_SIZE, NULL,
                          APP_TASK_PRIORITY, NULL, CORE_UI);

  logger_log("AppTask started");
}