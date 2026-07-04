#include <Arduino.h>

#include "app_config.h"
#include "button_task.h"
#include "logger_task.h"

/*
 * This function is private to button_task.cpp.
 */
static void button_task(void *pvParameters) {
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  bool last_state = HIGH;

  while (1) {
    bool current_state = digitalRead(BUTTON_PIN);

    /*
     * With INPUT_PULLUP:
     *   HIGH = not pressed
     *   LOW  = pressed
     *
     * We only log on the falling edge:
     *   HIGH -> LOW
     *
     * This avoids printing continuously while the button is held.
     */
    if (last_state == HIGH && current_state == LOW) {
      logger_log("Button pressed");
    }

    last_state = current_state;

    /*
     * Small delay to avoid wasting CPU time.
     * This is not real debounce yet.
     */
    vTaskDelay(pdMS_TO_TICKS(BUTTON_SCAN_MS));
  }
}

void button_task_start() {
  xTaskCreatePinnedToCore(button_task, "ButtonTask", BUTTON_TASK_STACK_SIZE,
                          NULL, BUTTON_TASK_PRIORITY, NULL, CORE_UI);

  logger_log("ButtonTask started");
}