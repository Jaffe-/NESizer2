/* NESIZER
   Bus interface

   (c) Johan Fjeldtvedt

   Handles low level bus communication. The bus consists of three address lines 
   and eight data lines.  
*/

#include <avr/io.h>
#include "bus.h"

void bus_setup()
{
    // Set address related port pins as outputs:
    DDRB |= ADDR_m;
    DDRD |= BUS_EN_m;

    // The bus_dir_output macro will take care of setting the databus pins
    // correctly:
    bus_dir_output();
}
  
