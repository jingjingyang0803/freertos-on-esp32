#include "ui_pages.h"

#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <Arduino.h>

#include "app_types.h"
#include "logger_task.h"

#define COLOR_BG ST77XX_BLACK
#define COLOR_TEXT ST77XX_WHITE
#define COLOR_TITLE ST77XX_CYAN
#define COLOR_LINE ST77XX_BLUE
#define COLOR_SELECTED ST77XX_YELLOW
#define COLOR_HINT ST77XX_GREEN

#define UI_TEXT_SMALL 1
#define UI_TEXT_NORMAL 2
#define UI_TEXT_TITLE 2
#define UI_TEXT_LARGE 3

#define SCREEN_W 240
#define SCREEN_H 240
#define HEADER_H 45

static const PageDef PAGE_DEFS[] = {
    {PAGE_HOME, "ESP32 Playground", {"Hardware", "Apps", "RTOS"}, 3},
    {PAGE_HARDWARE, "Hardware", {"Button", "Sensors", "Display"}, 3},
    {PAGE_APPS, "Apps", {"Stopwatch", "Dashboard", "Game"}, 3},
    {PAGE_RTOS, "RTOS", {"Queue Demo", "Mutex Demo", "Task Demo"}, 3},
};

static void draw_header(Adafruit_ST7789 &tft, const char *title) {
  tft.fillRect(0, 0, SCREEN_W, HEADER_H, COLOR_BG);

  tft.setTextColor(COLOR_TITLE);
  tft.setTextSize(UI_TEXT_TITLE);
  tft.setCursor(10, 10);
  tft.println(title);

  tft.drawLine(10, 35, 230, 35, COLOR_LINE);
}

static void clear_content(Adafruit_ST7789 &tft) {
  tft.fillRect(0, HEADER_H, SCREEN_W, SCREEN_H - HEADER_H, COLOR_BG);
}

static const PageDef *get_page_def(DisplayPage page) {
  for (uint8_t i = 0; i < sizeof(PAGE_DEFS) / sizeof(PAGE_DEFS[0]); i++) {
    if (PAGE_DEFS[i].page == page) {
      return &PAGE_DEFS[i];
    }
  }
  return &PAGE_DEFS[0];
}

uint8_t ui_page_get_item_count(DisplayPage page) {
  return get_page_def(page)->item_count;
}

void ui_draw_page(Adafruit_ST7789 &tft, DisplayPage page,
                  uint8_t selected_index) {

  const PageDef *def = get_page_def(page);

  draw_header(tft, def->title);
  clear_content(tft);

  tft.setTextSize(UI_TEXT_NORMAL);

  for (uint8_t i = 0; i < def->item_count; i++) {
    tft.setCursor(10, 55 + i * 25);

    if (i == selected_index) {
      tft.setTextColor(COLOR_SELECTED);
      tft.print("> ");
    } else {
      tft.setTextColor(COLOR_TEXT);
      tft.print("  ");
    }

    tft.println(def->items[i]);
  }
}