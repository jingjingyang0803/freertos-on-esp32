#pragma once

#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

QueueHandle_t button_get_queue();

void button_task_start();