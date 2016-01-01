## Software

#### Databus communication

`bus.h` contains utility macros for using the bus system. The target component is adressed using `bus_select(<address>)`. A value is put on the bus using `bus_write(<value>)`. Changing bus direction to input or output is done by `bus_dir_input()` and `bus_dir_output()`, respectively. Values are read from the bus using `bus_read()`. `bus_deselect()` will disable the currently selected component.

`bus.h` also defines names for the adresses:

* 2A03: `CPU_ADDRESS`
* LED column: `LEDCOL_ADDRESS`
* Row (for both LEDs and switches): `ROW_ADDRESS`
* Switch column: `SWITCHCOL_ADDRESS`
* SRAM low address: `MEMORY_LOW_ADDRESS`
* SRAM mid address: `MEMORY_MID_ADDRESS`
* SRAM high address: `MEMORY_HIGH_ADDRESS`

The bus system must be initialized with `bus_setup()` before first time use.


#### Communication with the 2A03

The 2A03 is kept idle by continuously holding 0x85, the opcode for `STA` (with zero page addressing), on the databus. The CPU thus continuously executes an `STA $85` instruction. 

The reason for using this opcode instead of the more obvious `NOP` is to be able to synchronize with the CPU when a new instruction is to be sent. This synchronization is done by monitoring the **R/W** line of the 2A03. When it goes low, the 6502 is writing to memory. This *only* happens when the `STA $85` instruction is in its third cycle, which means that the next cycle will be the fetch of the next opcode. The Atmega then has a very narrow window to prepare and write the values to the 6502. Because of the precision needed, this is done by cycle exact assembly code in `2a03_io.s`.

Writing to APU registers is done by calling the function `io_register_write`. It selects the 2A03 address, blocks any interrupts and calls the assembly function `register_setN`, where N is one of three possible 2A03 clock divisors (12, 15 or 16, more about this below). This function makes the 6502 perform the following series of instructions:

    LDA #VALUE		0xA9 VALUE
    STA $40RR		0x8D 0xRR 0x40

where 0xRR is the low byte of the register address to be written to. At the end, `register_setN` puts the `STA` zero page opcode (0x85) back on the bus to keep the CPU busy until the next time something needs to be written.

There are also functions `reset_pcN` and `disable_interruptsN` in `2a03_io.s` for resetting the `PC` register and disabling interrupts on the 6502, respectively. `disable_interruptsN` executes an `SEI` instruction on the 6502 and is done once on boot. `reset_pcN` executes a `JMP $8585` instruction. It is called periodically to make sure that the 6502's program counter (`PC` register) doesn't overflow and run into the addresses where the APU registers are mapped (in which case APU register contents will be interpreted as instructions).

##### Different 2A03 varieties

The NESIZER supports three different kinds of chips: 2A03, 2A07 (PAL version) and Dendy clones. The most critical difference is the internal clock divider used: The 2A03 divides its clock input by 12, while the 2A07 divides by 16 and the Dendy clones by 15. An assembly function `detect` is used to determine which type of chip is being used. It puts an `STA` instruction with absolute addressing on the bus and uses one of the Atmega's timers to count how long two such instructions take to execute. The timer is being clocked by the Atmega's main clock, so its value will be proportional to how many Atmega cycles each 6502 cycle takes. Since two `STA` instructions with absolute addressing take 8 6502 cycles to complete, dividing the timer's value by 8 yields how many Atmega cycles there are in one 6502 cycle.

When the NESIZER boots, `detect` is run, and the result is used to make the function pointers `register_set`, `reset_pc` and `disable_interrupts` point to the correct functions in `2a03_io.s`. It is also used to select which table of timer values to use in `periods.c`.


#### Calculating period values for APU channels

This is detailed in [periods.pdf](periods.pdf).


#### APU abstaction layer

The APU abstraction layer (`apu`) contains structs and functions for manipulating the 2A03 channels in a high level manner without having to deal with register writes manually. The channels are represented by structs having fields corresponding to each (used/interesting) parameter of the channel. 

