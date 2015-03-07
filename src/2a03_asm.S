
/*
  NESIZER

  (c) 2014-2015 Johan Fjeldtvedt

  2A03 communication
	
  Performs a write to an APU register by sending an LDA <value> instruction, then an 
  STA <reg> instruction, and finally putting the idle STA zero-page instruction back. 

  The code is cycle timed

*/
	#define __SFR_OFFSET 0
	#include <avr/io.h>	
	
	;; Opcodes:
	#define LDA_imm 0xA9
	#define STA_abs 0x8D
	#define STA_zp 0x85
	#define JMP_abs 0x4C
	#define SEI 0x78

	// Position of RW bit in PORTC
	#define RW 3
	
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

.macro SYNC
1:	sbic PINC, RW
	rjmp 1b
2:	sbis PINC, RW
	rjmp 2b
.endm
	
.macro REGISTER_SET fill
	in r18, PORTC
	andi r18, 0xFC          
	in r19, PORTD
	andi r19, 0x03  	

	// Put PORTC and PORTD bits of LDA_imm in r20, r21
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

	// Write value to 6502
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

register_set12:
	REGISTER_SET 0

register_set15:
	REGISTER_SET 3

register_set16:
	REGISTER_SET 4

	
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
	

detect:
	ldi r24, 0
	ldi r18, 4

loop:	
	SYNC
		
	// count for how long RW is high
1:	inc r24
	sbic PINC, RW
	rjmp 1b
	dec r18
	brne loop
	
	ret
