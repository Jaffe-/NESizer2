## Hardware

The following images shows the most current schematics.

Main board:
![alt text](https://raw.githubusercontent.com/Jaffe-/NESizer2/master/hardware/mainboard.png "Main board")

Front panel board:
![alt text](https://raw.githubusercontent.com/Jaffe-/NESizer2/master/hardware/panelboard.png "Panel board")

The circuit essentially consists of the following parts:

- Atmega328 microcontroller
- 2A03
- LED matrix
- Switch matrix
- Battery backed SRAM
- Analog signal amplifier


#### Communication

The Atmega accesses the 2A03, LED matrix, switch matrix and SRAM through a simple bus system consisting of a 3-bit address bus and an 8-bit data bus. Bits 0, 1 and 2 of **PORTB** are used as the address, while bits 0 and 1 of **PORTC** and bits 2 to 7 of **PORTD** are connected to the data bus. The odd choice of combining some bits from **PORTC** with some bits of **PORTD** is because bits 0 and 1 of **PORTD** are used by the USART which takes care of MIDI communication. A 74HC238 decoder is used to decode the 3-bit address into one of eight activation signals for each component. The addresses are decoded as follows (R and W indicate read or write direction, respectively):

- 0: 2A03 data bus (W) 
- 1: LED matrix column (W)
- 2: matrix row for both LED and switch matrices (W)
- 3: switch matrix column (R)
- 4: memory, low 8 bits of address (W)
- 5: memory, middle 8 bits of address (W)
- 6: memory, high 3 bits of address and chip select bits (W)

Each writable component is connected to the databus via a latch in order to store the value when the bus is being used for something else.


#### 2A03 setup

Both the Atmega and the 2A03 are clocked by a 20 MHz crystal oscillator circuit based on 74HC04 inverters. The 2A03 divides this clock by 12 internally to provide a 1.66 MHz clock for the 6502 and APU. This is a bit lower than the usual frequency for the 2A03 (1.79 MHz), but it has no serious impact on APU operation (the APU timer values needed to produce a given note become a bit different).

The Atmega is hooked up to the 2A03 using the following connections:

- **PD2** .. **PD7**  --->  74HC573 latch ---> **D2** .. **D7** on 2A03 
- **PC0** .. **PC1**  --->  74HC573 latch ---> **D0** .. **D1** on 2A03
- **PC2**  --->  **RESET** pin on 2A03
- **PC3**  <---  **R/W** pin on 2A03

The reset connection could possibly be omitted (and the 2A03 reset pin just connected to a standard reset circuit), but connecting it to the Atmega allows for the 2A03 to be reset at any time, which might be handy in case the 6502 should halt.

The **R/W** output from the 2A03 is necessary to synchronize with the CPU.

In addition to the connections to the Atmega, some other connections are necessary for the 2A03 to function properly:

- **/NMI** and **/IRQ** are pulled high to avoid interrupts disturbing the 6502
- Pin 30 (diagonistics pin) is pulled low
- **SND1** and **SND2** (APU outputs) are pulled low via 100 ohm resistors

The 100 ohm pull-down resistors on **SND1** and **SND2** are required for the DACs in the APU to function properly. (This also has the effect that the output signals are very weak.)

Apart from this and the usual power supply connections, there are no further connections made. The address bus and the gamepad inputs/outputs are simply left unconnected. 


#### LED and switch matrices

These are both pretty standard. The LED matrix is a column scan matrix with 8 columns and 5 rows. 

The switch matrix is a row scan matrix with 8 columns and 3 rows. The lower five bits of a 74HC573 latch are used to hold the selected LED row, and the upper three bits are used to hold the selected switch row. This latch is accessed through the data bus by selecting address 2. 

The LED column is specified by writing to a 74HC573 latch connected to the data bus and selected with address 1. (Note that in order to light an LED at row *x* and column *y*, a 1 must be written at bit position *y* in the column latch, while a 0 must be written to bit position *x* of the row latch. This is because the column latch is sourcing the current while the row latch is sinking it.) 

Reading a switch state is done by activating the correct row in the row latch, and then selecting address 3 and reading from the bus. 


#### Battery backed SRAM memory

There are two SRAM ICs of 512KByte each, thus a 20 bit address is needed to address each individual byte (1MB in total). Since the databus is only 8 bit wide, three latches are used to specify an address before reading or writing to memory. The lowest 8 bits are written to the latch at databus address 4, the next 8 bits to the latch at address 5, and the next three bits to the latch at address 6. This totals to 19 bits, enough to address any byte in one of the ICs. The desired chip is selected by the upper two bits of the high address latch, where a value of 0b10 selects the first, 0b01 the other, and 0b11 neither. 

The write enable (/WE) line of the SRAM ICs is connected to pin 4 of **PORTC**. This pin must be kept high at all times except when a write is to performed. When an address is set up and a desired value is put on the databus, this pin is pulled low and thereafter pulled high to write the value to the address in memory. 

The SRAM ICs will retain their data as long as their supply voltage is larger than 1.5 V and the chip select lines are held high. The battery backup circuit takes care of this. A constant supply voltage is supplied by either a 3 V coin cell battery, or the regulated 5 V supply when the NESizer is powered on. The supplies are connected through diodes to prevent the 5 V supply from being applied to the battery, and to minimize the current flowing out of the battery when the 5 V supply is off.

A simple transistor switch circuit is used to force the chip select lines high when the 5 V supply is turned off. R19 and R20 set up a resistive divider giving a base voltage of the transistors that is about 0.15 * VCC. Assuming a typical VBE drop of 0.6 V, this gives a threshold of about 4 V for the transistor to start turning on. The pullups R21 and R22 ensure that the chip enable outputs are tied to MEM_VCC when the transistors are off. As the transistors are turned on, the outputs will follow the inputs closely, with a small drop between the emitter and collector.


#### MIDI

The MIDI input circuit is the standard circuit suggested in the MIDI standard, using an optocoupler (6N138) to isolate the MIDI current loop from the circuit. The incoming signal goes to the RX input of the Atmega's USART.


#### Output audio path

There are two identical output paths for the two 2A03 sound outputs, and a mixer that mixes the two audio signals with approximately the same ratio as in the NES.

The signal is AC-coupled by C8, C11 to the volume potentiometers set up as voltage dividers (VR1L, VR1R). The AC coupling is necessary to avoid noises when turning the volume potentiometers. It is then AC coupled by C9, C12 in to the virtual ground of an inverting amplifier with a gain of -6.8 set up by R8/R7 and R11/R10.  The non-inverting input of the op-amps IC3A, IC3B is set by a voltage divider to 2.5 V, so the output voltage is 2.5 V - 6.8 * v_in. In the feedback loop capacitors C26, C27 stabilize and also set the upper corner frequency of the bandwidth. The mix is done by passively summing the two resulting signals at the virtual ground of a third inverting amplifier. This ensure minimal crossbleed between the two output paths.


 