# Target name:
TARGET = nesizer_fw

# Target type
MCU=atmega328p
PROGRAMMER=jtag3isp
F_CPU=20000000L

MODULES = ./ $(shell ls -d */)
CSRC = $(foreach m, $(MODULES), $(wildcard $(m)*.c))
HEADERS = $(foreach m, $(MODULES), $(wildcard $(m)*.h))
OBJ = $(CSRC:.c=.o) 2a03_s.o

# Compiler / Linker flags:
CFLAGS = -mmcu=$(MCU) -Wall -O2 -std=gnu11 -funsigned-char -funsigned-bitfields -fpack-struct -fshort-enums -ffunction-sections -fdata-sections -I$(shell pwd) -DTARGET
LDFLAGS = -mmcu=$(MCU) -Wl,-Map=$(TARGET).map -Wl,--gc-sections

###################################

.PHONY: compile flash clean

compile: $(TARGET).hex

flash: compile
	avrdude -c $(PROGRAMMER) -P usb -p $(MCU) -B 1 -U flash:w:$(TARGET).hex

clean:
	rm -f $(OBJ) $(TARGET).{hex,map}

.SECONDARY: $(OBJS)

###################################

%.hex: %.elf
	avr-objcopy -O ihex -R .eeprom -R .fuse -R .lock -R .signature $< $@

%.elf: $(OBJ)
	avr-gcc $^ $(LDFLAGS) -o $@

%.o : %.c
	avr-gcc $(CFLAGS) -c $< -o $@

-include $(CSRC:%.c=%.d)

2a03_s.o : io/2a03.s
	avr-as -mmcu=$(MCU) -c $< -o $@
