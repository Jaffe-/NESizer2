
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
                f = f_C1 * 2 ** (octave + note/12)
                T = round((20e6 / divisor) / (16 * f) - 1)
                fileobj.write(str(T))
                if not (octave == 6 and note == 11):
                    fileobj.write(", ")
            fileobj.write("\n")                        
        fileobj.write("};\n\n")                        
    fileobj.close()
    
