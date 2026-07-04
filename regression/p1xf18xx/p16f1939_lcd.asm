;
; 
; Copyright (c) 2025 Roy Rankin
; MACROS and some code included the following
;************************************************************************
;*	Microchip Technology Inc. 2006
;*	02/15/06
;*	Designed to run at 20MHz
;************************************************************************
; gpsim command
.command macro x
  .direct "C", x
  endm


	;; The purpose of this program is to test gpsim's ability 
	;; to simulate a pic 16F1939.
	;; Specifically, LCD module with Type A 

	list		p=16F1939
	#include	p16f1939.inc
        include <coff.inc>


        __CONFIG _CONFIG1, _CP_OFF & _WDTE_ON &  _FOSC_INTOSC & _PWRTE_ON &  _BOREN_OFF & _MCLRE_OFF & _CLKOUTEN_OFF & _IESO_ON



	errorlevel	-302


variables	UDATA_SHR
temp1           RES     1
temp2           RES     1
loop_cnt	RES	1
lmux		RES	1


STARTUP CODE
	NOP
	goto	start
	NOP
	NOP
	NOP
PROG1 	CODE

;
MAIN    CODE
start	
   .sim "p16f1939.frequency = 20000000."
   .sim "break c 0x2000000"
   .sim "module lib libgpsim_modules"
   .sim "module load pu bias1"
   .sim "module load pu bias2"
   .sim "module load pu bias3"
    .sim "module load lcd_7segments NUM"
    .sim "module load lcd_7segments LMUX"
   .sim "node n1"
   .sim "attach n1 bias1.pin portb1"
   .sim "node n2"
   .sim "attach n2 bias2.pin portb2"
   .sim "node n3"
   .sim "attach n3 bias3.pin portb3"
   .sim "node n_com0"
   .sim "attach n_com0 portb4 portc0 NUM.com0 LMUX.com0"
   .sim "node n_com1"
   .sim "attach n_com1 portb5 NUM.com1 LMUX.com1"
   .sim "node n_com2"
   .sim "attach n_com2 porta2 NUM.com2 LMUX.com2"
   .sim "node n_com3"
   .sim "attach n_com3 porta3 NUM.com3 LMUX.com3"
   .sim "node n_seg0"
   .sim "attach n_seg0 portb0 portc1 NUM.seg0"
   .sim "node n_seg1"
   .sim "attach n_seg1 porta6 NUM.seg1"
   .sim "node n_seg2"
   .sim "attach n_seg2 porta7  NUM.seg2"
   .sim "node n_seg3"
   .sim "attach n_seg3 portc2  NUM.seg3"
   .sim "node n_seg4"
   .sim "attach n_seg4 porta4  NUM.seg4"
   .sim "node n_seg5"
   .sim "attach n_seg5 porta5  NUM.seg5"
   .sim "node n_seg6"
   .sim "attach n_seg6 portc3  NUM.seg6"
   .sim "node n_seg8"
   .sim "attach n_seg8 portc7  LMUX.seg0"
   .sim "node n_seg9"
   .sim "attach n_seg9 portc6 LMUX.seg1"
   .sim "node n_seg10"
   .sim "attach n_seg10 portc5  LMUX.seg2"
   .sim "node n_seg11"
   .sim "attach n_seg11 portc4  LMUX.seg3"
   .sim "node n_seg12"
   .sim "attach n_seg12 porta0  LMUX.seg4"
   .sim "node n_seg13"
   .sim "attach n_seg13 portb7  LMUX.seg5"
   .sim "node n_seg14"
   .sim "attach n_seg14 portb6  LMUX.seg6"
   
   .sim "scope.ch0 = 'portc0'"
   .sim "scope.ch1 = 'portb5'"
   .sim "scope.ch2 = 'portc1'"
   .sim "scope.ch3 = 'porta6'"
   .sim "bias1.xpos = 300."
   .sim "bias1.ypos = 160."
   .sim "bias1.voltage = 1.67"   ; for bias = 3
