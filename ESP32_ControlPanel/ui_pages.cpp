#include "ui_pages.h"

#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <Arduino.h>

#include "app_types.h"

#define COLOR_BG       ST77XX_BLACK
#define COLOR_TEXT     ST77XX_WHITE
#define COLOR_TITLE    ST77XX_CYAN
#define COLOR_LINE     ST77XX_BLUE
#define COLOR_SELECTED ST77XX_YELLOW
#define COLOR_HINT     ST77XX_GREEN

#define UI_TEXT_SMALL   1
#define UI_TEXT_NORMAL  2
#define UI_TEXT_TITLE   2
#define UI_TEXT_LARGE   3

#define SCREEN_W 240
#define SCREEN_H 240
#define HEADER_H 45

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

static void draw_home(Adafruit_ST7789 &tft) {
  draw_header(tft, "ESP32 Playground");
  clear_content(tft);

  tft.setTextSize(UI_TEXT_NORMAL);

  tft.setTextColor(COLOR_TEXT);
  tft.setCursor(10, 55);
  tft.println("Embedded Lab");

  tft.setTextColor(COLOR_HINT);
  tft.setCursor(10, 90);
  tft.println("Short: Next");

  tft.setCursor(10, 115);
  tft.println("Long: Home");

  tft.setCursor(10, 140);
  tft.println("Double: Select");
}

static void draw_hardware(Adafruit_ST7789 &tft) {
  draw_header(tft, "Hardware");
  clear_content(tft);

  tft.setTextSize(UI_TEXT_NORMAL);

  tft.setTextColor(COLOR_SELECTED);
  tft.setCursor(10, 55);
  tft.println("> Button");

  tft.setTextColor(COLOR_TEXT);
  tft.setCursor(10, 80);
  tft.println("  Sensors");

  tft.setCursor(10, 105);
  tft.println("  Display");
}

static void draw_apps(Adafruit_ST7789 &tft) {
  draw_header(tft, "Apps");
  clear_content(tft);

  tft.setTextSize(UI_TEXT_NORMAL);

  tft.setTextColor(COLOR_SELECTED);
  tft.setCursor(10, 55);
  tft.println("> Stopwatch");

  tft.setTextColor(COLOR_TEXT);
  tft.setCursor(10, 80);
  tft.println("  Dashboard");

  tft.setCursor(10, 105);
  tft.println("  Game");
}

static void draw_rtos(Adafruit_ST7789 &tft) {
  draw_header(tft, "RTOS");
  clear_content(tft);

  tft.setTextSize(UI_TEXT_NORMAL);

  tft.setTextColor(COLOR_SELECTED);
  tft.setCursor(10, 55);
  tft.println("> Queue Demo");

  tft.setTextColor(COLOR_TEXT);
  tft.setCursor(10, 80);
  tft.println("  Mutex Demo");

  tft.setCursor(10, 105);
  tft.println("  Task Demo");
}

void ui_draw_page(Adafruit_ST7789 &tft, DisplayPage page) {
  switch (page) {
    case PAGE_HOME:
      draw_home(tft);
      break;

    case PAGE_HARDWARE:
      draw_hardware(tft);
      break;

    case PAGE_APPS:
      draw_apps(tft);
      break;

    case PAGE_RTOS:
      draw_rtos(tft);
      break;

    default:
      draw_home(tft);
      break;
  }
}