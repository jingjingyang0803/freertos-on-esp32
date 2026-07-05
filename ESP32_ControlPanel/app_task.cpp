#include <Arduino.h>

#include "app_config.h"
#include "app_task.h"
#include "app_types.h"
#include "button_task.h"
#include "display_task.h"
#include "encoder_task.h"
#include "logger_task.h"

static void app_task(void *pvParameters) {
  InputEvent event;
  uint8_t current_page = PAGE_HOME;

  display_show_page(current_page);

  while (1) {
    if (xQueueReceive(encoder_get_queue(), &event, pdMS_TO_TICKS(10)) ==
        pdTRUE) {
      if (event.type == INPUT_EVENT_ENCODER_CW) {
        logger_log("Encoder: CW");
      } else if (event.type == INPUT_EVENT_ENCODER_CCW) {
        logger_log("Encoder: CCW");
      }
    }

    if (xQueueReceive(button_get_queue(), &event, pdMS_TO_TICKS(10)) ==
        pdTRUE) {
      if (event.type == INPUT_EVENT_BUTTON_LONG) {
        current_page = PAGE_HOME;
        display_show_page(current_page);
      } else if (event.type == INPUT_EVENT_BUTTON_SHORT) {
        current_page++;
        if (current_page >= PAGE_COUNT) {
          current_page = PAGE_HOME;
        }
        display_show_page(current_page);
      } else if (event.type == INPUT_EVENT_BUTTON_DOUBLE) {
        display_show_page(current_page);
      }
    }
  }
}

void app_task_start() {
  xTaskCreatePinnedToCore(app_task, "AppTask", APP_TASK_STACK_SIZE, NULL,
                          APP_TASK_PRIORITY, NULL, CORE_UI);

  logger_log("AppTask started");
}