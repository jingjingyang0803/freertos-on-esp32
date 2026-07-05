#include <Arduino.h>

#include "app_config.h"
#include "app_task.h"
#include "button_task.h"
#include "display_task.h"
#include "encoder_task.h"
#include "logger_task.h"

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("BOOT OK");

  Serial.println("start logger");
  logger_task_start();

  Serial.println("start display");
  display_task_start();

  Serial.println("start button");
  button_task_start();

  Serial.println("start encoder");
  encoder_task_start();

  Serial.println("start app");
  app_task_start();

  Serial.println("setup done");
}

void loop() { vTaskDelay(pdMS_TO_TICKS(1000)); }