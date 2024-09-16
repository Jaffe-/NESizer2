![alt text](https://raw.githubusercontent.com/Jaffe-/NESizer2/master/nesizer_black.png "NESIZER")

## NESizer2: 2A03 Synthesizer Project

[![Join the chat at https://gitter.im/Jaffe-/NESizer2](https://badges.gitter.im/Join%20Chat.svg)](https://gitter.im/Jaffe-/NESizer2?utm_source=badge&utm_medium=badge&utm_campaign=pr-badge&utm_content=badge)

![alt text](https://raw.githubusercontent.com/Jaffe-/NESizer2/master/nesizer_boards.jpg "NESIZER")

The main idea of this project is to use an isolated NES 2A03 CPU/APU IC as a stand-alone synthesizer, controlled by an Atmel Atmega microcontroller. (The project is named NESizer2 after a failed attempt to create a NES synthesizer using only the 2A03 alone, with its own ROM, RAM and I/O logic.)

The 2A03 IC consists of a 6502 CPU core (with some minor changes), a DMA controller and the Audio Processing Unit. The APU is controlled via 22 registers, which are connected internally to the 6502 only; there are no external input pins facilitating communication with the APU directly. This means that in order to control the APU, the 6502 must act as a proxy: The microcontroller must send the 6502 instructions to take a value and put it in a desired APU register.

This project is inspired by a similar approach taken here: http://www.soniktech.com/tsundere/, but the idea here is to have the microcontroller communicate more directly with the 2A03, with a minimum of circuitry to do so.

Check out the most recent demo [here](https://www.youtube.com/watch?v=pXKrs0bFvvk)!

See the documentation for a description of how the NESizer works:

- [Hardware documentation](docs/hardware.md)
- [Software documentation](docs/software.md)
