;;; Copyright (c) 2014-2015 Johan Fjeldtvedt
;;;
;;; This file is part of NESIZER
;;;
;;; NESIZER is free software: you can redistribute it and/or modify
;;; it under the terms of the GNU General Public License as published by
;;; the Free Software Foundation, either version 3 of the License, or
;;; (at your option) any later version.
;;;
;;; NESIZER is distributed in the hope that it will be useful,
;;; but WITHOUT ANY WARRANTY without even the implied warranty of
;;; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
;;; GNU General Public License for more details.
;;;
;;; You should have received a copy of the GNU General Public License
;;; along with Foobar.  If not, see <http: //www.gnu.org/licenses/>.


;;; 2a03_asm.s - Low level 2A03 I/O interfrace
;;;
;;; Functions for communicating with the 6502 processor in the 2A03.
;;; Provides functions for
;;;
;;; 	* Setting an APU register
;;; 	* Disabling 6502 interrupts
;;; 	* Resetting the 6502 program counter
;;; 	* Detecting the type of 2A03 used 
;;;
;;; Communication is done by monitoring the 6502's R/W line. When nothing is
;;; being done, the 6502 is being fed an STA zero page instruction, with opcode
;;; 0x85, so the 6502 is effectively trying to store its A register in memory
;;; address 0x85. This instruction takes three cycles, where the last one is the
;;; write to memory. About halfway through the last cycle, the R/W line is being
;;; pulled low. When it goes high again, the 6502 has started on the first cycle
;;; of the next instruction, and this is when a new instruction can be fed to
;;; it.
;;;
;;; Depending on the 2A03/07 type used (NTSC, clone or PAL), the amount of
;;; Atmega cycles per 6502 cycle is 12, 15 or 16. This means that when writing
;;; data to the 6502, the code must be timed so that it is exactly 12, 15 or 16
;;; Atmega cycles between each time the databus is updated.  
;;;
;;; Every write sequence is initiated by synchronizing with the 6502, which
;;; means waiting for R/W to transition from low to high, followed by a sequence
;;; of cycle exact writes to the databus, where the last write is the STA_zp
;;; opcode which is keeping the 6502 busy and synchronizable. 

	
;; Opcodes:
LDA_imm = 0xA9
STA_abs = 0x8D
STA_zp = 0x85
JMP_abs = 0x4C
SEI = 0x78

;; R/W bit position
RW = 3

;; I/O ports
PINC = 0x06
PORTC = 0x08
PORTD = 0x0B
TCCR0B = 0x25
TCNT0 = 0x26
	
.global register_set12
.global register_set15
.global register_set16
.global disable_interrupts12
.global disable_interrupts15
.global disable_interrupts16
.global reset_pc12
.global reset_pc15
.global reset_pc16
.global detect
.section .text


;;; ----------------------------------------------------------------------------

;;; SYNC macro
;;;
;;; Waits for the R/W line to go low, then to go high. When this has happened
;;; the 6502 is just finished with the last cycle of its idle STA_zp
;;; instruction, and a new instruction can be put on the bus.
	
.macro SYNC
1:	sbic PINC, RW
	rjmp 1b
2:	sbis PINC, RW
	rjmp 2b
.endm


;;; ----------------------------------------------------------------------------

;;; REGISTER_SET
;;;
;;; Parameters: r24: APU register
;;; 		r22: value
;;; Return:	none
;;; 
;;; Puts a given value in a given APU register by writing the following
;;; instructions:
;;; 
;;; LDA #<val>
;;; STA 0x40<reg>
;;;
;;; The write is followed by an STA_zp to keep the 6502 running.

.macro REGISTER_SET fill
	in r18, PORTC
	andi r18, 0xFC          
	in r19, PORTD
	andi r19, 0x03  	

	;; Put PORTC and PORTD bits of LDA_imm in r20, r21
	mov r20, r18             
	ori r20, LDA_imm & 0x03
	mov r21, r19             
	ori r21, LDA_imm & 0xFC	

	SYNC
	
	.rept \fill
	nop
	.endr	
	out PORTC, r20	       
	out PORTD, r21          

	;; Write value to 6502
	mov r20, r22
	andi r20, 0x03
	or r20, r18		
	mov r21, r22
	andi r21, 0xFC
	or r21, r19
	nop
	nop
	nop
	nop
	.rept \fill
	nop
	.endr
	out PORTC, r20
	out PORTD, r21
	
	;; Write STA_abs:
	mov r20, r18
	ori r20, STA_abs & 0x03
	mov r21, r19
	ori r21, STA_abs & 0xFC
	nop
	nop
	nop
	nop
	nop
	nop
	.rept \fill
	nop
	.endr
	out PORTC, r20
	out PORTD, r21

	;; Write low byte:
	mov r20, r24
	andi r20, 0x03
	or r20, r18
	mov r21, r24
	andi r21, 0xFC
	or r21, r19
	nop
	nop
	nop
	nop
	.rept \fill
	nop
	.endr
	out PORTC, r20
	out PORTD, r21

	;; Write high byte
	mov r21, r19
	ori r21, 0x40
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	.rept \fill
	nop
	.endr
	out PORTC, r18
	out PORTD, r21

	;; Write STA_zp
	mov r20, r18
	ori r20, STA_zp & 0x03
	mov r21, r19
	ori r21, STA_zp & 0xFC
	nop
	nop
	nop
	nop
	nop
	nop
	.rept \fill
	nop
	.endr
	out PORTC, r20
	out PORTD, r21

	ret
