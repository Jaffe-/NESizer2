## NESizer2: 2A03 Synthesizer Project

The main idea of this project is to use an isolated NES 2A03 CPU/APU IC as a stand-alone synthesizer, controlled by an Atmega168 microcontroller. (The project is named NESizer2 after a failed attempt to create a NES synthesizer using only the 2A03 alone, with its own ROM, RAM and I/O logic.)

The 2A03 IC consists of a 6502 core (with some minor differences), a DMA controller and the Audio Processing Unit. The various aspects of the APU are controlled via 22 registers, which are connected internally to the 6502 only. There are no external input pins facilitating communication with the APU. This means that in order to control the APU, the 6502 must act as a proxy. The microcontroller must send the 6502 instructions to take a value and put it in a desired register. 

This project is inspired by a similar approach taken here: http://www.soniktech.com/tsundere/, but the idea here is to have the microcontroller communicate more directly with the 2A03, instead of using dedicated logic circuitry to send instructions.


### Hardware

The following image shows the most current schematic:

![alt text](https://raw.githubusercontent.com/Jaffe-/NESizer2/master/hw.png "Hardware")


#### The communication interface

Both the Atmega168 and the 2A03 are clocked by a 20 MHz crystal oscillator circuit based on 74HCT04 inverters. The 2A03 divides this clock by 12 internally to provide a 1.66 MHz clock for the 6502 and APU. This is a bit lower than the usual frequency for the 2A03 (1.79 MHz), but it has no serious impact on APU operation (timer values become a bit different).

The Atmega is hooked up to the 2A03 using the following connections:

- **PORTD** (**PD0** .. **PD7**)  <--->  **D0** .. **D7** on 2A03
- **PC0**  --->  **RESET** pin on 2A03
- **PC1**  <---  **PHI2** clock output from 2A03
- **PC2**  <---  **R/W** pin on 2A03

The reset connection could possibly be omitted (and the 2A03 reset pin just connected to a standard reset circuit), but connecting it to the Atmega allows for the 2A03 to be reset at any time, which might be handy. 

The **PHI2** output (M2 in the schematic's 2A03 pinout) from the 2A03 is the clock signal of the 6502 which goes high when the 6502 reads or writes in memory. This simplifies synchronizing with the 6502 when sending it instructions. 

The **R/W** output from the 2A03 is necessary to synchronize with the CPU.

	     
#### 2A03 setup

In addition to the connections to the Atmega, some other connections are necessary for the 2A03 to function properly:

- **/NMI** and **/IRQ** are pulled high
- Pin 30 (diagonistics pin?) is pulled low
- **SND1** and **SND2** (APU outputs) are pulled low via 100 ohm resistors

The 100 ohm pull-down resistors on **SND1** and **SND2** are required for the DACs in the APU to function properly. (This also has the effect that the output signals are very weak.)

Apart from this and the usual power supply connections, there are no further connections made. The address bus and the gamepad inputs/outputs are simply left unconnected. 


#### Audio signal amplification

The output signals **SND1** and **SND2** are amplified up to line level and brought out on a stereo jack. There is also a mono mix made of the two with the same mixing ratio as in the NES, which is also amplified to line level. A somewhat shitty LM324 op-amp is used for all those purposes, in lack of a better one.


#### Status LEDs

The prototype board has 8 LEDs for debugging purposes. These are connected to a 74HC164 shift register which is connected to the SPI interface of the Atmega168. 


### Software


#### Running the 2A03 and synchronizing

The 2A03 is kept idle by continuously holding 0x85, the opcode for `STA` (with zero page addressing), on the databus. The CPU thus continuously executes an `STA $85` instruction. 

The reason for using this opcode instead of the more obvious `NOP` is to be able to synchronize with the CPU when a new instruction is to be sent. This synchronization is done by monitoring the **R/W** line of the 2A03. When it goes low, the 6502 is writing to memory. This *only* happens when the `STA $85` instruction is in its third cycle, which means that the next cycle will be the fetch of the next opcode.

The synchronization is done in the inline function `sync` by waiting for **R/W** to go low and **PHI2** to go high. When this is the case, the next time **PHI2** transitions from low to high the new instruction can be put on the bus. 

During actual transmission of instructions, the `databus_wait` inline function is used to synchronize with the 6502's read cycles by waiting for **PHI2** to go high.


#### Writing to the APU registers

Writing to APU registers is done by the functions `register_write` and `register_write_all` which write one or all of the registers, respectively. 

Actually writing to the APU registers is done by the function `register_set`. It makes the 6502 perform the following series of instructions:

    LDA #VALUE		0xA9 VALUE
    STA $40RR		0x8D 0xRR 0x40

where 0xRR is the low byte of the register address to be written to. The Atmega must put these byte strings on the 6502 databus when **PHI2** goes high, and keep them on there long enough for the 6502 to read. The `databus_wait` function is used to wait for **PHI2**'s transition. 

When a register write sequence (or the last of many in `register_write_all`'s case) is done, the function `reset_pc` is called. This sends a `JMP $4018` instruction (in the same manner as described) to reset the program counter back to $4018. This reset is necessary to keep the CPU from reading from addresses in the internal register range, which could lead to a bus conflict where the internal registers could win and the CPU fetches an unintentional opcode.

Lastly, the idle `STA` opcode (0x85) is put back on the bus to keep the CPU busy.


#### APU abstaction layer

The APU abstraction layer (`apu.h`, `apu.c`) contains structs and functions for manipulating the 2A03 channels in a high level manenr without having to deal with register writes manually. The channels are represented by structs having fields corresponding to each parameter of the channel. 

Each channel type is represented by a struct, `Square`, `Triangle`, `Noise` and `DMC`, respectively. Global objects `sq1`, `sq2`, `tri`, `noise` and `dmc` of corresponding types are allocated on the stack. Each channel has a setup function named `<channel>_setup`, intended for initializing the struct, and an update function `<channel>_update` which takes the data in a struct and fills the appropriate registers in a register buffer. The function `apu_refresh` takes the data in the register buffer and writes them to the 2A03. This function updates 6 registers at a time, so it needs to be called 3 times to update all registers. This is necessary to reduce time spent on register updates. 

The abstraction makes producing sound easy: 

	int main() 
	{
		// Initialize 2A03:
		2a03_setup();
		
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