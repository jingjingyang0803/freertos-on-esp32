/******************************************************************************
 * File: logger_task.cpp
 *
 * Owns the logging system.
 * Other tasks send log messages through a queue.
 * Only LoggerTask accesses the serial port directly.
 ******************************************************************************/

#include <Arduino.h>

#include "app_config.h"
#include "logger_task.h"

// A single log message that will be sent through the queue.
// The queue stores copies of this structure.
struct LogMessage {
  char text[128];
};

// Only this module is allowed to access the logger queue directly.
// Other modules should call logger_log().
static QueueHandle_t logger_queue = NULL;

//--------------------------------------------------------------
// Logger Task
//
// Waits for log messages and prints them to the serial port.
// This is the only task that should access Serial directly.
//--------------------------------------------------------------
static void logger_task(void *pvParameters) {
  LogMessage msg;

  while (1) {
    // Block here until a message arrives.
    // While blocked, this task uses no CPU time.
    if (xQueueReceive(logger_queue, &msg, portMAX_DELAY) == pdTRUE) {
      Serial.println(msg.text);
    }
  }
}

//--------------------------------------------------------------
// Create the logger queue and start the logger task.
//
// The queue must be created before the task starts.
// Otherwise the task could try to access a NULL queue.
//--------------------------------------------------------------
void logger_task_start() {
  logger_queue = xQueueCreate(LOGGER_QUEUE_LENGTH, sizeof(LogMessage));

  if (logger_queue == NULL) {
    Serial.println("ERROR: Failed to create logger queue");
    return;
  }

  xTaskCreatePinnedToCore(logger_task, "LoggerTask", LOGGER_TASK_STACK_SIZE,
                          NULL, LOGGER_TASK_PRIORITY, NULL, CORE_UI);
}

//--------------------------------------------------------------
// Public logging function.
//
// Other tasks call this function instead of using
// Serial.println() directly.
//
// The message is copied into the queue.
// LoggerTask will print it later.
//--------------------------------------------------------------
void logger_log(const char *message) {
  if (logger_queue == NULL) {
    return;
  }

  LogMessage msg;

  // Copy the string into the queue object.
  // The caller's buffer can safely disappear afterwards.
  snprintf(msg.text, sizeof(msg.text), "%s", message);

  // Wait up to 10 ms if the queue is full.
  // Logging is not critical, so we do not wait forever.
  xQueueSend(logger_queue, &msg, pdMS_TO_TICKS(10));
}