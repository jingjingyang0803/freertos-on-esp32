#pragma once
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

typedef enum {
  ENCODER_EVENT_CW,
  ENCODER_EVENT_CCW,
} encoder_event_type_t;

typedef struct {
  encoder_event_type_t type;
} encoder_event_t;

QueueHandle_t encoder_get_queue();

void encoder_task_start();