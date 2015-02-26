#pragma once

typedef struct {
    uint8_t period;
    uint8_t counter;
    void (*handler)();
} Task;

void task_add(void (*handler)(), uint8_t period, uint8_t offset);
void task_manager();
void task_setup();
