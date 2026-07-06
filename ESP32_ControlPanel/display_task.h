#pragma once

#include "app_types.h"

void display_task_start();
void display_show_page(uint8_t page, uint8_t selected_index = 0);
DisplayPage display_show_next_page(DisplayPage page);