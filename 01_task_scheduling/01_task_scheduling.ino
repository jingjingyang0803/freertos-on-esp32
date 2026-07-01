/**
 * FreeRTOS Task Creation Demo
 *
 * Learning goals:
 * 1. Create FreeRTOS tasks with xTaskCreatePinnedToCore().
 * 2. Understand the required task function format:
 *      void taskName(void *parameter)
 * 3. Run multiple tasks independently.
 * 4. Use vTaskDelay() to give CPU time to other tasks.
 * 5. Understand stack size, priority, task name, and task parameter.
 *
 * Demo:
 * - LedTask blinks the LED every 500 ms.
 * - PrintTask prints a message every 1000 ms.
 */

#include <Arduino.h>

#if CONFIG_FREERTOS_UNICORE
static const BaseType_t app_cpu = 0;
#else
static const BaseType_t app_cpu = 1;
#endif

static const int led_pin = LED_BUILTIN;

/*
 * Task 1: Blink LED
 *
 * Every FreeRTOS task must:
 * - return void
 * - take one void pointer parameter
 * - usually run forever inside while(1)
 */
void LedTask(void *parameter)
{
  pinMode(led_pin, OUTPUT);

  while (1)
  {
    digitalWrite(led_pin, HIGH);
    vTaskDelay(pdMS_TO_TICKS(500));

    digitalWrite(led_pin, LOW);
    vTaskDelay(pdMS_TO_TICKS(500));
  }
}

/*
 * Task 2: Print message
 *
 * This task runs independently from LedTask.
 * vTaskDelay() blocks only this task, not the whole CPU.
 */
void PrintTask(void *parameter)
{
  while (1)
  {
    Serial.println("PrintTask is running");

    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}

void setup()
{
  Serial.begin(115200);
  vTaskDelay(pdMS_TO_TICKS(1000));

  Serial.println("--- FreeRTOS Task Creation Demo ---");

  /*
   * Create LedTask
   *
   * Parameters:
   * 1. Task function
   * 2. Task name
   * 3. Stack size in words
   * 4. Task parameter
   * 5. Task priority
   * 6. Task handle
   * 7. CPU core
   */
  xTaskCreatePinnedToCore(
    LedTask,
    "LED Task",
    4096,
    NULL,
    1,
    NULL,
    app_cpu
  );

  /*
   * Create PrintTask
   */
  xTaskCreatePinnedToCore(
    PrintTask,
    "Print Task",
    4096,
    NULL,
    1,
    NULL,
    app_cpu
  );
}

void loop()
{
  /*
   * In ESP32 Arduino, loop() is also running as a task.
   * We leave it empty and delay it, so it does not waste CPU time.
   */
  vTaskDelay(pdMS_TO_TICKS(1000));
}