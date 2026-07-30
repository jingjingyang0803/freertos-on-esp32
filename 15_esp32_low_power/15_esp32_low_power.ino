#include <Arduino.h>

#include "esp_sleep.h"

/**
 * ESP32 Deep Sleep Timer Wake-up Demo
 *
 * Learning goals:
 * 1. Enter deep sleep with esp_deep_sleep_start().
 * 2. Wake up using the RTC timer.
 * 3. Read the wake-up reason.
 * 4. Store a value across deep-sleep resets with RTC_DATA_ATTR.
 *
 * Demo:
 * - ESP32 starts and prints the boot count.
 * - It simulates one sensor measurement.
 * - It enters deep sleep for 5 seconds.
 * - After timer wake-up, ESP32 restarts and setup() runs again.
 *
 * Important:
 * - Deep sleep stops normal CPU execution.
 * - esp_deep_sleep_start() does not return.
 * - After waking from deep sleep, the application starts again
 *   from setup().
 * - Normal RAM contents are lost.
 * - Variables marked RTC_DATA_ATTR can remain available across
 *   deep-sleep wake-ups.
 */

static const uint64_t MICROSECONDS_PER_SECOND = 1000000ULL;
static const uint64_t SLEEP_TIME_SECONDS = 5;

/*
 * RTC_DATA_ATTR places this variable in RTC memory.
 *
 * Normal global variables are reinitialized after deep-sleep wake-up,
 * but bootCount can retain its value.
 */
RTC_DATA_ATTR uint32_t bootCount = 0;

void printWakeupReason() {
  esp_sleep_wakeup_cause_t wakeupReason = esp_sleep_get_wakeup_cause();

  Serial.print("Wake-up reason: ");

  switch (wakeupReason) {
  case ESP_SLEEP_WAKEUP_TIMER:
    Serial.println("RTC timer");
    break;

  case ESP_SLEEP_WAKEUP_EXT0:
    Serial.println("external signal using EXT0");
    break;

  case ESP_SLEEP_WAKEUP_EXT1:
    Serial.println("external signal using EXT1");
    break;

  case ESP_SLEEP_WAKEUP_TOUCHPAD:
    Serial.println("touchpad");
    break;

  case ESP_SLEEP_WAKEUP_ULP:
    Serial.println("ULP processor");
    break;

  default:
    /*
     * On the first normal power-up, the device was not
     * awakened from deep sleep.
     */
    Serial.println("normal power-on or reset");
    break;
  }
}

void simulateSensorMeasurement() {
  /*
   * Simulate one short measurement before sleeping.
   *
   * A real low-power sensor node might:
   * 1. Power the sensor.
   * 2. Read the sensor.
   * 3. Save or transmit the result.
   * 4. Return to deep sleep.
   */
  int temperature = 20 + (bootCount % 6);

  Serial.print("Measured temperature: ");
  Serial.print(temperature);
  Serial.println(" degrees C");
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println();
  Serial.println("--- ESP32 Deep Sleep Demo ---");

  /*
   * Increment the value stored in RTC memory.
   *
   * This shows that bootCount survives deep-sleep wake-ups.
   */
  bootCount++;

  Serial.print("Boot count: ");
  Serial.println(bootCount);

  printWakeupReason();

  simulateSensorMeasurement();

  /*
   * Configure the RTC timer as a wake-up source.
   *
   * The duration is specified in microseconds.
   * 5 seconds = 5 * 1,000,000 microseconds
   *
   * esp_err_t is the standard ESP-IDF error-code type.
   *
   * ESP_OK means the function completed successfully.
   * Other values represent specific errors.
   */
  esp_err_t result = esp_sleep_enable_timer_wakeup(SLEEP_TIME_SECONDS *
                                                   MICROSECONDS_PER_SECOND);

  if (result != ESP_OK) {
    Serial.print("Failed to configure timer wake-up. Error: ");
    Serial.println(static_cast<int>(result));
    return;
  }

  Serial.print("Sleeping for ");
  Serial.print(SLEEP_TIME_SECONDS);
  Serial.println(" seconds...");

  /*
   * Wait until all pending Serial data has been transmitted.
   *
   * Without Serial.flush(), the ESP32 may enter deep sleep
   * before the final Serial message is completely sent.
   */
  Serial.flush();

  /*
   * Enter deep sleep.
   *
   * This function does not return.
   *
   * After timer wake-up, the ESP32 restarts and setup()
   * executes again.
   */
  esp_deep_sleep_start();

  // This line is never reached.
  Serial.println("This will never be printed.");
}

void loop() {
  // Not used because setup() enters deep sleep.
}