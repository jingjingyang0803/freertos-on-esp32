/**
 * FreeRTOS Memory Management Demo
 *
 * Learning goals:
 * 1. Allocate memory from the FreeRTOS heap with pvPortMalloc().
 * 2. Free memory back to the FreeRTOS heap with vPortFree().
 * 3. Understand why dynamic memory needs to be released after use.
 * 4. Understand that a pointer should be set to NULL after freeing.
 * 5. Learn that shared global variables are not ideal in real projects.
 *
 * Demo:
 * - ReadSerialTask reads a string from Serial.
 * - It allocates memory for the string on the FreeRTOS heap.
 * - PrintMessageTask prints the message.
 * - PrintMessageTask frees the heap memory after printing.
 *
 * Important note:
 * This demo uses shared global variables: msg_ptr and msg_ready.
 * This is acceptable for learning heap usage only.
 * In real projects, use a Queue or Mutex to avoid race conditions.
 */

#include <Arduino.h>

#if CONFIG_FREERTOS_UNICORE
static const BaseType_t app_cpu = 0;
#else
static const BaseType_t app_cpu = 1;
#endif

// Pointer to memory allocated from the FreeRTOS heap
static char *msg_ptr = NULL;

// Flag used to tell PrintMessageTask that a message is ready
static volatile bool msg_ready = false;

/*
 * Task 1: Read string from Serial and allocate heap memory
 */
void ReadSerialTask(void *parameter)
{
  while (1)
  {
    if (Serial.available() > 0)
    {
      String input = Serial.readStringUntil('\n');
      input.trim();

      if (input.length() > 0 && msg_ready == false)
      {
        // +1 for the null terminator '\0'
        int len = input.length() + 1;

        // Allocate memory from the FreeRTOS heap
        msg_ptr = (char *)pvPortMalloc(len);

        // Stop the program if allocation fails
        configASSERT(msg_ptr);

        // Copy the String content into the heap buffer
        memcpy(msg_ptr, input.c_str(), len);

        // Tell the other task that the message is ready
        msg_ready = true;
      }
    }

    vTaskDelay(pdMS_TO_TICKS(10));
  }
}

/*
 * Task 2: Print message and free heap memory
 */
void PrintMessageTask(void *parameter)
{
  while (1)
  {
    if (msg_ready == true)
    {
      Serial.print("Message: ");
      Serial.println(msg_ptr);

      // Free the memory back to the FreeRTOS heap
      vPortFree(msg_ptr);

      // Avoid using a dangling pointer
      msg_ptr = NULL;

      // Mark the message as consumed
      msg_ready = false;
    }

    vTaskDelay(pdMS_TO_TICKS(10));
  }
}

void setup()
{
  Serial.begin(115200);
  vTaskDelay(pdMS_TO_TICKS(1000));

  Serial.println();
  Serial.println("--- FreeRTOS Memory Management Demo ---");
  Serial.println("Enter a string:");

  xTaskCreatePinnedToCore(
    ReadSerialTask,
    "Read Serial",
    4096,
    NULL,
    1,
    NULL,
    app_cpu
  );

  xTaskCreatePinnedToCore(
    PrintMessageTask,
    "Print Message",
    2048,
    NULL,
    1,
    NULL,
    app_cpu
  );
}

void loop()
{
  vTaskDelay(pdMS_TO_TICKS(1000));
}