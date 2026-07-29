#include <Arduino.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/**
 * FreeRTOS Task Notification Array Demo
 *
 * Learning goals:
 * 1. Send different sensor values to different notification indexes.
 * 2. Receive values with xTaskNotifyWaitIndexed().
 * 3. Process different physical quantities meaningfully.
 * 4. Combine multiple sensor indicators to estimate room usage.
 *
 * Notification array:
 * - Index 0: PIR(Passive Infrared Sensor) movement state
 * - Index 1: sound level in dB
 * - Index 2: CO2 concentration in ppm
 *
 * Decision rule:
 * - Movement detected:       1 point
 * - Sound >= 45 dB:          1 point
 * - CO2 >= 800 ppm:          1 point
 *
 * A score of 2 or more means the room is likely being used.
 *
 * Important:
 * - Each sensor is evaluated using its own threshold.
 * - This is a simple learning heuristic, not a reliable occupancy system.
 */

static const int TASK_STACK_SIZE = 3072;

static const UBaseType_t MOVEMENT_INDEX = 0;
static const UBaseType_t SOUND_INDEX = 1;
static const UBaseType_t CO2_INDEX = 2;

/*
 * Check the configured task notification array size.
 *
 * If the configured number is smaller than 3, the compiler therefore stops here
 * and shows the error message below.
 */
#if configTASK_NOTIFICATION_ARRAY_ENTRIES < 3
#error "Set configTASK_NOTIFICATION_ARRAY_ENTRIES to at least 3"
#endif

static const uint32_t SOUND_THRESHOLD_DB = 45;
static const uint32_t CO2_THRESHOLD_PPM = 800;

TaskHandle_t roomMonitorTaskHandle = NULL;

void RoomMonitorTask(void *parameter) {
  while (1) {
    uint32_t movementDetected = 0;
    uint32_t soundLevelDb = 0;
    uint32_t co2LevelPpm = 0;

    Serial.println();
    Serial.println("RoomMonitorTask: Waiting for sensor data...");

    /*
     * Wait for the PIR movement state on index 0.
     *
     * 0 means no movement.
     * 1 means movement detected.
     */
    xTaskNotifyWaitIndexed(MOVEMENT_INDEX, 0, UINT32_MAX, &movementDetected,
                           portMAX_DELAY);

    /*
     * Wait for the sound level on index 1.
     */
    xTaskNotifyWaitIndexed(SOUND_INDEX, 0, UINT32_MAX, &soundLevelDb,
                           portMAX_DELAY);

    /*
     * Wait for the CO2 concentration on index 2.
     */
    xTaskNotifyWaitIndexed(CO2_INDEX, 0, UINT32_MAX, &co2LevelPpm,
                           portMAX_DELAY);

    uint32_t occupancyScore = 0;

    bool movementActive = movementDetected == 1;
    bool soundActive = soundLevelDb >= SOUND_THRESHOLD_DB;
    bool co2Active = co2LevelPpm >= CO2_THRESHOLD_PPM;

    /*
     * Evaluate each physical quantity using its own rule.
     */
    if (movementActive) {
      occupancyScore++;
    }

    if (soundActive) {
      occupancyScore++;
    }

    if (co2Active) {
      occupancyScore++;
    }

    Serial.println();
    Serial.println("--- Room Sensor Data ---");

    Serial.print("Movement: ");
    Serial.println(movementActive ? "DETECTED" : "NOT DETECTED");

    Serial.print("Sound level: ");
    Serial.print(soundLevelDb);
    Serial.println(" dB");

    Serial.print("CO2 level: ");
    Serial.print(co2LevelPpm);
    Serial.println(" ppm");

    Serial.print("Occupancy score: ");
    Serial.print(occupancyScore);
    Serial.println(" / 3");

    if (occupancyScore >= 2) {
      Serial.println("Result: Room is likely being used.");
    } else {
      Serial.println("Result: Room is likely empty.");
    }

    Serial.println("------------------------");
  }
}

void SensorTask(void *parameter) {
  /*
   * Simulated room conditions:
   *
   * movement, sound dB, CO2 ppm
   */
  static const uint32_t roomData[][3] = {
      {0, 30, 500},  // Empty room
      {1, 38, 600},  // One person briefly entered
      {1, 55, 850},  // Class starting
      {1, 68, 1100}, // Active class
      {0, 35, 950},  // Class ended, CO2 still high
      {0, 28, 650}   // Room becoming empty again
  };

  static const int DATA_SET_COUNT = sizeof(roomData) / sizeof(roomData[0]);

  int dataIndex = 0;

  while (1) {
    Serial.println();
    Serial.println("SensorTask: Collecting room sensor data...");

    vTaskDelay(pdMS_TO_TICKS(2000));

    uint32_t movement = roomData[dataIndex][0];
    uint32_t soundDb = roomData[dataIndex][1];
    uint32_t co2Ppm = roomData[dataIndex][2];

    /*
     * Store each sensor value in its own notification entry.
     */
    xTaskNotifyIndexed(roomMonitorTaskHandle, MOVEMENT_INDEX, movement,
                       eSetValueWithOverwrite);

    xTaskNotifyIndexed(roomMonitorTaskHandle, SOUND_INDEX, soundDb,
                       eSetValueWithOverwrite);

    xTaskNotifyIndexed(roomMonitorTaskHandle, CO2_INDEX, co2Ppm,
                       eSetValueWithOverwrite);

    Serial.println("SensorTask: Sensor data sent.");

    dataIndex++;

    if (dataIndex >= DATA_SET_COUNT) {
      dataIndex = 0;
    }

    vTaskDelay(pdMS_TO_TICKS(4000));
  }
}

void setup() {
  Serial.begin(115200);
  vTaskDelay(pdMS_TO_TICKS(1000));

  Serial.print("Task notification array entries = ");
  Serial.println(configTASK_NOTIFICATION_ARRAY_ENTRIES);

  Serial.println();
  Serial.println("--- Room Occupancy Notification Array Demo ---");

  BaseType_t monitorResult =
      xTaskCreate(RoomMonitorTask, "Room Monitor Task", TASK_STACK_SIZE, NULL,
                  1, &roomMonitorTaskHandle);

  if (monitorResult != pdPASS) {
    Serial.println("Failed to create RoomMonitorTask.");
    return;
  }

  BaseType_t sensorResult =
      xTaskCreate(SensorTask, "Sensor Task", TASK_STACK_SIZE, NULL, 1, NULL);

  if (sensorResult != pdPASS) {
    Serial.println("Failed to create SensorTask.");
    return;
  }

  vTaskDelete(NULL);
}

void loop() {
  // Not used.
}