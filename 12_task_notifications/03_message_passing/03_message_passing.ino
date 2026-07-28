#include <Arduino.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/**
 * FreeRTOS Task Notification Message Passing Demo
 *
 * Learning goals:
 * 1. Send a 32-bit value with xTaskNotify().
 * 2. Receive the value with xTaskNotifyWait().
 * 3. Understand eSetValueWithOverwrite.
 *
 * Demo:
 * - SensorTask generates a simulated sensor value every 3 seconds.
 * - DisplayTask blocks while waiting for a new value.
 * - SensorTask sends the value directly to DisplayTask.
 * - DisplayTask receives and prints the value.
 *
 * Message passing:
 * - The notification value is used to store a uint32_t message.
 * - xTaskNotify() writes the message to the target task.
 * - xTaskNotifyWait() waits for and reads the message.
 *
 * Important:
 * - A task notification can directly carry one 32-bit value.
 * - It is a lightweight mailbox, not a queue.
 * - Only one notification value is stored at a time.
 */

static const int TASK_STACK_SIZE = 2048;

TaskHandle_t displayTaskHandle = NULL;

void DisplayTask(void *parameter) {
  while (1) {
    Serial.println();
    Serial.println("DisplayTask: Waiting for sensor value...");

    uint32_t receivedValue = 0;

    /*
     * Wait for a notification and copy its value
     * into receivedValue.
     *
     * First parameter: 0
     * - Do not clear any bits before waiting.
     *
     * Second parameter: UINT32_MAX
     * - Clear all notification-value bits after receiving.
     *
     * Third parameter: &receivedValue
     * - Store the received 32-bit value here.
     *
     * Fourth parameter: portMAX_DELAY
     * - Wait indefinitely for a message.
     *
     * The function returns pdTRUE after receiving a notification.
     */
    BaseType_t received =
        xTaskNotifyWait(0, UINT32_MAX, &receivedValue, portMAX_DELAY);

    if (received == pdTRUE) {
      Serial.print("DisplayTask: Received sensor value = ");
      Serial.println(receivedValue);
    }
  }
}

void SensorTask(void *parameter) {
  uint32_t sensorValue = 20;

  while (1) {
    Serial.println("SensorTask: Measuring...");

    vTaskDelay(pdMS_TO_TICKS(3000));

    /*
     * Generate a changing simulated value:
     *
     * 20, 21, 22, 23, 24, 25, 20...
     */
    sensorValue++;

    if (sensorValue > 25) {
      sensorValue = 20;
    }

    Serial.print("SensorTask: Sending sensor value = ");
    Serial.println(sensorValue);

    /*
     * Send sensorValue directly to DisplayTask.
     *
     * eSetValueWithOverwrite:
     * - Set DisplayTask's notification value to sensorValue.
     * - If an unread value already exists, replace it with
     *   the new value.
     */
    BaseType_t result =
        xTaskNotify(displayTaskHandle, sensorValue, eSetValueWithOverwrite);

    if (result == pdPASS) {
      Serial.println("SensorTask: Value sent.");
    }
  }
}

void setup() {
  Serial.begin(115200);
  vTaskDelay(pdMS_TO_TICKS(1000));

  Serial.println();
  Serial.println("--- Task Notification Message Passing Demo ---");

  /*
   * Create DisplayTask first and save its handle.
   */
  BaseType_t displayResult =
      xTaskCreate(DisplayTask, "Display Task", TASK_STACK_SIZE, NULL, 1,
                  &displayTaskHandle);

  if (displayResult != pdPASS) {
    Serial.println("Failed to create DisplayTask.");
    return;
  }

  BaseType_t sensorResult =
      xTaskCreate(SensorTask, "Sensor Task", TASK_STACK_SIZE, NULL, 1, NULL);

  if (sensorResult != pdPASS) {
    Serial.println("Failed to create SensorTask.");
    return;
  }

  vTaskDelete(NULL);
}

void loop() {
  // Not used.
}