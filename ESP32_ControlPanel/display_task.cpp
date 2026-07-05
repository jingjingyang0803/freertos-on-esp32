#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <Arduino.h>
#include <SPI.h>

#include "app_config.h"
#include "display_task.h"
#include "logger_task.h"
#include "ui_pages.h"

struct DisplayCommand {
  uint8_t page;
};

static QueueHandle_t display_queue = NULL;

static Adafruit_ST7789 tft =
    Adafruit_ST7789(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST);

static void display_task(void *pvParameters) {
  DisplayCommand cmd;

  tft.init(240, 240); // Initialize ST7789 with 240x240 resolution
  tft.setRotation(1); // Set rotation to landscape
  tft.fillScreen(ST77XX_BLACK);

  logger_log("TFT initialized");

  while (1) {
    if (xQueueReceive(display_queue, &cmd, portMAX_DELAY) == pdTRUE) {
      ui_draw_page(tft, (DisplayPage)cmd.page);
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