#include <Arduino.h>

#include "app_config.h"
#include "display_task.h"
#include "logger_task.h"

struct DisplayCommand {
  uint8_t page;
};

static QueueHandle_t display_queue = NULL;

static void display_task(void *pvParameters) {
  DisplayCommand cmd;

  while (1) {
    if (xQueueReceive(display_queue, &cmd, portMAX_DELAY) == pdTRUE) {
      logger_log("---------------");

      if (cmd.page == 0) {
        logger_log("[Display] Page 0");
      } else if (cmd.page == 1) {
        logger_log("[Display] Page 1");
      } else {
        logger_log("[Display] Unknown page");
      }

      logger_log("---------------");
    }
  }
}

void display_task_start() {
  display_queue = xQueueCreate(DISPLAY_QUEUE_LENGTH, sizeof(DisplayCommand));

  if (display_queue == NULL) {
    logger_log("ERROR: Failed to create display queue");
    return;
  }

  xTaskCreatePinnedToCore(display_task, "DisplayTask", DISPLAY_TASK_STACK_SIZE,
                          NULL, DISPLAY_TASK_PRIORITY, NULL, CORE_UI);

  logger_log("DisplayTask started");

  display_show_page(0);
}

void display_show_page(uint8_t page) {
  if (display_queue == NULL) {
    return;
  }

  DisplayCommand cmd;
  cmd.page = page;

  xQueueSend(display_queue, &cmd, pdMS_TO_TICKS(10));
}