#include <Arduino.h>

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

/**
 * FreeRTOS Queue Sets Demo
 *
 * Learning goals:
 * 1. Create multiple queues.
 * 2. Add the queues to one queue set.
 * 3. Wait for multiple queues with xQueueSelectFromSet().
 * 4. Identify which queue became ready.
 * 5. Observe task wake-up and preemption clearly.
 *
 * Demo:
 * - TemperatureTask sends a temperature every 2 seconds.
 * - CommandTask sends START or STOP every 7 seconds.
 * - ControllerTask waits for either queue.
 * - The first queue that receives data wakes ControllerTask.
 *
 * Important:
 * - A queue set does not store the application data.
 * - It only tells ControllerTask which member queue is ready.
 * - ControllerTask must still call xQueueReceive().
 * - ControllerTask has a higher priority, so it may run immediately
 *   after xQueueSend() and print before the sending task continues.
 */

static const int TASK_STACK_SIZE = 3072;

static const int TEMPERATURE_QUEUE_LENGTH = 5;
static const int COMMAND_QUEUE_LENGTH = 3;

/*
 * The queue set must have enough capacity for all possible
 * pending items in all member queues.
 *
 * Temperature queue: 5 items
 * Command queue:     3 items
 *
 * Total queue-set capacity: 8
 */
static const int QUEUE_SET_LENGTH =
    TEMPERATURE_QUEUE_LENGTH + COMMAND_QUEUE_LENGTH;

enum ControlCommand { COMMAND_START = 1, COMMAND_STOP = 2 };

QueueHandle_t temperatureQueue = NULL;
QueueHandle_t commandQueue = NULL;

QueueSetHandle_t controllerQueueSet = NULL;

void TemperatureTask(void *parameter) {
  int temperature = 20;

  while (1) {
    vTaskDelay(pdMS_TO_TICKS(2000));

    temperature += 2;

    if (temperature > 30) {
      temperature = 20;
    }

    /*
     * Print before sending.
     *
     * This makes the execution order easier to understand.
     */
    Serial.print("[Temperature] Sending value = ");
    Serial.println(temperature);

    BaseType_t result =
        xQueueSend(temperatureQueue, &temperature, pdMS_TO_TICKS(100));

    if (result == pdPASS) {
      Serial.println("[Temperature] Value queued.");
    } else {
      Serial.println("[Temperature] Queue is full.");
    }
  }
}

void CommandTask(void *parameter) {
  ControlCommand command = COMMAND_START;

  while (1) {
    vTaskDelay(pdMS_TO_TICKS(7000));

    if (command == COMMAND_START) {
      Serial.println("[Command] Sending START...");
    } else {
      Serial.println("[Command] Sending STOP...");
    }

    BaseType_t result = xQueueSend(commandQueue, &command, pdMS_TO_TICKS(100));

    if (result == pdPASS) {
      /*
       * ControllerTask may run before this line because it has
       * a higher priority and becomes ready after xQueueSend().
       */
      if (command == COMMAND_START) {
        Serial.println("[Command] START queued.");
        command = COMMAND_STOP;
      } else {
        Serial.println("[Command] STOP queued.");
        command = COMMAND_START;
      }
    } else {
      Serial.println("[Command] Queue is full.");
    }
  }
}

void ControllerTask(void *parameter) {
  bool systemRunning = false;

  Serial.println("[Controller] Waiting for temperature or command...");

  while (1) {
    /*
     * Block until one member queue becomes ready.
     *
     * The returned handle identifies the ready queue.
     */
    QueueSetMemberHandle_t readyMember =
        xQueueSelectFromSet(controllerQueueSet, portMAX_DELAY);

    /*
     * Temperature queue became ready.
     */
    if (readyMember == temperatureQueue) {
      int temperature = 0;

      BaseType_t result = xQueueReceive(temperatureQueue, &temperature, 0);

      if (result == pdPASS) {
        Serial.print("[Controller] Temperature received = ");
        Serial.println(temperature);

        if (temperature >= 28) {
          Serial.println("[Controller] Temperature warning.");
        }

        Serial.print("[Controller] System state = ");
        Serial.println(systemRunning ? "RUNNING" : "STOPPED");
      }
    }

    /*
     * Command queue became ready.
     */
    else if (readyMember == commandQueue) {
      ControlCommand command;

      BaseType_t result = xQueueReceive(commandQueue, &command, 0);

      if (result == pdPASS) {
        if (command == COMMAND_START) {
          systemRunning = true;
          Serial.println("[Controller] START received.");
        } else if (command == COMMAND_STOP) {
          systemRunning = false;
          Serial.println("[Controller] STOP received.");
        }

        Serial.print("[Controller] System state = ");
        Serial.println(systemRunning ? "RUNNING" : "STOPPED");
      }
    }

    Serial.println("[Controller] Waiting for temperature or command...");
  }
}

void setup() {
  Serial.begin(115200);
  vTaskDelay(pdMS_TO_TICKS(1000));

  Serial.println();
  Serial.println("--- FreeRTOS Queue Sets Demo ---");

  /*
   * Create the two member queues.
   */
  temperatureQueue = xQueueCreate(TEMPERATURE_QUEUE_LENGTH, sizeof(int));

  commandQueue = xQueueCreate(COMMAND_QUEUE_LENGTH, sizeof(ControlCommand));

  if (temperatureQueue == NULL || commandQueue == NULL) {
    Serial.println("Failed to create queues.");
    return;
  }

  /*
   * Create one queue set for both queues.
   */
  controllerQueueSet = xQueueCreateSet(QUEUE_SET_LENGTH);

  if (controllerQueueSet == NULL) {
    Serial.println("Failed to create queue set.");
    return;
  }

  /*
   * Add both queues to the queue set.
   *
   * Member queues should be empty when added.
   */
  BaseType_t temperatureAddResult =
      xQueueAddToSet(temperatureQueue, controllerQueueSet);

  BaseType_t commandAddResult =
      xQueueAddToSet(commandQueue, controllerQueueSet);

  if (temperatureAddResult != pdPASS || commandAddResult != pdPASS) {
    Serial.println("Failed to add queues to queue set.");
    return;
  }

  /*
   * ControllerTask has higher priority.
   *
   * When a producer sends data, ControllerTask can wake up
   * immediately and preempt the producer task.
   */
  BaseType_t controllerResult = xTaskCreate(ControllerTask, "Controller Task",
                                            TASK_STACK_SIZE, NULL, 2, NULL);

  BaseType_t temperatureResult = xTaskCreate(
      TemperatureTask, "Temperature Task", TASK_STACK_SIZE, NULL, 1, NULL);

  BaseType_t commandResult =
      xTaskCreate(CommandTask, "Command Task", TASK_STACK_SIZE, NULL, 1, NULL);

  if (controllerResult != pdPASS || temperatureResult != pdPASS ||
      commandResult != pdPASS) {
    Serial.println("Failed to create one or more tasks.");
    return;
  }

  vTaskDelete(NULL);
}

void loop() {
  // Not used.
}