#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <Arduino.h>
#include <SPI.h>

#include "app_config.h"
#include "display_task.h"
#include "logger_task.h"

struct DisplayCommand {
  uint8_t page;
};

static QueueHandle_t display_queue = NULL;

static Adafruit_ST7789 tft =
    Adafruit_ST7789(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST);

static void draw_page(uint8_t page) {
  tft.fillScreen(ST77XX_BLACK);

  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(2);
  tft.setCursor(10, 10);
  tft.println("ESP32 Panel");

  tft.setTextSize(1);
  tft.setCursor(10, 45);

  if (page == 0) {
    tft.println("Page 0: Home");
    tft.setCursor(10, 65);
    tft.println("Status: OK");
  } else if (page == 1) {
    tft.println("Page 1: Sensors");
    tft.setCursor(10, 65);
    tft.println("Temp: --.- C");
  } else {
    tft.println("Unknown page");
  }
}

static void display_task(void *pvParameters) {
  DisplayCommand cmd;

  tft.init(240, 240); // Initialize ST7789 with 240x240 resolution
  tft.setRotation(1); // Set rotation to landscape
  tft.fillScreen(ST77XX_BLACK);

  logger_log("TFT initialized");

  while (1) {
    if (xQueueReceive(display_queue, &cmd, portMAX_DELAY) == pdTRUE) {
      draw_page(cmd.page);
    }
  }
}

void display_task_start() {
  display_queue = xQueueCreate(DISPLAY_QUEUE_LENGTH, sizeof(DisplayCommand));

  if (display_queue == NULL) {
    logger_log("ERROR: Failed to create display queue");
    return;
  }

  xTaskCreatePinnedToCore(display_task, "DisplayTask", DISPLAY_TASK_STACK_SIZE,
                          NULL, DISPLAY_TASK_PRIORITY, NULL, CORE_UI);

  logger_log("DisplayTask started");

  display_show_page(0);
}

void display_show_page(uint8_t page) {
  if (display_queue == NULL) {
    return;
  }

  DisplayCommand cmd;
  cmd.page = page;

  xQueueSend(display_queue, &cmd, pdMS_TO_TICKS(10));
}