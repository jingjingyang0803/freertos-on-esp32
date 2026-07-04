#include <Arduino.h>

/**
 * FreeRTOS Interrupt Challenge: ADC Sampling with Double Buffer
 *
 * Board:
 * - ESP32-S3
 * - Arduino-ESP32 Core 3.x
 *
 * Learning goals:
 * 1. Use a hardware timer interrupt to trigger periodic ADC sampling.
 * 2. Keep ISR very short.
 * 3. Use double buffering to avoid data corruption.
 * 4. Notify a task from ISR with a task notification.
 * 5. Read ADC in a task, not in the ISR.
 * 6. Process sampled data in another task.
 * 7. Protect shared variables with a critical section.
 *
 * Demo:
 * - Hardware timer fires every 100 ms.
 * - Timer ISR only wakes SamplingTask.
 * - SamplingTask reads ADC and fills double buffer.
 * - Each buffer stores 10 ADC samples.
 * - When one buffer is full, SamplingTask notifies ProcessingTask.
 * - ProcessingTask calculates the average.
 * - CliTask reads Serial input.
 * - Command "avg" prints the latest average.
 */

#if CONFIG_FREERTOS_UNICORE
static const BaseType_t app_cpu = 0;
#else
static const BaseType_t app_cpu = 1;
#endif

// -------------------- Settings --------------------

// ESP32-S3 ADC input pin.
static const int ADC_PIN = 1;

// Each buffer stores 10 samples.
// Timer period is 100 ms, so one full buffer represents 1 second of data.
static const int SAMPLES_PER_BUFFER = 10;

// CLI task polling delay.
static const int CLI_DELAY_MS = 20;

// Serial command settings.
static const char AVG_COMMAND[] = "avg";
static const char RAW_COMMAND[] = "raw";
static const int CMD_BUF_LEN = 64;

/*
 * Timer settings.
 *
 * Goal:
 * - Trigger one sampling event every 100 ms.
 * - 10 samples × 100 ms = 1 second per average.
 *
 * Why 1 MHz?
 * - 1 MHz makes 1 tick = 1 microsecond.
 * - This makes the alarm value easy to calculate.
 *
 * 100 ms = 100000 us = 100000 ticks.
 */
static const uint32_t TIMER_FREQUENCY_HZ = 1000000;
static const uint64_t TIMER_ALARM_TICKS = 100000;

// -------------------- Globals --------------------

static hw_timer_t *sample_timer = NULL;

static TaskHandle_t sampling_task_handle = NULL;
static TaskHandle_t processing_task_handle = NULL;

/*
 * Spinlock for shared data.
 *
 * Used by:
 * - SamplingTask
 * - ProcessingTask
 * - CliTask
 */
static portMUX_TYPE spinlock = portMUX_INITIALIZER_UNLOCKED;

// Double buffer.
// SamplingTask writes into one buffer.
// ProcessingTask reads a full buffer after it becomes ready.
static volatile uint16_t sample_buffer[2][SAMPLES_PER_BUFFER];

static volatile int write_buffer_index = 0;
static volatile int write_sample_index = 0;

/*
 * ready_buffer_index:
 * -1 = no full buffer is ready
 *  0 = buffer 0 is ready
 *  1 = buffer 1 is ready
 */
static volatile int ready_buffer_index = -1;

// True if a new buffer becomes full while the previous full buffer is still pending.
static volatile bool buffer_overrun = false;

// Latest calculated average.
// Written by ProcessingTask, read by CliTask.
static float latest_average = 0.0f;


/*
 * Program flow:
 *
 * Hardware timer fires every 100 ms
 *   -> OnSampleTimer() wakes SamplingTask
 *
 * SamplingTask
 *   -> reads ADC
 *   -> fills double buffer
 *   -> wakes ProcessingTask when one buffer is full
 *
 * ProcessingTask
 *   -> copies full buffer
 *   -> calculates average
 *
 * CliTask
 *   -> reads Serial commands
 *   -> "avg" prints the latest average
 *   -> "raw" prints one immediate ADC reading
 */

// -------------------- ISR --------------------

/*
 * Hardware timer ISR
 *
 * Runs every 100 ms.
 *
 * ISR job:
 * - Only notify SamplingTask.
 *
 * Important:
 * - No analogRead() here.
 * - No Serial.print().
 * - No delay().
 * - No heavy work.
 */
