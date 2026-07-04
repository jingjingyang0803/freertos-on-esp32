#include <Arduino.h>

/**
 * FreeRTOS Priority Inversion Demo
 *
 * Version A:
 * - USE_MUTEX = 0
 * - Use binary semaphore
 * - No priority inheritance
 * - MediumTask can delay LowTask, so HighTask waits longer
 *
 * Version B:
 * - USE_MUTEX = 1
 * - Use mutex
 * - FreeRTOS mutex supports priority inheritance
 * - LowTask can be temporarily boosted, so HighTask waits less
 */

#define USE_MUTEX 0   // Change to 1 to test mutex version

#if CONFIG_FREERTOS_UNICORE
static const BaseType_t app_cpu = 0;
#else
static const BaseType_t app_cpu = 1;
#endif

static const int TASK_STACK_SIZE = 4096;

static SemaphoreHandle_t shared_resource = NULL;

/*
 * Busy CPU work.
 *
 * This simulates a task using CPU time.
 * It does not call delay(), so task scheduling behavior is easier to see.
 */
void BusyWork(uint32_t loops)
{
  volatile uint32_t dummy = 0;

  for (uint32_t i = 0; i < loops; i++)
  {
    dummy++;
  }
}

/*
 * LowTask
 *
 * Priority: low
 *
 * It takes the shared resource first and holds it for a while.
 * HighTask will later need the same resource.
 */
void LowTask(void *parameter)
{
  Serial.println("Low: starting");

  Serial.println("Low: taking resource");
  xSemaphoreTake(shared_resource, portMAX_DELAY);

  Serial.println("Low: resource taken");
  Serial.println("Low: working while holding resource");

  for (int i = 0; i < 6; i++)
  {
    Serial.print("Low: work ");
    Serial.println(i + 1);

    BusyWork(12000000);
  }

  Serial.println("Low: releasing resource");
  xSemaphoreGive(shared_resource);

  Serial.println("Low: done");

  vTaskDelete(NULL);
}

/*
 * MediumTask
 *
 * Priority: medium
 *
 * It does not need the shared resource.
 * It only consumes CPU time.
 *
 * In the binary semaphore version, this task can keep preempting LowTask.
 */
void MediumTask(void *parameter)
{
  // Start after LowTask has taken the resource.
  vTaskDelay(pdMS_TO_TICKS(100));

  Serial.println("Medium: starting CPU work");

  for (int i = 0; i < 12; i++)
  {
    Serial.print("Medium: work ");
    Serial.println(i + 1);

    BusyWork(12000000);
  }

  Serial.println("Medium: done");

  vTaskDelete(NULL);
}

/*
 * HighTask
 *
 * Priority: high
 *
 * It tries to take the same resource held by LowTask.
 * It measures how long it waits.
 */
void HighTask(void *parameter)
{
  // Start after LowTask has taken the resource.
  vTaskDelay(pdMS_TO_TICKS(150));

  Serial.println("High: wants resource");

  TickType_t start_time = xTaskGetTickCount();

  xSemaphoreTake(shared_resource, portMAX_DELAY);

  TickType_t wait_ticks = xTaskGetTickCount() - start_time;

  Serial.print("High: got resource after ");
  Serial.print(pdTICKS_TO_MS(wait_ticks));
  Serial.println(" ms");

  xSemaphoreGive(shared_resource);

  Serial.println("High: released resource");
  Serial.println("High: done");

  vTaskDelete(NULL);
}

void setup()
{
  Serial.begin(115200);
  vTaskDelay(pdMS_TO_TICKS(1000));

  Serial.println();

#if USE_MUTEX
  Serial.println("--- Version B: Mutex with Priority Inheritance ---");
#else
  Serial.println("--- Version A: Binary Semaphore without Priority Inheritance ---");
#endif

#if USE_MUTEX
  /*
   * Mutex version.
   *
   * FreeRTOS mutexes support priority inheritance.
   */
  shared_resource = xSemaphoreCreateMutex();
#else
  /*
   * Binary semaphore version.
   *
   * Binary semaphores do not provide priority inheritance.
   *
   * A binary semaphore starts empty, so we give it once
   * to make the resource initially available.
   */
  shared_resource = xSemaphoreCreateBinary();
#endif

  if (shared_resource == NULL)
  {
    Serial.println("ERROR: Failed to create resource");

    while (1)
    {
      vTaskDelay(pdMS_TO_TICKS(1000));
    }
  }

#if !USE_MUTEX
  // Binary semaphore starts empty, so give once to make the resource available.
  // Mutex starts unlocked, so this is only needed for the semaphore version.
  xSemaphoreGive(shared_resource);
#endif

  /*
   * Priorities:
   * - LowTask:    priority 1
   * - MediumTask: priority 2
   * - HighTask:   priority 3
   *
   * All tasks are pinned to the same core.
   * This makes priority inversion easier to observe.
   */
  xTaskCreatePinnedToCore(
    LowTask,
    "Low Task",
    TASK_STACK_SIZE,
    NULL,
    1,
    NULL,
    app_cpu
  );

  xTaskCreatePinnedToCore(
    MediumTask,
    "Medium Task",
    TASK_STACK_SIZE,
    NULL,
    2,
    NULL,
    app_cpu
  );

  xTaskCreatePinnedToCore(
    HighTask,
    "High Task",
    TASK_STACK_SIZE,
    NULL,
    3,
    NULL,
    app_cpu
  );

  vTaskDelete(NULL);
}

void loop()
{
  // Not used.
}