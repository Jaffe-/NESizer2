
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
;;; Uses timer 0 to count how many clock cycles it takes to synchronize twice
;;; with the 2A03. During this period the 2A03 should have executed exactly 8
;;; clock cycles, meaning that the counted clock cycles divided by 8 is the
;;; clock divisor of the 2A03 being used. 

	ldi r18, 1 
	ldi r19, 0
	
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
