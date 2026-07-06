#include <Arduino.h>

#include "app_config.h"
#include "app_task.h"
#include "app_types.h"
#include "button_task.h"
#include "display_task.h"
#include "encoder_task.h"
#include "logger_task.h"
#include "ui_pages.h"

static void app_task(void *pvParameters) {
  InputEvent event;
  DisplayPage current_page = PAGE_HOME;
  uint8_t selected_index = 0;

  display_show_page(current_page);

  while (1) {
    if (xQueueReceive(encoder_get_queue(), &event, pdMS_TO_TICKS(10)) ==
        pdTRUE) {
      uint8_t item_count = ui_page_get_item_count(current_page);

      if (event.type == INPUT_EVENT_ENCODER_CW) {
        selected_index++;

        if (selected_index >= item_count) {
          selected_index = 0;
        }

        display_show_page(current_page, selected_index);
      } else if (event.type == INPUT_EVENT_ENCODER_CCW) {

        if (selected_index == 0) {
          selected_index = item_count - 1;
        } else {
          selected_index--;
        }

        display_show_page(current_page, selected_index);
      }
    }

    if (xQueueReceive(button_get_queue(), &event, pdMS_TO_TICKS(10)) ==
        pdTRUE) {
      if (event.type == INPUT_EVENT_BUTTON_LONG) {
        current_page = PAGE_HOME;
        selected_index = 0;
        display_show_page(current_page, selected_index);
      } else if (event.type == INPUT_EVENT_BUTTON_SHORT) {
        current_page = display_show_next_page(current_page);
        selected_index = 0;
        display_show_page(current_page, selected_index);
      } else if (event.type == INPUT_EVENT_BUTTON_DOUBLE) {
        display_show_page(current_page, selected_index);
      }
    }
  }
}

void app_task_start() {
  xTaskCreatePinnedToCore(app_task, "AppTask", APP_TASK_STACK_SIZE, NULL,
                          APP_TASK_PRIORITY, NULL, CORE_UI);

  logger_log("AppTask started");
}