;   .sim "bias1.voltage = 2.5"	  ; for bias = 2
   .sim "bias2.xpos = 300."
   .sim "bias2.ypos = 100"
   .sim "bias2.voltage = 3.33"	  ; for bias = 3
;   .sim "bias2.voltage = 2.5"	  ; for bias = 2
   .sim "bias3.xpos = 300."
   .sim "bias3.ypos = 040"
   .sim "bias3.voltage = 5.00"
    .sim "p16f1939.xpos = 72"
    .sim "p16f1939.ypos = 36"

    .sim "NUM.Lmux = 0"
    .sim "NUM.xpos = 468"
    .sim "NUM.ypos = 228"
    
    .sim "LMUX.Lmux = 0"
    .sim "LMUX.xpos = 264"
    .sim "LMUX.ypos = 228"





	call	ProgInit		; Get everything running step-by-step
	goto	setup_L0
char_loop:
	movf	loop_cnt,W
	btfsc	lmux,0		; bit 0 lmux0
	call	load_lmux0
	btfsc	lmux,1		; bit 1 lmux1
	call	load_lmux1
	btfsc	lmux,2		; bit 2 lmux2
	call	load_lmux2
	btfsc	lmux,3		; bit 3 lmux3
	call	load_lmux3
        movlw   0x1f		;delay value
        movwf   temp2
        clrf    temp1
        call    delay
	clrwdt
	incf	loop_cnt,F
	btfss   loop_cnt,4	;exit loop at 16
	goto   char_loop
;    .assert "pause"
	nop
	btfsc	lmux,0		; now lmux1 goto lmux1
	goto	setup_L1
	btfsc	lmux,1		; now lmux1 goto lmux2
	goto	setup_L2
	btfsc	lmux,2		; now lmux2 goto lmux3
	goto	setup_L3
leave:
  .assert "'*** PASSED p16f1939 LCD test'"
	goto $

;****************** Initialize Registers and Variables  *****************
;************************************************************************
ProgInit

	banksel ANSELA
	clrf    ANSELA
	clrf    ANSELB
	banksel	PORTA
	clrf	PORTA			; Set all bits to zero on Port A
	banksel TRISA
	clrf	TRISA
 ;	clrf	TRISC
	BANKSEL T1CON
	movlw	(1<<T1OSCEN) | (1<<TMR1ON)
	movwf	T1CON
	clrf	loop_cnt

	BANKSEL LCDPS
	; check register write bits and configure
	movlw	0xff
	movwf	LCDPS
    .assert "lcdps == 0xcf, '*** FAIL p16f1939_lcd lcdps write bits'"
	nop
	movlw	0x0f ;| (1<<BIASMD)	;prescaler 16; bias = 1/2
	movwf	LCDPS
	movlw	0x7f	;check bits except LCDEN
	movwf	LCDCON
    .assert "lcdcon == 0x4f, '*** FAIL p16f1939_lcd lcdcon write bits'"
	nop
	movlw	0xff
	movwf   LCDREF
    .assert "lcdref == 0xee, '*** FAIL p16f1939_lcd lcdref write bits'"
	nop
;	movlw   0x0e	; enable all VLCD external bias pins
	movlw   (1<<LCDIRE) ;| (1<<LCDIRS) ; Internal reference, Vtop=3.072
	movwf	LCDREF
	movlw	0xff
	movwf	LCDCST
    .assert "lcdcst == 0x07, '*** FAIL p16f1939_lcd lcdcst write bits'"
	nop
	clrf	LCDCST
	return

