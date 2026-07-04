#pragma once

// Serial configuration
#define SERIAL_BAUD_RATE 115200

// Logger task configuration
#define LOGGER_TASK_STACK_SIZE 4096
#define LOGGER_TASK_PRIORITY 1
#define LOGGER_QUEUE_LENGTH 16

// UI task configuration
#define CORE_UI 1

// Button task configuration
#define BUTTON_PIN 0

#define BUTTON_TASK_STACK_SIZE 4096
#define BUTTON_TASK_PRIORITY 2
#define BUTTON_SCAN_MS 20

#define BUTTON_QUEUE_LENGTH 8

#define BUTTON_DEBOUNCE_MS 50

// App task configuration
#define APP_TASK_STACK_SIZE 4096
#define APP_TASK_PRIORITY 2