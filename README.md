![alt text](https://raw.githubusercontent.com/Jaffe-/NESizer2/master/nesizer.svg "NESIZER")
(Logo by Vegard Sjonfjell)

## NESizer2: 2A03 Synthesizer Project

The main idea of this project is to use an isolated NES 2A03 CPU/APU IC as a stand-alone synthesizer, controlled by an Atmega168 microcontroller. (The project is named NESizer2 after a failed attempt to create a NES synthesizer using only the 2A03 alone, with its own ROM, RAM and I/O logic.)

The 2A03 IC consists of a 6502 core (with some minor differences), a DMA controller and the Audio Processing Unit. The various aspects of the APU are controlled via 22 registers, which are connected internally to the 6502 only. There are no external input pins facilitating communication with the APU. This means that in order to control the APU, the 6502 must act as a proxy. The microcontroller must send the 6502 instructions to take a value and put it in a desired register. 

This project is inspired by a similar approach taken here: http://www.soniktech.com/tsundere/, but the idea here is to have the microcontroller communicate more directly with the 2A03, instead of using dedicated logic circuitry to send instructions.

Here is a demo: https://www.youtube.com/watch?v=0pwFglPS3n8
A second demo: https://www.youtube.com/watch?v=6Rg04oqwLCA


### Changelog

**29/08/14**: The NESIZER now has a logo! Done some fixes on the SRAM battery backup circuitry, I'm still battling some data corruption during power-off and power-on. Sample playback has been moved from flash to SRAM. 

**25/08/14**: Changed a lot of things in the user interface: Entering pattern notes now also includes a length setting, and octave setting. The upper 16 buttons are converted to "keyboard keys" when waiting for a note. The note is played on the corresponding channel whenever a new setting is made. Patterns are now storable in memory as well. While a pattern is playing the user can switch to patch programming mode and change parameters live. 

**22/08/14**: Fixed clock circuit. It turned out to be inadequate for an older 2A03 IC I got off eBay. It now inverts the oscillator output twice, and there's also been added a 1M resistor in parallel with the crystal. 

**20/08/14**: Much of the front panel functionality has been implemented. Patch memory saving/loading as well. SRAM battery backup added. A CR2032 3V battery powers the SRAMs when main power is off.

**17/08/14**: Started on planning and implementing the intended user interface. A prototype panel PCB has been designed and sent for production. I have settled on trying to implement as much functionality as possible, without making the user interface too complicated. The final product will have the step sequencer showed in the videos above. 

**14/08/14**: SRAM memory added. I plan to use two 512kx8 SRAM ICs, but in theory the way I have implemented it allows for up to 8MB memory (using two 4MB SRAM ICs, if such a thing exists) 

**12/08/14**: Analog potentiometer input added. This is needed in the front panel for the user to select attack times, LFO speed, etc. 

**09/08/14**: PHI2 is no longer needed for communication between the Atmega168 and the 6502, as the register write routine is now written in assembly and timed by counting clock cycles instead of reading the state of PHI2.  

**08/07/14**: Started preparing for MIDI, databus now occupies lower 2 bits of PORTC and upper 6 bits of PORTD. This is to be able to use the RX and possibly TX pins on the Atmega168 for MIDI. 

**08/05/14**: Bus addresses are now 3 bits instead of 2, to be able to select more than 4 components. This is not needed right now, but I plan to add a battery backed SRAM to store sequences composed on the sequencer, as well as "patches", etc. 


### Hardware

The following images shows the most current schematics.

Main board:
![alt text](https://raw.githubusercontent.com/Jaffe-/NESizer2/master/mainboard.png "Main board")

Front panel board:
![alt text](https://raw.githubusercontent.com/Jaffe-/NESizer2/master/panelboard.png "Panel board")

The circuit essentially consists of the following parts:

- Atmega168 microcontroller
- 2A03 CPU/APU
- LED matrix
- Switch matrix
- Battery backed SRAM
- Analog signal amplifier


#### Communication

The Atmega168 accesses the 2A03, LED matrix, switch matrix and SRAM through a simple bus system consisting of a 3-bit "address bus" and an 8-bit data bus. Bits 0, 1 and 2 of **PORTB** are used as the address, while bits 0 and 1 of **PORTC** and bits 2 to 7 of **PORTD** are connected to the data bus. A 74HC238 decoder is used to decode the 3-bit address into one of eight activation signals for each component. The addresses are decoded as follows:

- 0: 2A03 data bus 
- 1: LED matrix column
- 2: matrix row for both LED and switch matrices
- 3: switch matrix column / default/idle address
- 4: memory, lower part of address
- 5: memory, middle part of address
- 6: memory high part of address
- 7: unused/"idle" address


#### 2A03 setup

Both the Atmega168 and the 2A03 are clocked by a 20 MHz crystal oscillator circuit based on 74HCT04 inverters. The 2A03 divides this clock by 12 internally to provide a 1.66 MHz clock for the 6502 and APU. This is a bit lower than the usual frequency for the 2A03 (1.79 MHz), but it has no serious impact on APU operation (timer values become a bit different).

The Atmega is hooked up to the 2A03 using the following connections:

- **PD2** .. **PD7**  --->  74HC573 latch ---> **D2** .. **D7** on 2A03 
- **PC0** .. **PC1**  --->  74HC573 latch ---> **D0** .. **D1** on 2A03
- **PC2**  --->  **RESET** pin on 2A03
- **PC3**  <---  **R/W** pin on 2A03

The latch is necessary to hold on to the databus value when the bus is being used for something else. 

The reset connection could possibly be omitted (and the 2A03 reset pin just connected to a standard reset circuit), but connecting it to the Atmega allows for the 2A03 to be reset at any time, which might be handy. 

The **R/W** output from the 2A03 is necessary to synchronize with the CPU.

In addition to the connections to the Atmega, some other connections are necessary for the 2A03 to function properly:

- **/NMI** and **/IRQ** are pulled high
- Pin 30 (diagonistics pin?) is pulled low
- **SND1** and **SND2** (APU outputs) are pulled low via 100 ohm resistors

The 100 ohm pull-down resistors on **SND1** and **SND2** are required for the DACs in the APU to function properly. (This also has the effect that the output signals are very weak.)

Apart from this and the usual power supply connections, there are no further connections made. The address bus and the gamepad inputs/outputs are simply left unconnected. 


#### LED and switch matrices

These are both pretty standard. The LED matrix is a row scan matrix with 8 columns and 5 rows. The switch matrix is also a row scan matrix with 8 columns and 3 rows. The lower five bits of a 74HC573 latch are used to hold the selected LED row, and the upper three bits are used to hold the selected switch row. This latch is accessed through the data bus by selecting address 2. 

The LED column is specified by writing to a 74HC573 latch connected to the data bus and selected with address 1. (Note that in order to light an LED at row *x* and column *y*, a 1 must be written at bit position *y* in the column latch, while a 0 must be written to bit position *x* of the row latch. This is because the column latch is sourcing the current while the row latch is sinking it.) 

Reading a switch state is done by activating the correct row in the row latch, and then selecting address 3 and reading from the bus. 


#### Battery backed SRAM memory

There are two SRAM ICs of 512KByte each, thus a 20 bit address is needed to address each individual byte (1MB in total). Since the databus is only 8 bit wide, three latches are used to specify an address before reading or writing to memory. The lowest 8 bits are written to the latch at databus address 4, the next 8 bits to the latch at address 5, and the next three bits to the latch at address 6. This totals to 19 bits, enough to address any byte in one of the ICs. The desired chip is selected by the upper two bits of the high address latch, where a value of 0b10 selects the first, 0b01 the other, and 0b11 neither. 

The write enable (/WE) line of the SRAM ICs is connected to pin 4 of **PORTC**. This pin must be kept high at all times except when a write is to performed. When an address is set up and a desired value is put on the databus, this pin is pulled low and thereafter pulled high to write the value to the address in memory. 

The SRAM ICs are powered by both the 5V supply (VCC) and also a 3V lithium battery. Diodes are placed in series between each voltage source and the supply inputs to prevent current from going from voltage source one to the other. 


#### Audio signal amplification

The output signals **SND1** and **SND2** are amplified up to line level and brought out on a stereo jack. There is also a mono mix made of the two with the same mixing ratio as in the NES, which is also amplified to line level. A somewhat shitty LM324 op-amp is used for all those purposes, in lack of a better one.


### Software

#### Databus communication

`bus.h` contains utility macros for using the bus system. The target component is adressed using `bus_set_address(<address>)`. A value is put on the bus using `bus_set_value(<value>)`. Changing bus direction to input or output is done by `bus_set_input()` and `bus_set_output()`, respectively. Values are read from the bus using `bus_read_value()`. 

`bus.h` also contains definitions of the adresses:

* 2A03: `CPU_ADDR`
* LED column: `LEDCOL_ADDR`
* Row (for both LEDs and switches): `ROW_ADDR`
* Switch column: `SWITCHCOL_ADDR`
* SRAM low address: `MEMORY_ADDRLOW_ADDR`
* SRAM mid address: `MEMORY_ADDRMID_ADDR`
* SRAM high address: `MEMORY_ADDRHIGH_ADDR`


#### Running the 2A03 and synchronizing

The 2A03 is kept idle by continuously holding 0x85, the opcode for `STA` (with zero page addressing), on the databus. The CPU thus continuously executes an `STA $85` instruction. 

The reason for using this opcode instead of the more obvious `NOP` is to be able to synchronize with the CPU when a new instruction is to be sent. This synchronization is done by monitoring the **R/W** line of the 2A03. When it goes low, the 6502 is writing to memory. This *only* happens when the `STA $85` instruction is in its third cycle, which means that the next cycle will be the fetch of the next opcode.

The synchronization is done in the inline function `sync` by waiting for **R/W** to go low, high and then low again. The 6502 is then guaranteed to be at the start of its third cycle, and preparations for the next cycle can start.


#### Writing to the APU registers

Writing to APU registers is done by the function `register_write` (exposed externally as `io_register_write`).

The actual write sequence is performed by the function `register_set`. It makes the 6502 perform the following series of instructions:

    LDA #VALUE		0xA9 VALUE
    STA $40RR		0x8D 0xRR 0x40

where 0xRR is the low byte of the register address to be written to. The Atmega must put these byte strings on the 6502 databus when the 6502 enters a new read cycle. Because of the tight timing requirements, and the computations necessary because the databus is divided across PORTC and PORTD, assembly is used to write this routine. See the source code for a more detailed explanation of it.

At the end, `register_set` puts the `STA` zero page opcode (0x85) back on the bus to keep the CPU busy until the next time something needs to be written.


#### APU abstaction layer

The APU abstraction layer (`apu.h`, `apu.c`) contains structs and functions for manipulating the 2A03 channels in a high level manner without having to deal with register writes manually. The channels are represented by structs having fields corresponding to each parameter of the channel. 

Each channel type is represented by a struct, `Square`, `Triangle`, `Noise` and `DMC`, respectively. Global objects `sq1`, `sq2`, `tri`, `noise` and `dmc` of corresponding types are allocated on the stack. Each channel has a setup function named `<channel>_setup`, intended for initializing the struct, and an update function `<channel>_update` which takes the data in a struct and fills the appropriate registers in a register buffer. The function `apu_refresh_channel` takes the data for a given channel in the register buffer and writes them to the 2A03. This function updates one channel's registers at a time, so it needs to be called 5 times to update all registers. This is necessary to reduce time spent on register updates.

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

One of the Atmega's timers is used to generate an interrupt at approximately 16 kHz. This interrupt provides the basic timing used by various subsystems (LFOs, envelopes, APU updates, etc.)  

A simple task handler (`task.h`, `task.c`) is used to sequence tasks to be performed. Tasks are registered with a desired frequency and a time delay to spread tasks out in time. 


#### LEDs and switches

These are handled in `leds.c`, `leds.h` and `input.c`, `input.h`. 

LED states are held in a 32 byte array `leds`. The function `leds_refresh` is intended to be registered as a task, and will update one column of the LED array each time it is called. It is intended to be run often enough for the sequential updating to happen unnoticed. 

Switch states are held in a 32 byte array `input`. The function `input_refresh` reads one row of switch data at a time and updates `input` accordingly. It is intended to be registered as a task and executed often enough for input to be seamless. 


#### SRAM

Functions for using the SRAM are available in `memory.c`, `memory.h`. Even though addresses are represented by 19 bits and two bits for chip selection at the hardware level, these details are glossed over by the software memory interface; an address is 20 bits, giving a total address space of 0 - 0xFFFFF. The internal function `set_address` translates a 20-bit address to the corresponding 19 bits and chip select bits.

The functions `memory_write(<address>, <value>)` and `memory_read(<address>)` are used to access the RAMs. 