.endm

;;; The only thing differing between the three varieties are the number of nops
;;; to pad out each write sequence with.
	
register_set12:
	REGISTER_SET 0

register_set15:
	REGISTER_SET 3

register_set16:
	REGISTER_SET 4

	
;;; ----------------------------------------------------------------------------

;;; DISABLE_INTERRUPTS
;;;
;;; Parameters: none
;;; Return: 	none
;;; 
;;; Sends an SEI instruction followed by the idling STA_zp
	
.macro DISABLE_INTERRUPTS fill
	in r18, PORTC
	andi r18, 0xFC          ; Keep PORTC & 0xFC in r18
	in r19, PORTD
	andi r19, 0x03  	; Keep PORTD & 0x03 in r19

	mov r20, r18             
	ori r20, SEI & 0x03
	mov r21, r19             
	ori r21, SEI & 0xFC	

	SYNC
	
	.rept \fill
	nop
	.endr
	out PORTC, r20
	out PORTD, r21

	;; Write STA_zp
	mov r20, r18
	ori r20, STA_zp & 0x03
	mov r21, r19
	ori r21, STA_zp & 0xFC
	nop
	nop
	nop
	nop
	nop
	nop
	.rept \fill
	nop
	.endr
	out PORTC, r20
	out PORTD, r21

	ret
.endm
	
disable_interrupts12:
	DISABLE_INTERRUPTS 0
	
disable_interrupts15:
	DISABLE_INTERRUPTS 3

disable_interrupts16:
	DISABLE_INTERRUPTS 4
	
	
;;; ----------------------------------------------------------------------------
;;; RESET_PC
;;;
;;; Parameters: none
;;; Return:	none
;;; 
;;; Resets the program counter by sending a JMP_abs opcode followed by just
;;; STA_zp. This makes the 6502 jump to address $8585 and continue 'storing' to
;;; $85.
	
.macro RESET_PC fill
	in r18, PORTC
	andi r18, 0xFC          ; Keep PORTC & 0xFC in r18
	in r19, PORTD
	andi r19, 0x03  	; Keep PORTD & 0x03 in r19

	;; Write JMP_abs
	mov r20, r18
	ori r20, JMP_abs & 0x03
	mov r21, r19
	ori r21, JMP_abs & 0xFC

	SYNC
	
	.rept \fill
	nop
	.endr
	out PORTC, r20
	out PORTD, r21

	;; Write STA_zp
	mov r20, r18
	ori r20, STA_zp & 0x03
	mov r21, r19
	ori r21, STA_zp & 0xFC
	nop
	nop
	nop
	nop
	nop
	nop
	.rept \fill
	nop
	.endr
	out PORTC, r20
	out PORTD, r21

	ret
.endm	

reset_pc12:
	RESET_PC 0

reset_pc15:
	RESET_PC 3

reset_pc16:
	RESET_PC 4

	
;;; ----------------------------------------------------------------------------
	
;;; detect -- 2A03 type auto detection
;;;
;;; Parameters: none
;;; Return:	r24: number of Atmega clock cycles per 6502 clock cycle
;;; 
;;; When this function is run, the 6502 is being fed the STA instruction with
;;; absolute addressing, which takes four cycles. At its fourth cycle, it pulls
;;; the R/W line down. This can be used to measure how many clock cycles the
;;; Atmega has done for each cycle the 6502 has done by counting how many cycles
;;; it takes between each time R/W transitions from low to high. In this
;;; function the numer of Atmega cycles is counted for two such transitions to
;;; ensure that the timer is started and stopped at the exact same phase of the
;;; R/W transition. The counted number of cycles is divided by 8 to get the
;;; number of Atmega cycles per one 6502 cycle.

detect:
	ldi r18, 1 
	ldi r19, 0
	out TCNT0, r19
	
	;; Wait for R/W to tranisiton from low to high
	SYNC

	;; Start timer 0
	out TCCR0B, r18
	
	;; Wait for R/W to transition from low to high twice
	SYNC
	SYNC

	;; Stop timer 0
	out TCCR0B, r19

	;; Read timer count and divide by 8
	in r24, TCNT0
	lsr r24
	lsr r24
	lsr r24

	ret
