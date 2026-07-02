/**
 * FreeRTOS Binary Semaphore Basic Demo
 *
 * Goal:
 * Learn the core idea of a binary semaphore.
 *
 * Demo:
 * - ProducerTask gives a binary semaphore every 2 seconds.
 * - ConsumerTask waits for the semaphore.
 * - When ConsumerTask receives the semaphore, it prints a message.
 *
 * Key idea:
 * A binary semaphore is used to notify a task that an event happened.
 */

#include <Arduino.h>

#if CONFIG_FREERTOS_UNICORE
static const BaseType_t app_cpu = 0;
#else
static const BaseType_t app_cpu = 1;
#endif

SemaphoreHandle_t event_sem;

/*
 * ProducerTask
 *
 * This task creates an event every 2 seconds.
 * It gives the semaphore to notify ConsumerTask.
 */
void ProducerTask(void *parameter)
{
  while (1)
  {
    Serial.println("ProducerTask: event happened");

    /*
     * Give the semaphore.
     *
     * This changes the semaphore state from:
     *   empty -> available
     *
     * If ConsumerTask is waiting, it will be unblocked.
     */
    xSemaphoreGive(event_sem);

    vTaskDelay(pdMS_TO_TICKS(2000));
  }
}

/*
 * ConsumerTask
 *
 * This task waits for the event.
 */
void ConsumerTask(void *parameter)
{
  while (1)
  {
    /*
     * Wait forever until the semaphore is given.
     *
     * portMAX_DELAY means:
     *   block here until the event happens
     */
    if (xSemaphoreTake(event_sem, portMAX_DELAY) == pdTRUE)
    {
      Serial.println("ConsumerTask: received event");
    }
  }
}

void setup()
{
  Serial.begin(115200);
  delay(1000);

  Serial.println();
  Serial.println("--- Binary Semaphore Basic Demo ---");

  /*
   * Create a binary semaphore.
   *
   * Initial state:
   *   empty / not available
   *
   * So ConsumerTask will block until ProducerTask gives it.
   */
  event_sem = xSemaphoreCreateBinary();

  if (event_sem == NULL)
  {
    Serial.println("ERROR: Failed to create semaphore");

    while (1)
    {
      vTaskDelay(pdMS_TO_TICKS(1000));
    }
  }

  xTaskCreatePinnedToCore(
    ProducerTask,
    "Producer Task",
    4096,
    NULL,
    1,
    NULL,
    app_cpu
  );

  xTaskCreatePinnedToCore(
    ConsumerTask,
    "Consumer Task",
    4096,
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