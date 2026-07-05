#pragma once

// Serial configuration
#define SERIAL_BAUD_RATE 115200

// Logger task configuration
#define LOGGER_TASK_STACK_SIZE 4096
#define LOGGER_TASK_PRIORITY 1
#define LOGGER_QUEUE_LENGTH 16

// Core configuration
#define CORE_UI 1

// Button task configuration
#define BUTTON_PIN 0

#define BUTTON_TASK_STACK_SIZE 4096
#define BUTTON_TASK_PRIORITY 2
#define BUTTON_SCAN_MS 20

#define BUTTON_QUEUE_LENGTH 8

#define BUTTON_DEBOUNCE_MS 50
#define BUTTON_LONG_PRESS_MS 800
#define BUTTON_DOUBLE_CLICK_MS 300

// App task configuration
#define APP_TASK_STACK_SIZE 4096
#define APP_TASK_PRIORITY 2

// Display task configuration
#define DISPLAY_TASK_STACK_SIZE 4096
#define DISPLAY_TASK_PRIORITY 1
#define DISPLAY_QUEUE_LENGTH 8

// TFT screen configuration
#define TFT_CS 46
#define TFT_DC 9
#define TFT_RST 10
#define TFT_MOSI 11
#define TFT_SCLK 12

// Encoder configuration
#define ENCODER_PIN_A 35
#define ENCODER_PIN_B 36