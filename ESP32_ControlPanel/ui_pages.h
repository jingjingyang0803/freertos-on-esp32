#pragma once

#include "app_types.h"
#include <Adafruit_ST7789.h>

uint8_t ui_page_get_item_count(DisplayPage page);
void ui_draw_page(Adafruit_ST7789 &tft, DisplayPage page,
                  uint8_t selected_index);