void IRAM_ATTR OnSampleTimer()
{
  BaseType_t higher_priority_task_woken = pdFALSE;

  /*
   * Wake SamplingTask from ISR.
   *
   * The actual ADC read happens in SamplingTask,
   * not in interrupt context.
   */
  vTaskNotifyGiveFromISR(
    sampling_task_handle,
    &higher_priority_task_woken
  );

  // Request context switch if a higher-priority task was woken.
  if (higher_priority_task_woken == pdTRUE)
  {
    portYIELD_FROM_ISR();
  }
}

// -------------------- Tasks --------------------

/*
 * SamplingTask
 *
 * Waits for timer ISR notification.
 * Reads one ADC sample.
 * Stores it into the active write buffer.
 * When the buffer is full, it notifies ProcessingTask.
 */
void SamplingTask(void *parameter)
{
  while (1)
  {
    /*
     * Wait until the timer ISR says it is time to sample.
     *
     * pdTRUE:
     * - Clear the notification count when received.
     *
     * portMAX_DELAY:
     * - Wait forever.
     */
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

    /*
     * ADC read happens in task context.
     *
     * This is safer than calling analogRead() directly inside the ISR.
     */
    uint16_t sample = analogRead(ADC_PIN);

    bool notify_processing = false;

    /*
     * Protect shared buffer state.
     *
     * SamplingTask writes buffer state.
     * ProcessingTask reads and clears ready_buffer_index.
     */
    portENTER_CRITICAL(&spinlock);

    if (ready_buffer_index == -1)
    {
      sample_buffer[write_buffer_index][write_sample_index] = sample;
      write_sample_index++;

      if (write_sample_index >= SAMPLES_PER_BUFFER)
      {
        // Mark the current buffer as ready.
        ready_buffer_index = write_buffer_index;

        // Switch to the other buffer.
        write_buffer_index = 1 - write_buffer_index;
        write_sample_index = 0;

        notify_processing = true;
      }
    }
    else
    {
      /*
       * ProcessingTask has not handled the previous full buffer yet.
       * Do not overwrite unread data.
       */
      buffer_overrun = true;
    }

    portEXIT_CRITICAL(&spinlock);

    /*
     * Notify ProcessingTask outside the critical section.
     *
     * This keeps the protected section short.
     */
    if (notify_processing)
    {
      xTaskNotifyGive(processing_task_handle);
    }
  }
}

/*
 * ProcessingTask
 *
 * Waits for notification from SamplingTask.
 * Copies the full buffer quickly.
 * Calculates average outside the critical section.
 */
void ProcessingTask(void *parameter)
{
  uint16_t local_buffer[SAMPLES_PER_BUFFER];

  while (1)
  {
    // Wait until SamplingTask notifies that one buffer is full.
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

    int buffer_to_process = -1;

    /*
     * Copy shared data quickly.
     *
     * Keep this critical section short.
     */
    portENTER_CRITICAL(&spinlock);

    buffer_to_process = ready_buffer_index;

    if (buffer_to_process != -1)
    {
      for (int i = 0; i < SAMPLES_PER_BUFFER; i++)
      {
        local_buffer[i] = sample_buffer[buffer_to_process][i];
      }

      // Mark that no buffer is currently waiting.
      ready_buffer_index = -1;
    }

    portEXIT_CRITICAL(&spinlock);

    /*
     * Process data outside the critical section.
     */
    if (buffer_to_process != -1)
    {
      uint32_t sum = 0;

      for (int i = 0; i < SAMPLES_PER_BUFFER; i++)
      {
        sum += local_buffer[i];
      }

      float avg = (float)sum / SAMPLES_PER_BUFFER;

      // latest_average is shared with CliTask.
      portENTER_CRITICAL(&spinlock);
      latest_average = avg;
      portEXIT_CRITICAL(&spinlock);
    }
  }
}

/*
 * CliTask
 *
 * Reads Serial input.
 *
 * Commands:
 * - avg : print latest calculated average
 * - raw : print one immediate ADC reading
 */
