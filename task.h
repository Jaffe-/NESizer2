#pragma once

typedef struct {
    uint16_t period;
    uint16_t counter;
    void (*handler)();
    uint8_t enable : 1;
} Task;

Task tasks[16];
uint8_t num_tasks;

void task_add(void (*)(), uint16_t, uint8_t);
void task_manager();
