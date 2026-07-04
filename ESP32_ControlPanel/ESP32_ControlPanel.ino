#include <Arduino.h>

#include "app_config.h"
#include "button_task.h"
#include "logger_task.h"

void setup() {
  Serial.begin(SERIAL_BAUD_RATE);
  delay(500);

  logger_task_start();
  button_task_start();

  logger_log("System booting...");
}

void loop() {
  vTaskDelay(pdMS_TO_TICKS(1000));
}