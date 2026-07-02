#include <Arduino.h>
#include "freertos/timers.h"

/**
 * FreeRTOS Software Timer Demo
 *
 * Idea:
 * - Type a character -> LED turns ON
 * - Restart a 5-second one-shot timer
 * - If no input for 5 seconds -> timer expires -> LED turns OFF
 *
 * Important:
 * Software timer callback is NOT a hardware ISR.
 * It runs in the FreeRTOS Timer Service Task.
 * Keep callback short.
 */

#if CONFIG_FREERTOS_UNICORE
static const BaseType_t app_cpu = 0;
#else
static const BaseType_t app_cpu = 1;
#endif

static const int led_pin = LED_BUILTIN;

// Convert 5000 ms to FreeRTOS ticks
static const TickType_t dim_delay = pdMS_TO_TICKS(5000);

// Timer handle
static TimerHandle_t one_shot_timer = NULL;

// Set by timer callback, handled by task
static volatile bool turn_led_off_request = false;

/*
 * Timer callback
 *
 * Called when the 5-second timer expires.
 * Do not print or do heavy work here.
 */
void autoDimmerCallback(TimerHandle_t xTimer)
{
  turn_led_off_request = true;
}

/*
 * CLI task
 *
 * Handles Serial input and LED control.
 */
void doCLI(void *parameters)
{
  pinMode(led_pin, OUTPUT);
  digitalWrite(led_pin, LOW);

  while (1)
  {
    /*
     * Timer expired:
     * turn LED off in the task, not in the callback.
     */
    if (turn_led_off_request)
    {
      turn_led_off_request = false;

      digitalWrite(led_pin, LOW);
      Serial.println();
      Serial.println("Timer expired: LED OFF");
    }

    /*
     * User input:
     * turn LED on and restart the timer.
     */
    if (Serial.available() > 0)
    {
      char c = Serial.read();

      Serial.print(c);

      digitalWrite(led_pin, HIGH);

      Serial.println();
      Serial.println("Input received: LED ON");

      /*
       * xTimerReset():
       * - starts the timer if it is stopped
       * - restarts the countdown if it is already running
       */
      if (xTimerReset(one_shot_timer, portMAX_DELAY) == pdPASS)
      {
        Serial.println("Timer reset OK");
      }
      else
      {
        Serial.println("ERROR: Timer reset failed");
      }
    }

    // Avoid busy-looping
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}

void setup()
{
  Serial.begin(115200);
  delay(1000);

  Serial.println();
  Serial.println("--- FreeRTOS Software Timer Demo ---");
  Serial.println("Type a character.");
  Serial.println("LED turns OFF after 5 seconds of inactivity.");

  /*
   * Create a one-shot timer.
   *
   * pdFALSE means one-shot:
   * the timer expires once, then stops.
   */
  one_shot_timer = xTimerCreate(
    "One-shot timer",
    dim_delay,
    pdFALSE,
    NULL,
    autoDimmerCallback
  );

  if (one_shot_timer == NULL)
  {
    Serial.println("ERROR: Timer creation failed");

    while (1)
    {
      vTaskDelay(pdMS_TO_TICKS(1000));
    }
  }

  BaseType_t task_result = xTaskCreatePinnedToCore(
    doCLI,
    "Do CLI",
    4096,
    NULL,
    1,
    NULL,
    app_cpu
  );

  if (task_result != pdPASS)
  {
    Serial.println("ERROR: Task creation failed");

    while (1)
    {
      vTaskDelay(pdMS_TO_TICKS(1000));
    }
  }
}

void loop()
{
  vTaskDelay(pdMS_TO_TICKS(1000));
}