void CliTask(void *parameter)
{
  char cmd_buf[CMD_BUF_LEN];
  int cmd_index = 0;

  memset(cmd_buf, 0, sizeof(cmd_buf));

  while (1)
  {
    while (Serial.available() > 0)
    {
      char c = Serial.read();

      if (c == '\r' || c == '\n')
      {
        Serial.println();

        cmd_buf[cmd_index] = '\0';

        if (strcmp(cmd_buf, AVG_COMMAND) == 0)
        {
          float avg_copy;
          bool overrun_copy;

          /*
           * Copy shared values quickly.
           */
          portENTER_CRITICAL(&spinlock);
          avg_copy = latest_average;
          overrun_copy = buffer_overrun;
          buffer_overrun = false;
          portEXIT_CRITICAL(&spinlock);

          Serial.print("Average: ");
          Serial.println(avg_copy);

          if (overrun_copy)
          {
            Serial.println("Warning: sample buffer overrun. Some ADC samples were dropped.");
          }
        }
        else if (strcmp(cmd_buf, RAW_COMMAND) == 0)
        {
          /*
           * Debug command.
           *
           * This reads the ADC immediately from CliTask.
           * It does not use the double buffer.
           */
          uint16_t raw = analogRead(ADC_PIN);

          Serial.print("Raw ADC: ");
          Serial.println(raw);
        }
        else if (cmd_index > 0)
        {
          Serial.println("Unknown command. Try: avg or raw");
        }

        memset(cmd_buf, 0, sizeof(cmd_buf));
        cmd_index = 0;
      }
      else
      {
        // Echo typed character.
        Serial.print(c);

        if (cmd_index < CMD_BUF_LEN - 1)
        {
          cmd_buf[cmd_index] = c;
          cmd_index++;
        }
      }
    }

    vTaskDelay(pdMS_TO_TICKS(CLI_DELAY_MS));
  }
}

// -------------------- Setup --------------------

void setup()
{
  Serial.begin(115200);
  vTaskDelay(pdMS_TO_TICKS(1000));

  Serial.println();
  Serial.println("--- FreeRTOS ADC Sampling Interrupt Demo ---");
  Serial.println("Type 'avg' and press Enter.");
  Serial.println("Type 'raw' and press Enter.");

  /*
   * Use 12-bit ADC resolution.
   *
   * Typical ADC output range:
   * - low input voltage  -> smaller value
   * - high input voltage -> larger value
   */
  analogReadResolution(12);

  /*
   * Create SamplingTask first.
   *
   * The timer ISR uses sampling_task_handle,
   * so this task must exist before the timer starts.
   */
  BaseType_t sampling_result = xTaskCreatePinnedToCore(
    SamplingTask,
    "Sampling Task",
    4096,
    NULL,
    3,
    &sampling_task_handle,
    app_cpu
  );

  if (sampling_result != pdPASS)
  {
    Serial.println("ERROR: Failed to create SamplingTask");

    while (1)
    {
      vTaskDelay(pdMS_TO_TICKS(1000));
    }
  }

  /*
   * Create ProcessingTask.
   *
   * SamplingTask will notify this task when one buffer is full.
   */
  BaseType_t processing_result = xTaskCreatePinnedToCore(
    ProcessingTask,
    "Processing Task",
    4096,
    NULL,
    2,
    &processing_task_handle,
    app_cpu
  );

  if (processing_result != pdPASS)
  {
    Serial.println("ERROR: Failed to create ProcessingTask");

    while (1)
    {
      vTaskDelay(pdMS_TO_TICKS(1000));
    }
  }

  /*
   * Create CLI task.
   *
   * Lower priority than SamplingTask and ProcessingTask.
   */
  BaseType_t cli_result = xTaskCreatePinnedToCore(
    CliTask,
    "CLI Task",
    4096,
    NULL,
    1,
    NULL,
    app_cpu
  );

  if (cli_result != pdPASS)
  {
    Serial.println("ERROR: Failed to create CliTask");

    while (1)
    {
      vTaskDelay(pdMS_TO_TICKS(1000));
    }
  }

  /*
   * Start hardware timer.
   *
   * Arduino-ESP32 Core 3.x timer API.
   *
   * timerBegin(TIMER_FREQUENCY_HZ):
   * - Create a hardware timer.
   * - Set timer frequency to 1 MHz.
   * - 1 timer tick = 1 microsecond.
   */
  sample_timer = timerBegin(TIMER_FREQUENCY_HZ);

  if (sample_timer == NULL)
  {
    Serial.println("ERROR: Failed to create timer");

    while (1)
    {
      vTaskDelay(pdMS_TO_TICKS(1000));
    }
  }

  /*
   * Attach the ISR function to the timer.
   *
   * OnSampleTimer() runs whenever the timer alarm fires.
   */
  timerAttachInterrupt(sample_timer, &OnSampleTimer);

  /*
   * Configure and start the timer alarm.
   *
   * Arguments:
   * 1. sample_timer: timer handle
   * 2. TIMER_ALARM_TICKS: alarm period in ticks
   * 3. true: auto-reload
   * 4. 0: repeat forever
   */
  timerAlarm(sample_timer, TIMER_ALARM_TICKS, true, 0);

  /*
   * Delete Arduino setup/loop task.
   *
   * After this, only the created FreeRTOS tasks remain.
   */
  vTaskDelete(NULL);
}

void loop()
{
  // Not used.
}