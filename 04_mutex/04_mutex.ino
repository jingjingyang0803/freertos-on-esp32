#include <Arduino.h>

/**
 * FreeRTOS Mutex Demo
 *
 * Learning goals:
 * 1. Create a mutex with xSemaphoreCreateMutex().
 * 2. Protect a shared resource with xSemaphoreTake() and xSemaphoreGive().
 * 3. Understand that Serial is a shared resource.
 * 4. Avoid multiple tasks writing to Serial at the same time.
 *
 * Demo:
 * - TaskA and TaskB both print messages to Serial.
 * - A mutex is used so only one task can print at a time.
 *
 * Important note:
 * Mutex is used for resource protection.
 * Semaphore is usually used for task synchronization.
 */

#if CONFIG_FREERTOS_UNICORE
static const BaseType_t app_cpu = 0;
#else
static const BaseType_t app_cpu = 1;
#endif

static SemaphoreHandle_t serial_mutex;

void PrintTaskA(void *parameter)
{
  while (1)
  {
    xSemaphoreTake(serial_mutex, portMAX_DELAY);

    Serial.print("[Task A] ");
    Serial.print("Hello ");
    Serial.print("from ");
    Serial.println("Task A");

    xSemaphoreGive(serial_mutex);

    vTaskDelay(pdMS_TO_TICKS(500));
  }
}

void PrintTaskB(void *parameter)
{
  while (1)
  {
    xSemaphoreTake(serial_mutex, portMAX_DELAY);

    Serial.print("[Task B] ");
    Serial.print("Hello ");
    Serial.print("from ");
    Serial.println("Task B");

    xSemaphoreGive(serial_mutex);

    vTaskDelay(pdMS_TO_TICKS(700));
  }
}

void setup()
{
  Serial.begin(115200);
  vTaskDelay(pdMS_TO_TICKS(1000));

  Serial.println();
  Serial.println("--- FreeRTOS Mutex Demo ---");

  serial_mutex = xSemaphoreCreateMutex();

  if (serial_mutex == NULL)
  {
    Serial.println("ERROR: Mutex creation failed.");
    while (1)
    {
      vTaskDelay(pdMS_TO_TICKS(1000));
    }
  }

  xTaskCreatePinnedToCore(
    PrintTaskA,
    "Print Task A",
    2048,
    NULL,
    1,
    NULL,
    app_cpu
  );

  xTaskCreatePinnedToCore(
    PrintTaskB,
    "Print Task B",
    2048,
    NULL,
    1,
    NULL,
    app_cpu
  );
}

void loop()
{
  vTaskDelay(pdMS_TO_TICKS(1000));
}