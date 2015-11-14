## Changelog

**August 2015**: Added memory context handling to allow sequential access and random access from different tasks.

**April - July 2015**: Hardware: Completed assembling new prototype boards. Software: Various minor fixes, refactored UI code.

**26/03/15**: Audio circuitry updated. Op-amps are now operating at 2.5V bias to extend the linear range. Emitter follower replaced by op-amp buffer.

**08/03/15**: Optimized the cent to period calculations, now uses a period lookup table and a linear approximation in between seminotes.

**07/03/15**: 2A03 communication functions moved from inline assembly in C to an own assembly file. Added functions for auto-detecting what kind of APU chip (2A03, 2A07, clone) is being used and adjusting the period tables, write functions etc. accordingly.

**02/03/15**: Removed /MEM_EN signal as it is not needed with the DS1210 based backup circuitry.

**01/03/15**: MIDI transfers have been improved. The UI now shows a progress bar (using the upper 16 button leds) when transferring data. Optimizations done in the memory write/read routines.

**28/02/15**: Rationalized the SRAM backup circuitry by replacing one of the DS1210s with two transistors and four resistors. The single DS1210 now has CE tied to ground and uses /CEO to switch two PNP transistors which connect /CE1 and /CE2 to their respective SRAM ICs.

**26/02/15**: Modulation is now done with cents instead of frequency as the 'unit of measurement'. This gives a consistant modulation effect across the octaves. It also turned out that using cents makes computing the corresponding timer period change simpler.

**16/01/15**: Fixed some serious issues with the SRAM circuitry. The unused battery inputs on the DS1210s are grounded, as described in the data sheet, so memory no longer ceases to read correctly a short while after power on. The enable signal of the decoder is now used to deactivate the decoder's outputs before changing the address bits, and turning them on again afterwards. This was to fix a bug where an unintended component was intermittently selected while the address bits were changing.

**11/11/14**: Main board PCB received and assembled. Everything worked well, but some noise and leakage between channels was detected. The audio path has now been redesigned.

**02/10/14**: Prototype PCB finished, waiting for them to arrive.

**22/09/14**: Glide/portamento works. I'm now working on the CPU PCB, and am currently going through the circuit to make sure everything is right. Some redesign has been done on the amplifier section.

**11/09/14**: New LFO modulation implementation. Noise envelope modulation of triangle pitch added.

**05/09/14**: MIDI is now fairly stable, next up is adding more MIDI commands. Uploading samples for the DMC channel via MIDI sysex works, but it is a bit buggy. Detuning of the square and triangle channels has been added. I plan on adding portamento/glide as well.

**31/08/14**: Started implementing MIDI input support. So far SQ1 responds to incoming note on/off messages!

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
