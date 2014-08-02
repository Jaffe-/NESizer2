#pragma once

#define ADDR_m 0b110000
#define ADDR_p 4

#define CPU_ADDR 0
#define LEDCOL_ADDR 1
#define ROW_ADDR 2
#define SWITCHCOL_ADDR 3

#define bus_set_address(ADDR) PORTC = (PORTC & ~ADDR_m) | (ADDR << ADDR_p)
#define bus_set_value(VAL) PORTD = VAL
#define bus_read_value() PIND
#define bus_set_input() PORTD = 0; DDRD = 0; PORTD = 0xFF
#define bus_set_output() PORTD = 0; DDRD = 0xFF

#define nop() asm volatile("nop")
