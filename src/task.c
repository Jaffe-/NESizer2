#include <avr/io.h>
#include "task.h"
#include "timing.h"
#include "apu.h"

Task tasks[16];
uint8_t num_tasks = 0;

void task_manager()
/* 
   Schedules tasks, kind of like cooperative multitasking. 
   Tasks are assumed to not take too long to run. Tasks are prioritized by
   the order in which they are added to the task list.
 */

{
    uint16_t last_tick = 0;

    while (1) {
	while (ticks == last_tick);

	// Update task counters
	for (uint8_t i = 0; i < num_tasks; i++) 
	    tasks[i].counter += ticks - last_tick;

	last_tick = ticks;
    
	for (uint8_t i = 0; i < num_tasks; i++) {
	    if (ticks != last_tick) 
		break;

	    Task* task = &tasks[i];
	    //task->counter += diff; 
	    if (task->counter >= task->period) {
		task->counter = 0;
		if (task->enable) {
		    task->handler();
		}
	    }
	}

    }
}

void task_add(void (*handler)(), uint16_t period)
{
    tasks[num_tasks].handler = handler;
    tasks[num_tasks].period = period;
    tasks[num_tasks].counter = 0;
    tasks[num_tasks].enable = 1;
    num_tasks++;
}
