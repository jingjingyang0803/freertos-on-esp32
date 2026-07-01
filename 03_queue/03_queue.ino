/**
 * FreeRTOS Queue Demo
 *
 * Learning goals:
 * 1. Create queues with xQueueCreate().
 * 2. Send data between tasks with xQueueSend().
 * 3. Receive data from queues with xQueueReceive().
 * 4. Send simple values, such as int.
 * 5. Send structured data, such as struct Message.
 * 6. Avoid sharing global variables directly between tasks.
 *
 * Demo:
 * - CLI task reads Serial commands.
 * - CLI task sends LED delay values to Blink task through delay_queue.
 * - Blink task sends status messages back to CLI task through msg_queue.
 *
 * Important note:
 * Only the CLI task prints to Serial.
 * This avoids multiple tasks using Serial at the same time.
 */

#include <Arduino.h>

#if CONFIG_FREERTOS_UNICORE
static const BaseType_t app_cpu = 0;
#else
static const BaseType_t app_cpu = 1;
#endif

static const int led_pin = LED_BUILTIN;

QueueHandle_t delay_queue;
QueueHandle_t msg_queue;

struct Message
{
  int blink_delay_ms;       // Current blink delay
  unsigned long counter;    // Number of LED toggles
};

/*
 * BlinkTask
 *
 * Responsibilities:
 * 1. Blink the LED.
 * 2. Check delay_queue for new delay values.
 * 3. Send status messages to msg_queue.
 *
 * Important:
 * This task does NOT print to Serial.
 * It sends messages to CliTask instead.
 */
void BlinkTask(void *parameter)
{
  int blink_delay_ms = 1000;       // Default blink delay
  unsigned long blink_counter = 0;

  pinMode(led_pin, OUTPUT);

  while (1)
  {
    /*
     * Check if CLI sent a new delay value.
     *
     * The last argument is the block time.
     *
     * 0 means:
     * - Do not wait.
     * - If the queue has data, receive it.
     * - If the queue is empty, continue immediately.
     */
    int new_delay_ms;

    if (xQueueReceive(delay_queue, &new_delay_ms, 0) == pdTRUE)
    {
      /*
       * Validate the received value.
       *
       * Very small delay values make the LED annoying
       * and generate too many messages.
       */
      if (new_delay_ms >= 100 && new_delay_ms <= 5000)
      {
        blink_delay_ms = new_delay_ms;
      }
    }

    digitalWrite(led_pin, !digitalRead(led_pin));
    blink_counter++;

    /*
     * Prepare a status message.
     */
    Message msg;
    msg.blink_delay_ms = blink_delay_ms;
    msg.counter = blink_counter;

    /*
     * Send the message to msg_queue.
     *
     * 0 means:
     * - Do not wait if the queue is full.
     *
     * If the queue is full, this message is dropped.
     * For simple status messages, this is acceptable.
     */
    xQueueSend(msg_queue, &msg, 0);

    /*
     * Delay this task.
     *
     * This blocks only BlinkTask.
     * Other tasks, such as CliTask, can still run.
     */
    vTaskDelay(pdMS_TO_TICKS(blink_delay_ms));
  }
}

/*
 * CliTask
 *
 * Responsibilities:
 * 1. Read commands from Serial.
 * 2. Send new delay values to BlinkTask.
 * 3. Receive status messages from BlinkTask.
 * 4. Print all text to Serial.
 */
void CliTask(void *parameter)
{
  Serial.println();
  Serial.println("--- FreeRTOS Queue Demo ---");
  Serial.println("Type command:");
  Serial.println("  delay 1000");
  Serial.println();

  while (1)
  {
    if (Serial.available() > 0)
    {
      String cmd = Serial.readStringUntil('\n');
      cmd.trim();

      if (cmd.startsWith("delay"))
      {
        String number_part = cmd.substring(6);
        int value = number_part.toInt();

        if (value >= 100 && value <= 5000)
        {
          /*
           * Send the delay value to BlinkTask through the queue.
           */
          if (xQueueSend(delay_queue, &value, pdMS_TO_TICKS(100)) == pdTRUE)
          {
            Serial.print("CLI: new delay sent = ");
            Serial.print(value);
            Serial.println(" ms");
          }
          else
          {
            Serial.println("CLI: delay_queue is full");
          }
        }
        else
        {
          Serial.println("CLI: delay must be 100 to 5000 ms");
        }
      }
      else
      {
        Serial.println("CLI: unknown command");
      }
    }

    /*
     * Receive messages from BlinkTask.
     */
    Message msg;

    while (xQueueReceive(msg_queue, &msg, 0) == pdTRUE)
    {
      Serial.print("BlinkTask: counter = ");
      Serial.print(msg.counter);
      Serial.print(", delay = ");
      Serial.print(msg.blink_delay_ms);
      Serial.println(" ms");
    }

    /*
     * Small delay so this task does not use the CPU all the time.
     */
    vTaskDelay(pdMS_TO_TICKS(20));
  }
}

void setup()
{
  Serial.begin(115200);

  /*
   * Give the USB Serial Monitor time to connect.
   */
  delay(1000);

  /*
   * Create delay_queue.
   * This queue can hold up to 5 int values.
   */
  delay_queue = xQueueCreate(5, sizeof(int));

  /*
   * Create msg_queue.
   * This queue can hold up to 10 Message structs.
   */
  msg_queue = xQueueCreate(10, sizeof(Message));

  /*
   * Always check whether queue creation succeeded.
   *
   * If there is not enough heap memory,
   * xQueueCreate() returns NULL.
   */
  if (delay_queue == NULL || msg_queue == NULL)
  {
    Serial.println("ERROR: Failed to create queues");

    while (1)
    {
      vTaskDelay(pdMS_TO_TICKS(1000));
    }
  }

  /*
   * Create BlinkTask.
   */
  BaseType_t blink_result = xTaskCreatePinnedToCore(
    BlinkTask,       // Task function
    "Blink Task",    // Task name
    4096,            // Stack size in bytes on ESP32 Arduino
    NULL,            // Task parameter
    1,               // Priority
    NULL,            // Task handle
    app_cpu          // CPU core
  );

  /*
   * Create CliTask.
   *
   * CliTask uses String and Serial printing,
   * so we give it a larger stack.
   */
  BaseType_t cli_result = xTaskCreatePinnedToCore(
    CliTask,
    "CLI Task",
    8192,
    NULL,
    1,
    NULL,
    app_cpu
  );

  /*
   * Check task creation result.
   */
  if (blink_result != pdPASS || cli_result != pdPASS)
  {
    Serial.println("ERROR: Failed to create tasks");

    while (1)
    {
      vTaskDelay(pdMS_TO_TICKS(1000));
    }
  }
}

void loop()
{
  /*
   * Arduino loop() is also a FreeRTOS task.
   *
   * We do not use it in this demo.
   * Delay it so it does not waste CPU time.
   */
  vTaskDelay(pdMS_TO_TICKS(1000));
}