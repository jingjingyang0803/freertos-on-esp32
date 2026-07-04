#include <Arduino.h>

/**
 * FreeRTOS Multicore Core Affinity Demo
 *
 * Learning goals:
 * 1. Understand task core affinity.
 * 2. Pin one task to Core 0.
 * 3. Pin one task to Core 1.
 * 4. Create one task with no core affinity.
 * 5. Use xPortGetCoreID() to check where each task is running.
 *
 * Demo:
 * - Core0Task is pinned to Core 0.
 * - Core1Task is pinned to Core 1.
 * - NoAffinityTask is not pinned to any core.
 *
 * Important:
 * - tskNO_AFFINITY means the scheduler is allowed to run the task
 *   on either core.
 * - It does not mean the task will switch cores regularly.
 */

static const int TASK_STACK_SIZE = 2048;

static const int CORE_0 = 0;
static const int CORE_1 = 1;

void Core0Task(void *parameter)
{
  while (1)
  {
    Serial.print("Core0Task pinned to Core 0, running on Core ");
    Serial.println(xPortGetCoreID());

    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}

void Core1Task(void *parameter)
{
  while (1)
  {
    Serial.print("Core1Task pinned to Core 1, running on Core ");
    Serial.println(xPortGetCoreID());

    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}

void NoAffinityTask(void *parameter)
{
  while (1)
  {
    Serial.print("NoAffinityTask not pinned, currently running on Core ");
    Serial.println(xPortGetCoreID());

    vTaskDelay(pdMS_TO_TICKS(700));
  }
}

void setup()
{
  Serial.begin(115200);
  vTaskDelay(pdMS_TO_TICKS(1000));

  Serial.println();
  Serial.println("--- FreeRTOS Multicore Core Affinity Demo ---");

  xTaskCreatePinnedToCore(
    Core0Task,
    "Core 0 Task",
    TASK_STACK_SIZE,
    NULL,
    1,
    NULL,
    CORE_0
  );

  xTaskCreatePinnedToCore(
    Core1Task,
    "Core 1 Task",
    TASK_STACK_SIZE,
    NULL,
    1,
    NULL,
    CORE_1
  );

  /*
   * tskNO_AFFINITY means this task is not pinned.
   *
   * The scheduler may run it on either core,
   * but it may also keep it on the same core.
   */
  xTaskCreatePinnedToCore(
    NoAffinityTask,
    "No Affinity Task",
    TASK_STACK_SIZE,
    NULL,
    1,
    NULL,
    tskNO_AFFINITY
  );

  vTaskDelete(NULL);
}

void loop()
{
  // Not used.
}