setup_L0:
	bcf	LCDCON,LCDEN
	btfsc	LCDPS,LCDA
	goto	$-1
	clrf	loop_cnt
	movlw	0x7f	;7 segments with 1 com
	movwf	LCDSE0
	movwf	LCDSE1
	movlw	0x3f	;0 show zero
	movwf	LCDDATA1
	clrf	LCDDATA3
	clrf	LCDDATA4
	clrf	LCDDATA6
	clrf	LCDDATA7
	clrf	LCDDATA9
	clrf	LCDDATA10
    .command "NUM.Lmux = 0"
	nop
    .command "LMUX.Lmux = 0"
	nop
	movlw	(1<<0)
	movwf	lmux
	movlw	(1<<LCDEN) | (1 << CS0) 	;t1 clock, LMUX = 0
	movwf	LCDCON
	goto	char_loop

setup_L1:
    .assert "(pir2&(1<<2)) == 0, '***FAILED p16f1939 unexpected LCDIF set'"
	nop
	bcf	LCDCON,LCDEN
	btfsc	LCDPS,LCDA
	goto	$-1
    .assert "(pir2&(1<<2)) == (1<<2), '***FAILED p16f1939 LCDIF not set'"
	nop
	clrf	loop_cnt
	movlw	0x0f	;4 segments with 2 com
	movwf	LCDSE0
	movwf	LCDSE1
	movlw	1	; set LMUX display to 1
	call	load_lmux1
	movf	LCDDATA0,W
	movwf	LCDDATA1
	movf	LCDDATA3,W
	movwf	LCDDATA4
	clrf	LCDDATA6	; clear other DATA registers`
	clrf	LCDDATA7
	clrf	LCDDATA9
	clrf	LCDDATA10
    .command "NUM.Lmux=1"
	nop
    .command "LMUX.Lmux=1"
	nop
	movlw	(1<<1)
	movwf	lmux
	movlw	(1<<LCDEN) | (1 << CS0) | (1 << LMUX0)	;t1 clock, LMUX = 1
	movwf	LCDCON
	goto	char_loop

setup_L2:
	bcf	LCDCON,LCDEN
	btfsc	LCDPS,LCDA
	goto	$-1
	clrf	loop_cnt
	movlw	0x07	;3 segments with 3 con
	movwf	LCDSE0
	movwf	LCDSE1
	movlw	2	; set LMUX display to 2
	call	load_lmux2
	movf	LCDDATA0,W
	movwf	LCDDATA1
	movf	LCDDATA3,W
	movwf	LCDDATA4
	movf	LCDDATA6,W
	movwf	LCDDATA7
	clrf	LCDDATA9
	clrf	LCDDATA10
    .command "NUM.Lmux=2"
	nop
    .command "LMUX.Lmux=2"
	nop
	movlw	(1<<2)
	movwf	lmux
	movlw	(1<< LCDEN)|(1 << CS0) | (1 << LMUX1)	;t1 clock, LMUX = 2
	movwf	LCDCON
	clrwdt
	goto	char_loop

setup_L3:
	bcf	LCDCON,LCDEN
	btfsc	LCDPS,LCDA
	goto	$-1
	clrf	loop_cnt
	movlw	0x03	;2 segments with 4 com
	movwf	LCDSE0
	movwf	LCDSE1
	movlw	3	; set LMUX display to 3
	call	load_lmux3
	movf	LCDDATA0,W
	movwf	LCDDATA1
	movf	LCDDATA3,W
	movwf	LCDDATA4
	movf	LCDDATA6,W
	movwf	LCDDATA7
	movf	LCDDATA9,W
	movwf	LCDDATA10
    .command "NUM.Lmux = 3"
	nop
    .command "LMUX.Lmux = 3"
	nop
	movlw	(1<<3)
	movwf	lmux
	movlw	(1<<LCDEN) | (1 << CS0) | (1 << LMUX0) | (1 << LMUX1)  ;t1 clock, LMUX = 3
	movwf	LCDCON
	goto	char_loop


delay
        decfsz  temp1,f
         goto   $+2
        decfsz  temp2,f
         goto   delay
        return

;Fof Lmux=0 bits 0-7 data0
num_L0: addwf PCL,F
        retlw  0x3f	;'0'
        retlw  0x06	;'1'
        retlw  0x5b	;'2'
        retlw  0x4f	;'3'
        retlw  0x66	;'4'
        retlw  0x6d	;'5'
        retlw  0x7d	;'6'
        retlw  0x07	;'7'
        retlw  0x7f	;'8'
        retlw  0x67	;'9'
        retlw  0x77	;'A'
        retlw  0x7c	;'B'
        retlw  0x39	;'C'
        retlw  0x5e	;'D'
        retlw  0x79	;'E'
        retlw  0x71	;'F'

;For Lmux=1, bits 0-3 data0 4-7 lcddata3
num_L1: addwf PCL,F
        retlw  0xd7	;'0'
        retlw  0x03	;'1'
        retlw  0x75	;'2'
        retlw  0x37	;'3'
        retlw  0xa3	;'4'
        retlw  0xb6	;'5'
        retlw  0xf6	;'6'
        retlw  0x13	;'7'
        retlw  0xf7	;'8'
        retlw  0xb3	;'9'
        retlw  0xf3	;'A'
        retlw  0xe6	;'B'
        retlw  0xd4	;'C'
        retlw  0x67	;'D'
        retlw  0xf4	;'E'
        retlw  0xf0	;'F'

;For Lmux=2, bits 0-1 data0 2-4 lcddata3 5-7 lcddat6
num_L2: addwf PCL,F
        retlw  0xd7	;'0'
        retlw  0x05	;'1'
        retlw  0x5e	;'2'
        retlw  0x4f	;'3'
        retlw  0x8d	;'4'
        retlw  0xcb	;'5'
        retlw  0xdb	;'6'
        retlw  0x45	;'7'
        retlw  0xdf	;'8'
        retlw  0xcd	;'9'
        retlw  0xdd	;'A'
        retlw  0x9b	;'B'
        retlw  0xd2	;'C'
        retlw  0x1f	;'D'
        retlw  0xda	;'E'
        retlw  0xd8	;'F'

;For Lmux=3, bits 0-1 data0 2-3 lcddata3
;     4-5 dat6 6-7 lcddata9
num_L3: addwf PCL,F
        retlw  0xed	;'0'
        retlw  0x44	;'1'
        retlw  0xd9	;'2'
        retlw  0xd5	;'3'
        retlw  0x74	;'4'
        retlw  0xb5	;'5'
        retlw  0xbd	;'6'
        retlw  0xc4	;'7'
        retlw  0xfd	;'8'
        retlw  0xf4	;'9'
        retlw  0xfc	;'A'
        retlw  0x3d	;'B'
        retlw  0xa9	;'C'
        retlw  0x5d	;'D'
        retlw  0xb9	;'E'
        retlw  0xb8	;'F'

load_lmux0:
	call	num_L0
	movwf	LCDDATA0
	return

load_lmux1:
	call	num_L1
	movwf	temp1
	andlw	0x0f
	movwf	LCDDATA0
	swapf	temp1,W
	andlw	0x0f
	movwf	LCDDATA3
	return

load_lmux2:
	call	num_L2
	movwf	temp1
	andlw	0x03
	movwf	LCDDATA0
	rrf	temp1,F
	rrf	temp1,F
	movf	temp1,W
	andlw	0x07
	movwf	LCDDATA3
	rrf	temp1,F
	rrf	temp1,F
	rrf	temp1,F
	movf	temp1,W
	andlw	0X07
	MOVWF	LCDDATA6
	return

load_lmux3:
	call	num_L3
	movwf	temp1
	andlw	0x03
	movwf	LCDDATA0
	rrf	temp1,F
	rrf	temp1,F
	movf	temp1,W
	andlw	0x03
	movwf	LCDDATA3
	rrf	temp1,F
	rrf	temp1,F
	movf	temp1,W
	andlw	0X03
	MOVWF	LCDDATA6
	rrf	temp1,F
	rrf	temp1,F
	movf	temp1,W
	movwf	LCDDATA9
	return
	end	
