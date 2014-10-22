#pragma once

typedef struct {
    uint16_t period;
    uint16_t counter;
    void (*handler)();
    uint8_t enable : 1;
} Task;

void task_add(void (*)(), uint16_t);
void task_manager();
