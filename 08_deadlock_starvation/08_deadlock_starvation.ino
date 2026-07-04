#include <Arduino.h>

/**
 * FreeRTOS Deadlock Demo: Dining Philosophers Hierarchy Solution
 *
 * Learning goals:
 * 1. Understand how deadlock happens when tasks take multiple mutexes.
 * 2. Learn the lock ordering / hierarchy solution.
 * 3. Always take multiple mutexes in the same global order.
 * 4. Use one mutex to represent one shared resource.
 *
 * Problem:
 * - Each philosopher needs two chopsticks to eat.
 * - Each chopstick is protected by one mutex.
 * - If everyone takes one chopstick and waits for another,
 *   the system can deadlock.
 *
 * Solution:
 * - Give every chopstick a number.
 * - Always take the lower-numbered chopstick first.
 * - Then take the higher-numbered chopstick.
 * - This prevents circular waiting, so deadlock cannot occur.
 */

#if CONFIG_FREERTOS_UNICORE
static const BaseType_t app_cpu = 0;
#else
static const BaseType_t app_cpu = 1;
#endif

// Settings
static const int NUM_PHILOSOPHERS = 5;
static const int TASK_STACK_SIZE = 2048;

// Kernel objects
static SemaphoreHandle_t param_sem;
static SemaphoreHandle_t done_sem;
static SemaphoreHandle_t chopstick[NUM_PHILOSOPHERS];

/*
 * Philosopher task
 *
 * Each philosopher has:
 * - a left chopstick
 * - a right chopstick
 *
 * To avoid deadlock, the task always takes the lower-numbered
 * chopstick first, then the higher-numbered chopstick.
 */
void PhilosopherTask(void *parameters) {
  int philosopher_id = *(int *)parameters;

  // Tell setup() that the parameter has been copied safely.
  xSemaphoreGive(param_sem);

  int left_chopstick = philosopher_id;
  int right_chopstick = (philosopher_id + 1) % NUM_PHILOSOPHERS;

  int first_chopstick = min(left_chopstick, right_chopstick);
  int second_chopstick = max(left_chopstick, right_chopstick);

  Serial.printf("P%d needs C%d,C%d\n",
                philosopher_id,
                left_chopstick,
                right_chopstick);

  /*
   * Always take the lower-numbered chopstick first.
   * This fixed order prevents circular waiting.
   */
  xSemaphoreTake(chopstick[first_chopstick], portMAX_DELAY);
  Serial.printf("P%d took C%d\n", philosopher_id, first_chopstick);

  // Small delay to make task interleaving easier to observe.
  vTaskDelay(pdMS_TO_TICKS(1));

  xSemaphoreTake(chopstick[second_chopstick], portMAX_DELAY);
  Serial.printf("P%d took C%d\n", philosopher_id, second_chopstick);

  Serial.printf("P%d eating\n", philosopher_id);

  vTaskDelay(pdMS_TO_TICKS(10));

  // Release in reverse order.
  xSemaphoreGive(chopstick[second_chopstick]);
  Serial.printf("P%d gave C%d\n", philosopher_id, second_chopstick);

  xSemaphoreGive(chopstick[first_chopstick]);
  Serial.printf("P%d gave C%d\n", philosopher_id, first_chopstick);

  xSemaphoreGive(done_sem);

  vTaskDelete(NULL);
}

void setup() {
  Serial.begin(115200);
  vTaskDelay(pdMS_TO_TICKS(1000));

  Serial.println();
  Serial.println("--- FreeRTOS Dining Philosophers: Hierarchy Solution ---");

  // Create semaphores
  param_sem = xSemaphoreCreateBinary();
  done_sem = xSemaphoreCreateCounting(NUM_PHILOSOPHERS, 0);

  if (param_sem == NULL || done_sem == NULL) {
    Serial.println("ERROR: Failed to create semaphores.");
    while (1) {
      vTaskDelay(pdMS_TO_TICKS(1000));
    }
  }

  // Create one mutex for each chopstick
  for (int i = 0; i < NUM_PHILOSOPHERS; i++) {
    chopstick[i] = xSemaphoreCreateMutex();

    if (chopstick[i] == NULL) {
      Serial.println("ERROR: Failed to create chopstick mutex.");
      while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
      }
    }
  }

  // Create philosopher tasks
  for (int i = 0; i < NUM_PHILOSOPHERS; i++) {
    char task_name[24];
    sprintf(task_name, "Philosopher %d", i);

    xTaskCreatePinnedToCore(PhilosopherTask, task_name, TASK_STACK_SIZE, &i, 1,
                            NULL, app_cpu);

    /*
     * Wait until the new task copies the loop variable i.
     *
     * Without this, all tasks might receive the same final value of i,
     * because &i points to the same stack variable in setup().
     */
    xSemaphoreTake(param_sem, portMAX_DELAY);
  }

  // Wait until all philosophers finish eating
  for (int i = 0; i < NUM_PHILOSOPHERS; i++) {
    xSemaphoreTake(done_sem, portMAX_DELAY);
  }

  Serial.println("Done! No deadlock occurred.");
}

void loop() { vTaskDelay(pdMS_TO_TICKS(1000)); }