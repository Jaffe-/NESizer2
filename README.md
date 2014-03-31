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

The reason for using this opcode instead of the more obvious `NOP` is to be able to synchronize with the CPU. This synchronization is done by monitoring the **R/W** line of the 2A03. When it goes low, the 6502 is writing to memory. This *only* happens when the `STA $85` instruction is in its third cycle, which means that the next cycle will be the fetch of the next opcode.

The synchronization is done in the function `sync` by waiting for **R/W** to go low and **PHI2** to go high. When this is the case, the next time **PHI2** transitions from low to high the new instruction can be put on the bus. 


#### Writing to the APU registers

Writing to the APU registers is done by making the 6502 perform the following series of instructions:

    LDA #VALUE		0xA9 VALUE
    STA $40RR		0x8D 0xRR 0x40

where 0xRR is the low byte of the register address to be written to. The Atmega must put these byte strings on the 6502 databus when **PHI2** goes high, and keep them on there long enough for the 6502 to read. The `databus_wait` function is used to wait for **PHI2**'s transition. 

When a register write sequence (or the last of many) is done, the function `reset_pc` is called. This sends a `JMP $0000` instruction (in the same manner as described) to reset the program counter back to $0000. This reset is necessary to keep the CPU from reading from addresses in the internal register range, which could lead to a bus conflict where the internal registers most likely win and the CPU fetches an unintentional opcode.

Lastly, the Atmega puts the idle `STA` opcode (0x85) back on the bus.


#### Reading from status register

The status register is the only readable APU register, and it contains information that is pretty much useless when not doing interrupt-based programming of the APU. Nonetheless I think it would be nice to be able to read its contents, just as a challenge more than anything. 

Reading from the status register is done by sending the following instruction:

    LDA $4015		0xAD 0x15 0x40
    
After the three bytes are output (as described above), the Atmega waits for **PHI2** to go low and then high again. When this happens, the 6502 is in the last cycle of the `LDA $4015` instruction, where it reads from the APU status register. Even though the register is read internally in the IC, the 6502 still drives the buses, and so the value it reads from $4015 is visible on the databus. When this happens, **PORTD** on the Atmega is switched from output to input, and the databus value is stored. **PORTD** is then tri-stated and then switched back to output, and a NOP instruction opcode is put on the bus. 
