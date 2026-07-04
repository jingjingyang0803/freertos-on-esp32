#include <Arduino.h>

#include "app_config.h"
#include "logger_task.h"

void setup() {
  Serial.begin(SERIAL_BAUD_RATE);
  delay(500);

  logger_task_start();

  logger_log("System booting...");
  logger_log("LoggerTask is working.");
}

void loop() {
  logger_log("Hello from loop");
  vTaskDelay(pdMS_TO_TICKS(1000));
}