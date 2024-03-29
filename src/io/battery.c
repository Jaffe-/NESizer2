/*
  Copyright 2014-2016 Johan Fjeldtvedt

  This file is part of NESIZER.

  NESIZER is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  NESIZER is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with NESIZER.  If not, see <http://www.gnu.org/licenses/>.



  Battery voltage reading

  Configures ADC and reads battery voltage.
*/

#include <avr/io.h>

void battery_setup(void)
{
    /* Set up PORTC pin 5 as ADC input */
    DDRC &= ~(1 << 5);
    PORTC &= ~(1 << 5);
    DIDR0 = 1 << ADC5D;

    /* - Use AVCC (5V) as reference voltage,
       - Left adjust the result
       - Select ADC channel 5 as input */
    ADMUX = (1 << REFS0) | (1 << ADLAR) | (5 << MUX0);

    /* ADC clock is main clock divided by 128 */
    ADCSRA = 7;
}

uint8_t battery_read(void)
{
    /* Enable ADC, start new conversion */
    ADCSRA |= (1 << ADEN) | (1 << ADSC);

    /* Wait for conversion to complete */
    while (ADCSRA & (1 << ADSC));

    uint32_t val = ADCH;
    uint16_t voltage_rounded = (5 * 100 * val) / 256;

    /* Disable ADC */
    ADCSRA &= ~(1 << ADEN);

    return voltage_rounded / 10;
}
