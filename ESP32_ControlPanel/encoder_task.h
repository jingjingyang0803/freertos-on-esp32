#pragma once
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

QueueHandle_t encoder_get_queue();

void encoder_task_start();