#include <Arduino.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/**
 * FreeRTOS Task Notification as Binary Semaphore
 *
 * Learning goals:
 * 1. Use a task notification like a binary semaphore.
 * 2. Send a signal with xTaskNotifyGive().
 * 3. Wait for a signal with ulTaskNotifyTake().
 * 4. Understand why pdTRUE is used for binary behaviour.
 *
 * Demo:
 * - WorkerTask waits for a start signal.
 * - TriggerTask sends one signal every 5 seconds.
 * - WorkerTask remains blocked until the signal arrives.
 * - After receiving the signal, WorkerTask performs one operation.
 *
 * Binary semaphore behaviour:
 * - 0 means no signal is available.
 * - A value greater than 0 means a signal is available.
 * - xTaskNotifyGive() sends the signal by increasing the value.
 * - ulTaskNotifyTake(pdTRUE, ...) receives the signal and clears
 *   the complete notification value to 0.
 *
 * In this example, the exact notification count is not important.
 * WorkerTask only cares whether a signal is available.
 */

static const int TASK_STACK_SIZE = 2048;

TaskHandle_t workerTaskHandle = NULL;

void WorkerTask(void *parameter) {
  while (1) {
    Serial.println();
    Serial.println("WorkerTask: Waiting for start signal...");

    /*
     * Wait until the notification value becomes greater than 0.
     *
     * pdTRUE:
     * - Clear the complete notification value to 0 before returning.
     * - This is suitable when the notification is used like
     *   a binary semaphore.
     *      0                   means no signal is pending.
     *      greater than 0      means a signal is pending.
     *      Therefore, multiple pending notifications are treated as one
     * available signal.
     *
     * portMAX_DELAY:
     * - Wait indefinitely for a signal.
     * - WorkerTask stays blocked without consuming CPU time.
     */
    uint32_t notificationValue = ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

    Serial.print("WorkerTask: Signal received. Value = ");
    Serial.println(notificationValue);
    Serial.println("WorkerTask: Starting work...");

    // Simulate an operation that takes 2 seconds.
    vTaskDelay(pdMS_TO_TICKS(2000));

    Serial.println("WorkerTask: Work complete.");
  }
}

void TriggerTask(void *parameter) {
  while (1) {
    Serial.println("TriggerTask: Next signal in 10 seconds...");

    // Simulate an external event occurring later.
    vTaskDelay(pdMS_TO_TICKS(10000));

    Serial.println("TriggerTask: Event occurred.");

    /*
     * Send a signal directly to WorkerTask.
     *
     * This is similar to:
     *
     *   xSemaphoreGive(binarySemaphore);
     *
     * If WorkerTask is blocked in ulTaskNotifyTake(),
     * it becomes ready to run.
     */
    xTaskNotifyGive(workerTaskHandle);

    Serial.println("TriggerTask: Signal sent. Waiting for next event...");
  }
}

void setup() {
  Serial.begin(115200);
  vTaskDelay(pdMS_TO_TICKS(1000));

  Serial.println();
  Serial.println("--- Task Notification as Binary Semaphore ---");

  /*
   * Save WorkerTask's handle because TriggerTask needs it
   * when sending the notification.
   */
  BaseType_t workerResult = xTaskCreate(
      WorkerTask, "Worker Task", TASK_STACK_SIZE, NULL, 1, &workerTaskHandle);

  if (workerResult != pdPASS) {
    Serial.println("Failed to create WorkerTask.");
    return;
  }

  BaseType_t triggerResult =
      xTaskCreate(TriggerTask, "Trigger Task", TASK_STACK_SIZE, NULL, 1, NULL);

  if (triggerResult != pdPASS) {
    Serial.println("Failed to create TriggerTask.");
    return;
  }

  vTaskDelete(NULL);
}

void loop() {
  // Not used.
}