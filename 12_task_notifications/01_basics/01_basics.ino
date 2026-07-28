#include <Arduino.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/**
 * FreeRTOS Task Notification Basics Demo
 *
 * Learning goals:
 * 1. Send a notification with xTaskNotifyGive().
 * 2. Wait for notifications with ulTaskNotifyTake().
 * 3. Understand the task notification counter.
 * 4. Observe multiple notifications accumulating.
 *
 * Demo:
 * - ReceiverTask waits for notifications.
 * - SenderTask sends one notification every second.
 * - ReceiverTask simulates 5 seconds of processing.
 * - Notifications sent during processing accumulate.
 *
 * Notification counter:
 * - Every task has its own notification value.
 * - xTaskNotifyGive() increases the target task's value by 1.
 * - ulTaskNotifyTake(pdFALSE, ...) returns the current value
 *   and then decreases it by 1.
 * - ulTaskNotifyTake(pdTRUE, ...) returns the current value
 *   and then clears it to 0.
 *
 * Example:
 *
 *   Initial notification value: 0
 *   First Give:                1
 *   Second Give:               2
 *
 *   Take with pdFALSE:
 *   Return value:              2
 *   Value after Take:          1
 *
 *   Take with pdTRUE:
 *   Return value:              2
 *   Value after Take:          0
 */

static const int TASK_STACK_SIZE = 2048;

TaskHandle_t receiverTaskHandle = NULL;

void ReceiverTask(void *parameter) {
  while (1) {
    Serial.println();
    Serial.println("ReceiverTask: Waiting for notifications...");

    /*
     * Wait until this task's notification value is greater than 0.
     *
     * portMAX_DELAY:
     * - Wait indefinitely.
     * - The task stays blocked without consuming CPU time.
     */
    uint32_t notificationCount =
        ulTaskNotifyTake(pdFALSE, // Decrease the notification value by 1.
                         portMAX_DELAY);

    Serial.print("ReceiverTask: Received notification count = ");
    Serial.println(notificationCount);

    /*
     * Simulate slow processing.
     *
     * SenderTask continues sending notifications during these
     * 5 seconds, so the notification value can accumulate.
     */
    Serial.println("ReceiverTask: Processing for 5 seconds...");
    vTaskDelay(pdMS_TO_TICKS(5000));

    Serial.println("ReceiverTask: Processing complete.");
  }
}

void SenderTask(void *parameter) {
  while (1) {
    // Simulate an event occurring once every second.
    vTaskDelay(pdMS_TO_TICKS(1000));

    /*
     * Send one notification directly to ReceiverTask.
     *
     * This increases ReceiverTask's notification value by 1.
     *
     * If ReceiverTask is blocked in ulTaskNotifyTake(),
     * it becomes ready to run.
     */
    xTaskNotifyGive(receiverTaskHandle);

    Serial.println("SenderTask: Sent one notification.");
  }
}

void setup() {
  Serial.begin(115200);
  vTaskDelay(pdMS_TO_TICKS(1000));

  Serial.println();
  Serial.println("--- FreeRTOS Task Notification Basics Demo ---");

  /*
   * Create ReceiverTask first.
   *
   * &receiverTaskHandle stores the handle of ReceiverTask.
   * SenderTask uses this handle to send notifications to it.
   */
  BaseType_t receiverResult =
      xTaskCreate(ReceiverTask, "Receiver Task", TASK_STACK_SIZE, NULL, 1,
                  &receiverTaskHandle);

  if (receiverResult != pdPASS) {
    Serial.println("Failed to create ReceiverTask.");
    return;
  }

  BaseType_t senderResult =
      xTaskCreate(SenderTask, "Sender Task", TASK_STACK_SIZE, NULL, 1, NULL);

  if (senderResult != pdPASS) {
    Serial.println("Failed to create SenderTask.");
    return;
  }

  vTaskDelete(NULL);
}

void loop() {
  // Not used.
}