Each channel type is represented by a struct, `square`, `triangle`, `noise` and `dmc`, respectively. Global objects `sq1`, `sq2`, `tri`, `noise` and `dmc` of corresponding types are defined. Each channel has a setup function named `<channel>_setup`, intended for initializing the struct, and an update function `<channel>_update` which takes the data in a struct and fills the appropriate registers in a register buffer. The function `apu_refresh_channel` takes the data for a given channel in the register buffer and writes them to the 2A03. This function updates one channel's registers at a time, so it needs to be called 5 times to update all registers. This is necessary to reduce time spent on register updates.

The abstraction makes producing sound easy: 

	int main() 
	{
		// Initialize 2A03:
		io_setup();
		
		// Initialize Square 1 channel:
		sq1_setup();
		
		// Set the values for period, duty cycle and volume:
		sq1.enabled = 1;
		sq1.period = 400;
		sq1.duty = 2;
		sq1.volume = 15;
		
		// Update APU register buffer with new square 1 values:
		sq1_update();
		
		// Transfer changes to the APU:
		apu_refresh_channel(CHN_SQ1);
		
		// Wait indefinitely
		while(1);
	}


#### Interrupt timing and task handler

One of the Atmega's timers is used to generate an interrupt at approximately 16 kHz. This interrupt provides the basic timing used by various subsystems (LFOs, envelopes, APU updates, etc.).

A simple task handler (`task.h`, `task.c`) is used to sequence tasks to be performed. Tasks are registered with a desired frequency and a time delay to spread tasks out in time. 


#### LEDs and switches

These are handled in `leds.c`, `leds.h` and `input.c`, `input.h`. 

LED states are held in a 5 byte array `leds`. The function `leds_refresh` is intended to be registered as a task, and will update one column of the LED array each time it is called. It is intended to be run often enough for the sequential updating to happen unnoticed. 

Switch states are held in a 3 byte array `input`. The function `input_refresh` reads one row of switch data at a time and updates `input` accordingly. It is intended to be registered as a task and executed often enough for input to be seamless. 


#### SRAM

Functions for using the SRAM are available in `memory.c`, `memory.h`. Even though addresses are represented by 19 bits and two bits for chip selection at the hardware level, these details are glossed over by the software memory interface; an address is 20 bits, giving a total address space of 0 - 0xFFFFF. The internal function `set_address` translates a 20-bit address to the corresponding 19 bits and chip select bits.

The functions `memory_write()` and `memory_read()` are used to read and write bytes, while corresponding functions `read_word`, `read_dword`, `write_word` and `write_dword` are used to read or write 16- and 32-bit values. 

##### Sequential access

In order to reduce the time spent on memory operations, especially when playing back samples, a memory address can be set once using `memory_set_address()` and then values can be read or written sequentially using `memory_read_sequential()` and `memory_write_sequential()`, with as few address updates as possible (most often only the 8 lowest bits need to be changed). To make this work with several tasks using the memory, each task doing sequential memory access has to keep their own *memory context*, an object of type `struct memory_context`, whici holds the three (low, middle, high) memory latch bytes. `memory.c` keeps track of the last used memory context, so that when `memory_read_sequential` or `memory_write_sequential` is called, it makes sure that the task continues its memory access where it left off even if some other task changed the address in between. 


#### MIDI

Low level MIDI communication is implemented in `midi_io.c`, `midi_.h`. The Atmega's USART takes care of receiving MIDI data. In the function `midi_io_setup`, the USART is configured to use 1 start bit, 8 data bits and 1 stop bit, and to use a baud rate of 31250, which is the MIDI standard baud rate. 

The function `midi_io_handler` is intended to be registered as a task handler. It checks the state of the Atmega's USART receive buffer and reads new data into a ring buffer. 

Reading and interpreting the data is done by the functions in `midi.c`, `midi.h`. 