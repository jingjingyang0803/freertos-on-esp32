#include <Arduino.h>

#include "logger_task.h"

static void task_a(void *parameter) {
  while (1) {
    logger_log("Task AAAAAAAAAAAAAAAA");

    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}

static void task_b(void *parameter) {
  while (1) {
    logger_log("Task BBBBBBBBBBBBBBBB");

    vTaskDelay(pdMS_TO_TICKS(1500));
  }
}

void demo_task_start() {
  xTaskCreatePinnedToCore(task_a, "TaskA", 2048, NULL, 1, NULL, 1);

  xTaskCreatePinnedToCore(task_b, "TaskB", 2048, NULL, 1, NULL, 1);
}