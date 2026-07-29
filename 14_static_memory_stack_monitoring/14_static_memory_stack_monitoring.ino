#include <Arduino.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/**
 * FreeRTOS Static Task Memory and Stack Monitoring Demo
 *
 * Learning goals:
 * 1. Create tasks with xTaskCreateStatic().
 * 2. Provide task stacks manually.
 * 3. Provide StaticTask_t control blocks manually.
 * 4. Monitor stack usage with uxTaskGetStackHighWaterMark().
 *
 * Demo:
 * - WorkerTask performs simulated data processing.
 * - MonitorTask periodically checks both task stacks.
 *
 * Static task memory:
 * - A task needs memory for:
 *   1. Its stack.
 *      Used while the task is running.
 *      It stores local variables, function parameters,
 *      return addresses, and temporary function data.
 *   2. Its task control block (TCB).
 *      Used by FreeRTOS to manage the task.
 *      It stores information such as the task priority,
 *      current state, stack pointer, task name,
 *      and scheduling data.
 *
 * - xTaskCreateStatic() does not allocate these dynamically.
 * - The application provides both memory areas before creating
 *   the task.
 *
 * Stack high-water mark:
 * - It reports the smallest amount of unused stack space
 *   observed since the task was created.
 * - A smaller value means the task has used more of its stack.
 * - A value close to 0 means the stack may be too small.
 *
 * ESP32 note:
 * - In ESP-IDF's FreeRTOS implementation, task stack sizes and
 *   stack high-water marks are reported in bytes.
 */

static const uint32_t WORKER_STACK_SIZE = 3072;
static const uint32_t MONITOR_STACK_SIZE = 2048;

static const UBaseType_t WORKER_PRIORITY = 1;
static const UBaseType_t MONITOR_PRIORITY = 2;

/*
 * Statically allocated task stacks.
 *
 * These arrays contain the stack memory used by each task.
 * They must remain valid for the entire lifetime of the tasks.
 */
static StackType_t workerStack[WORKER_STACK_SIZE];
static StackType_t monitorStack[MONITOR_STACK_SIZE];

/*
 * Statically allocated task control blocks.
 *
 * FreeRTOS stores task information such as state, priority,
 * stack location, and scheduling data in these structures.
 */
static StaticTask_t workerTaskControlBlock;
static StaticTask_t monitorTaskControlBlock;

static TaskHandle_t workerTaskHandle = NULL;
static TaskHandle_t monitorTaskHandle = NULL;

void WorkerTask(void *parameter) {
  uint32_t cycle = 0;

  while (1) {
    cycle++;

    Serial.println();
    Serial.print("[Worker] Processing cycle ");
    Serial.println(cycle);

    /*
     * Local variables are stored on this task's stack.
     *
     * This temporary array deliberately uses some stack space
     * so the stack monitor has something meaningful to report.
     */
    uint8_t temporaryBuffer[512];

    for (size_t i = 0; i < sizeof(temporaryBuffer); i++) {
      temporaryBuffer[i] = static_cast<uint8_t>((cycle + i) % 256);
    }

    uint32_t checksum = 0;

    for (size_t i = 0; i < sizeof(temporaryBuffer); i++) {
      checksum += temporaryBuffer[i];
    }

    Serial.print("[Worker] Checksum = ");
    Serial.println(checksum);

    vTaskDelay(pdMS_TO_TICKS(3000));
  }
}

void MonitorTask(void *parameter) {
  while (1) {
    vTaskDelay(pdMS_TO_TICKS(2000));

    /*
     * Read the minimum remaining stack space of each task.
     *
     * The value is not the current free stack.
     * It is the lowest free-stack value observed since
     * the task was created.
     */
    UBaseType_t workerHighWaterMark =
        uxTaskGetStackHighWaterMark(workerTaskHandle);

    /*
     * Check the current task's own stack.
     *
     * NULL means the task that is currently running.
     */
    UBaseType_t monitorHighWaterMark = uxTaskGetStackHighWaterMark(NULL);

    Serial.println();
    Serial.println("--- Stack Monitor ---");

    Serial.print("WorkerTask minimum free stack: ");
    Serial.print(workerHighWaterMark);
    Serial.println(" bytes");

    Serial.print("MonitorTask minimum free stack: ");
    Serial.print(monitorHighWaterMark);
    Serial.println(" bytes");

    /*
     * A warning threshold is only a learning aid.
     *
     * The correct safety margin depends on the application,
     * call depth, interrupts, libraries, and worst-case paths.
     */
    if (workerHighWaterMark < 512) {
      Serial.println("Warning: WorkerTask stack margin is low.");
    }

    if (monitorHighWaterMark < 512) {
      Serial.println("Warning: MonitorTask stack margin is low.");
    }

    Serial.println("---------------------");
  }
}

void setup() {
  Serial.begin(115200);
  vTaskDelay(pdMS_TO_TICKS(1000));

  Serial.println();
  Serial.println("--- Static Task Memory and Stack Monitoring Demo ---");

  /*
   * Create WorkerTask using application-provided memory.
   *
   * The returned value is the task handle.
   */
  workerTaskHandle =
      xTaskCreateStatic(WorkerTask, "Worker Task", WORKER_STACK_SIZE, NULL,
                        WORKER_PRIORITY, workerStack, &workerTaskControlBlock);

  if (workerTaskHandle == NULL) {
    Serial.println("Failed to create WorkerTask.");
    return;
  }

  /*
   * Create MonitorTask using another independent stack
   * and task control block.
   */
  monitorTaskHandle = xTaskCreateStatic(
      MonitorTask, "Monitor Task", MONITOR_STACK_SIZE, NULL, MONITOR_PRIORITY,
      monitorStack, &monitorTaskControlBlock);

  if (monitorTaskHandle == NULL) {
    Serial.println("Failed to create MonitorTask.");
    return;
  }

  /*
   * setup() runs inside an Arduino-created task.
   * Delete that task after initialization is complete.
   */
  vTaskDelete(NULL);
}

void loop() {
  // Not used.
}