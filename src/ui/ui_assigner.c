/*
  Copyright 2014-2015 Johan Fjeldtvedt 

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



  Assigner user interface

  Handles the user interface when in the assigner mode.
*/


#include <stdint.h>
#include <avr/pgmspace.h>
#include "ui_assigner.h"
#include "ui.h"
#include "assigner/assigner.h"

#define BTN_ARP_ON 0
#define BTN_ARP_CHN 1
#define BTN_ARP_RATE 2

#define BTN_MONO 4
#define BTN_POLY1 5
#define BTN_POLY2 6
#define BTN_CHORD 7

#define BTN_ARP_UP 8
#define BTN_ARP_DOWN 9
#define BTN_ARP_UPDOWN 10
#define BTN_ARP_RANDOM 11
#define BTN_ARP_RANGE1 12
#define BTN_ARP_RANGE2 13
#define BTN_ARP_RANGE3 14
#define BTN_ARP_RANGE4 15

uint8_t assigner_leds[6];

void assigner(void)
{
}
