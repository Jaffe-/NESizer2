#
#  Copyright 2014-2015 Johan Fjeldtvedt 

#  This file is part of NESIZER.

#  NESIZER is free software: you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation, either version 3 of the License, or
#  (at your option) any later version.

#  NESIZER is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.

#  You should have received a copy of the GNU General Public License
#  along with NESIZER.  If not, see <http://www.gnu.org/licenses/>.



#  Period table generator
#
#  Generates period tables to be included as C code. 

import sys

if len(sys.argv) < 2:
    print("Arguments: filename")
else:
    f_C1 = 32.70
    filename = sys.argv[1]

    fileobj = open(filename, 'w')

    for divisor in [12, 15, 16]:
        fileobj.write("const uint16_t period_table" + str(divisor) + "[84] PROGMEM = {\n")

        note_min = 0
        
        for octave in range(0, 7):
            fileobj.write("  ")
            for note in range(0, 12):
                f = f_C1 * 2 ** (octave + note/12.0)
                T = int(round((20e6 / divisor) / (16 * f)))
                fileobj.write(str(T))
                if not (octave == 6 and note == 11):
                    fileobj.write(", ")
            fileobj.write("\n")                        
        fileobj.write("};\n\n")                        
    fileobj.close()
    
