#include <Arduino.h>

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"

/**
 * FreeRTOS Event Groups Demo
 *
 * Learning goals:
 * 1. Understand what an event group is.
 * 2. Create an event group with xEventGroupCreate().
 * 3. Set event bits with xEventGroupSetBits().
 * 4. Wait for multiple events with xEventGroupWaitBits().
 * 5. Understand the difference between waiting for:
 *    - Any event bit
 *    - All event bits
 *
 * Demo:
 * - WiFiTask simulates connecting to Wi-Fi.
 * - SensorTask simulates initializing a sensor.
 * - SystemTask waits until both operations are complete.
 *
 * Event bits:
 * - WIFI_READY_BIT:   Wi-Fi connection is ready.
 * - SENSOR_READY_BIT: Sensor initialization is ready.
 *
 * Important:
 * - An event group stores multiple Boolean events as individual bits.
 * - Setting a bit means the corresponding event has occurred.
 * - Event groups transfer event states, not actual data.
 */

static const int TASK_STACK_SIZE = 2048;

// Each bit represents one event.
static const EventBits_t WIFI_READY_BIT = BIT0;   // 0000 0001
static const EventBits_t SENSOR_READY_BIT = BIT1; // 0000 0010

/*
 * Combine the Wi-Fi-ready bit and sensor-ready bit into one bit mask.
 *
 * WIFI_READY_BIT:    0000 0001
 * SENSOR_READY_BIT:  0000 0010
 *
 * Bitwise OR:
 *
 *   0000 0001
 * | 0000 0010
 * -----------
 *   0000 0011
 *
 * ALL_READY_BITS does not mean both events have already happened.
 * It only describes which event bits SystemTask is interested in.
 */
static const EventBits_t ALL_READY_BITS = WIFI_READY_BIT | SENSOR_READY_BIT;

EventGroupHandle_t systemEventGroup = NULL;

void WiFiTask(void *parameter) {
  while (1) {
    Serial.println("WiFiTask: Connecting to Wi-Fi...");

    // Simulate a Wi-Fi connection operation.
    vTaskDelay(pdMS_TO_TICKS(3000));

    Serial.println("WiFiTask: Wi-Fi is ready.");

    /*
     * Set WIFI_READY_BIT.
     *
     * Setting a bit does not overwrite other bits
     * already stored in the event group.
     */
    xEventGroupSetBits(systemEventGroup, WIFI_READY_BIT);

    // Wait before repeating the Wi-Fi simulation.
    vTaskDelay(pdMS_TO_TICKS(10000));
  }
}

void SensorTask(void *parameter) {
  while (1) {
    Serial.println("SensorTask: Initializing sensor...");

    // Simulate sensor initialization.
    vTaskDelay(pdMS_TO_TICKS(8000));

    Serial.println("SensorTask: Sensor is ready.");

    xEventGroupSetBits(systemEventGroup, SENSOR_READY_BIT);

    // Wait before the next simulated initialization.
    vTaskDelay(pdMS_TO_TICKS(10000));
  }
}

void SystemTask(void *parameter) {
  while (1) {
    Serial.println();
    Serial.println("SystemTask: Waiting for all components...");

    /*
     * Wait for both WIFI_READY_BIT and SENSOR_READY_BIT.
     *
     * Parameters:
     *
     * systemEventGroup:
     *   The event group to wait on.
     *
     * ALL_READY_BITS:
     *   Specifies which bits to wait for.
     *   It does not mean the returned value contains only those bits.
     *
     * receivedBits may also contain other event bits that were already set.
     *
     * pdTRUE:
     *   Clear the waited bits before returning.
     *   This allows the demonstration to repeat.
     *
     * pdTRUE:
     *   Wait until ALL requested bits are set.
     *
     * portMAX_DELAY:
     *   Wait indefinitely.
     */
    EventBits_t receivedBits =
        xEventGroupWaitBits(systemEventGroup, ALL_READY_BITS,
                            pdTRUE, // Clear the waited bits before returning.
                            pdTRUE, // Wait for all bits in ALL_READY_BITS.
                            portMAX_DELAY);

    /*
     * Check whether all required bits were present.
     *
     * Do not simply use:
     *
     *   receivedBits == ALL_READY_BITS
     *
     * because receivedBits may contain additional unrelated bits.
     *
     * The bitwise AND removes unrelated bits and keeps only
     * the bits listed in ALL_READY_BITS.
     */
    if ((receivedBits & ALL_READY_BITS) == ALL_READY_BITS) {
      Serial.println("--------------------------------");
      Serial.println("SystemTask: All components ready.");
      Serial.println("SystemTask: Starting system...");
      Serial.println("--------------------------------");
    }
  }
}

void setup() {
  Serial.begin(115200);
  vTaskDelay(pdMS_TO_TICKS(1000));

  Serial.println();
  Serial.println("--- FreeRTOS Event Groups Demo ---");

  /*
   * Create the event group.
   *
   * All event bits are initially cleared.
   */
  systemEventGroup = xEventGroupCreate();

  if (systemEventGroup == NULL) {
    Serial.println("Failed to create event group.");

    // Stop setup because tasks cannot use a NULL event group.
    return;
  }

  xTaskCreate(WiFiTask, "WiFi Task", TASK_STACK_SIZE, NULL, 1, NULL);

  xTaskCreate(SensorTask, "Sensor Task", TASK_STACK_SIZE, NULL, 1, NULL);

  xTaskCreate(SystemTask, "System Task", TASK_STACK_SIZE, NULL, 1, NULL);

  // Delete the Arduino setup task.
  vTaskDelete(NULL);
}

void loop() {
  // Not used.
}