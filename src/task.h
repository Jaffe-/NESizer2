#pragma once

typedef struct {
  void (*const handler)();
  const uint8_t period;
  uint8_t counter;
} Task;

void task_add(void (*handler)(), uint8_t period, uint8_t offset);
void task_manager();
void task_setup();
