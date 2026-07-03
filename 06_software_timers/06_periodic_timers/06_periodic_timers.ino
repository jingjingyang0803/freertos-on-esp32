#include <Arduino.h>
#include "freertos/timers.h"

/**
 * FreeRTOS Auto-Reload Software Timer Demo
 *
 * Learning goals:
 * 1. Create an auto-reload software timer with xTimerCreate().
 * 2. Start the timer with xTimerStart().
 * 3. Let the timer expire every 500 ms.
 * 4. Keep the timer callback short.
 * 5. Let a normal task handle the real work.
 *
 * Demo:
 * - A software timer expires every 500 ms.
 * - The timer callback only sets a flag.
 * - MonitorTask sees the flag and prints a heartbeat message.
 */

#if CONFIG_FREERTOS_UNICORE
static const BaseType_t app_cpu = 0;
#else
static const BaseType_t app_cpu = 1;
#endif

/*
 * Timer period: 500 ms
 */
static const TickType_t timer_period = pdMS_TO_TICKS(500);

/*
 * Software timer handle.
 */
static TimerHandle_t heartbeat_timer = NULL;

/*
 * This flag is set by the timer callback.
 * MonitorTask checks it later.
 */
static volatile bool heartbeat_event = false;

/*
 * Timer callback
 *
 * This function runs every 500 ms.
 *
 * Important:
 * This is NOT a hardware ISR.
 * It runs inside the FreeRTOS Timer Service Task.
 *
 * Keep it short:
 * - no delay()
 * - no long loop
 * - avoid heavy Serial printing
 */
void HeartbeatTimerCallback(TimerHandle_t xTimer)
{
  heartbeat_event = true;
}

/*
 * MonitorTask
 *
 * This task handles the periodic event created by the timer.
 */
void MonitorTask(void *parameter)
{
  unsigned long heartbeat_count = 0;

  while (1)
  {
    /*
     * Check whether the timer callback requested a heartbeat.
     */
    if (heartbeat_event)
    {
      heartbeat_event = false;

      heartbeat_count++;

      Serial.print("Heartbeat count = ");
      Serial.println(heartbeat_count);
    }

    /*
     * Small delay to avoid busy-looping.
     */
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}

void setup()
{
  Serial.begin(115200);
  delay(1000);

  Serial.println();
  Serial.println("--- FreeRTOS Auto-Reload Timer Demo ---");
  Serial.println("Timer creates one heartbeat event every 500 ms.");

  /*
   * Create an auto-reload software timer.
   *
   * Parameters:
   * 1. Timer name
   * 2. Timer period
   * 3. Auto-reload
   *      pdTRUE  = periodic timer
   *      pdFALSE = one-shot timer
   * 4. Timer ID
   * 5. Callback function
   */
  heartbeat_timer = xTimerCreate(
    "Heartbeat Timer",
    timer_period,
    pdTRUE,
    NULL,
    HeartbeatTimerCallback
  );

  if (heartbeat_timer == NULL)
  {
    Serial.println("ERROR: Timer creation failed");

    while (1)
    {
      vTaskDelay(pdMS_TO_TICKS(1000));
    }
  }

  /*
   * Create the task that handles the timer event.
   */
  BaseType_t task_result = xTaskCreatePinnedToCore(
    MonitorTask,
    "Monitor Task",
    4096,
    NULL,
    1,
    NULL,
    app_cpu
  );

  if (task_result != pdPASS)
  {
    Serial.println("ERROR: Task creation failed");

    while (1)
    {
      vTaskDelay(pdMS_TO_TICKS(1000));
    }
  }

  /*
   * Start the auto-reload timer.
   *
   * After this, FreeRTOS will call HeartbeatTimerCallback()
   * every 500 ms.
   */
  if (xTimerStart(heartbeat_timer, portMAX_DELAY) != pdPASS)
  {
    Serial.println("ERROR: Timer start failed");
  }
}

void loop()
{
  vTaskDelay(pdMS_TO_TICKS(1000));
}