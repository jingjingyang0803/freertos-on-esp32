#include <Arduino.h>

/**
 * FreeRTOS Interrupt Demo
 *
 * Demo:
 * - Button press triggers GPIO interrupt.
 * - ISR gives a binary semaphore.
 * - ButtonTask wakes up, debounces, toggles LED.
 *
 * Wiring:
 * GPIO 4 ---- Button ---- GND
 *
 * With INPUT_PULLUP:
 * - Not pressed = HIGH
 * - Pressed     = LOW
 *
 * FALLING interrupt means HIGH -> LOW, so it triggers on button press.
 */

static const int button_pin = 4;
static const int led_pin = LED_BUILTIN;

static SemaphoreHandle_t button_sem = NULL;

/*
 * ISR: Interrupt Service Routine
 *
 * Keep it short:
 * - no Serial.print()
 * - no delay()
 * - no heavy work
 *
 * Only notify ButtonTask.
 */
void IRAM_ATTR ButtonISR()
{
  BaseType_t higher_priority_task_woken = pdFALSE;

  // In ISR, use xSemaphoreGiveFromISR(), not xSemaphoreGive().
  xSemaphoreGiveFromISR(button_sem, &higher_priority_task_woken);

  // If a higher-priority task was woken, switch to it after ISR exits.
  if (higher_priority_task_woken == pdTRUE)
  {
    portYIELD_FROM_ISR();
  }
}

/*
 * ButtonTask
 *
 * Waits for the ISR event, then handles the button press.
 */
void ButtonTask(void *parameter)
{
  bool led_state = false;

  while (1)
  {
    // Wait here until ISR gives the semaphore.
    if (xSemaphoreTake(button_sem, portMAX_DELAY) == pdTRUE)
    {
      // Debounce: wait for mechanical bouncing to settle.
      vTaskDelay(pdMS_TO_TICKS(50));

      // Confirm the button is still pressed.
      if (digitalRead(button_pin) == LOW)
      {
        led_state = !led_state;
        digitalWrite(led_pin, led_state);

        // Printing is safe here because this is a task, not ISR.
        Serial.println("Button pressed");
      }
    }
  }
}

void setup()
{
  Serial.begin(115200);
  delay(1000);

  Serial.println();
  Serial.println("--- FreeRTOS Interrupt Demo ---");

  pinMode(button_pin, INPUT_PULLUP);
  pinMode(led_pin, OUTPUT);
  digitalWrite(led_pin, LOW);

  // Create semaphore before enabling interrupt.
  button_sem = xSemaphoreCreateBinary();

  if (button_sem == NULL)
  {
    Serial.println("ERROR: Failed to create semaphore");

    while (1)
    {
      vTaskDelay(pdMS_TO_TICKS(1000));
    }
  }

  BaseType_t task_result = xTaskCreatePinnedToCore(
    ButtonTask,
    "Button Task",
    4096,
    NULL,
    1,
    NULL,
    1
  );

  if (task_result != pdPASS)
  {
    Serial.println("ERROR: Failed to create ButtonTask");

    while (1)
    {
      vTaskDelay(pdMS_TO_TICKS(1000));
    }
  }

  /*
   * Attach interrupt after semaphore and task are ready.
   *
   * FALLING = trigger when button goes HIGH -> LOW.
   */
  attachInterrupt(
    digitalPinToInterrupt(button_pin),
    ButtonISR,
    FALLING
  );
}

void loop()
{
  // loop() is unused.
  vTaskDelay(pdMS_TO_TICKS(1000));
}