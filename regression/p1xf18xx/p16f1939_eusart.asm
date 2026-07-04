   ;;  USART test 
   ;;
   ;;  The purpose of this program is to verify that gpsim's
   ;; USART functions properly. The USART module is used to loop
   ;; characters back to the receiver testing  RCIF interupts.
   ;;
   ;;
   ;;

	list	p=16f1939
	include <p16f1939.inc>
	include <coff.inc>
        __CONFIG _CONFIG1, _WDTE_OFF


.command macro x
  .direct "C", x
  endm

        errorlevel -302 
	radix dec
;----------------------------------------------------------------------
; RAM Declarations


;
INT_VAR        UDATA   0x70
w_temp          RES     1
status_temp     RES     1
pclath_temp     RES     1
;fsr_temp	RES	1

temp1		RES	1
temp2		RES	1
temp3		RES	1

tx_ptr		RES	1

rxLastByte	RES	1
rxFlag		RES	1

;----------------------------------------------------------------------
;   ********************* RESET VECTOR LOCATION  ********************
;----------------------------------------------------------------------
RESET_VECTOR  CODE    0x000              ; processor reset vector
        movlw  high  start               ; load upper byte of 'start' label
        movwf  PCLATH                    ; initialize PCLATH
        goto   start                     ; go to beginning of program



;------------------------------------------------------------------------
;
;  Interrupt Vector
;
;------------------------------------------------------------------------

INT_VECTOR   CODE    0x004               ; interrupt vector location

	movwf	w_temp
	swapf	STATUS,w
	clrf	STATUS
	movwf	status_temp
	movf	PCLATH,w
	movwf	pclath_temp
	clrf	PCLATH


	BANKSEL PIR1
	btfsc	INTCON,PEIE
	 btfss	PIR1,RCIF
	  goto	int_done

;;;	Received a Character
   .assert "rcreg == txreg, '*** FAILED p16f1939 sent character looped back'"
	nop
	BANKSEL RCREG
	movf	RCREG,W
	movwf	rxLastByte
	bsf	rxFlag,0
	
int_done:
	clrf	STATUS
	movf	pclath_temp,w
	movwf	PCLATH
	swapf	status_temp,w
	movwf	STATUS
	swapf	w_temp,f
	swapf	w_temp,w
	retfie


;; ----------------------------------------------------
;;
;;            start
;;

MAIN    CODE
start	

   .sim ".frequency=20e6"
   .sim "break c 70000"
   .sim "module library libgpsim_modules"
   .sim "module load usart U1"
   .sim ".xpos=72"
   .sim ".ypos=156"
 ;  .sim "U1.xpos = 250.0"
;   .sim "U1.ypos = 80.0"

   .sim "node PIC_tx"
   .sim "node PIC_rx"

   ;; Tie the USART module to the PIC
   .sim "attach PIC_tx portc6 U1.RXPIN"
   .sim "attach PIC_rx portc7 U1.TXPIN"

   ;; Set the USART module's Baud Rate

   .sim "U1.txbaud = 28800"
   .sim "U1.rxbaud = 28800"
   .sim "U1.loop = true"
   .sim "scope.ch0='portc6'"
   .sim "scope.ch2='portc7'"

	;; USART Initialization
	;;
	;; Turn on the high baud rate (BRGH), disable the transmitter,
	;; disable synchronous mode.
	;;
	
	clrf	STATUS

	bsf	PORTC,6         ;Make sure the TX line drives high when 
                                ;it is programmed as an output.

	BANKSEL TRISC
;	bcf	OPTION_REG,NOT_RBPU	; turn on B pullups



	bsf	TRISC,7		;RX is an input
	bsf	TRISC,6		;TX is an input

	;; CSRC - clock source is a don't care
	;; TX9  - 0 8-bit data
	;; TXEN - 0 don't enable the transmitter.
	;; SYNC - 0 Asynchronous
	;; BRGH - 1 Select high baud rate divisor
	;; TRMT - x read only
	;; TX9D - 0 not used

	; for BRGH set (high rate divisor)
	; SPBRG = Freq/(baud * 16) - 1 
	; so SPBRG = 20e6/(28800 * 16) -1 = 42
	
	BANKSEL TXSTA
	movlw	(1<<BRGH)
	movwf	TXSTA 

	movlw   42		;28800 baud.
	movwf   SPBRG

  .assert "(portc & 0x40) == 0x40, 'FAILED: 16f1939 TX bit initilized as high'"
        nop

	clrf	tx_ptr
			
	;; Turn on the serial port
	movlw	(1<<SPEN) | (1<<CREN)
	movwf	RCSTA

	movf	RCREG,w          ;Clear RCIF
	bsf	INTCON,GIE
	bsf	INTCON,PEIE

	;; Delay for a moment to allow the I/O lines to settle
;	clrf	temp2
;	call	delay
	
	movf	RCREG,w          ;Clear RCIF
	movf	RCREG,w          ;Clear RCIF

	;; Test TXIF, RCIF bits of PIR1 are not writable

	BANKSEL PIR1
	clrf	PIR1
	bsf	PIR1,RCIF
	bsf	PIR1,TXIF
  .assert "pir1 == 0x00, '*** FAILED p16f1939 TXIF, RCIF not writable'"
	nop

	;; Enable the transmitter
	BANKSEL	TXSTA
	bsf	TXSTA,TXEN

  .assert "pir1 == 0x10, '*** FAILED p16f1939 TXIF should now be set'"
	nop

	;; Now Transmit some data and verify that it is transmitted correctly.
   .command "U1.tx = \"Hi!\r\n\""
	nop
	BANKSEL PIR1
	bcf	PIR1,RCIF
	btfss   PIR1,RCIF
	 goto	$-1
    .assert "rcreg == 0x48, '*** FAILED p16f1939 USART string tx H'"
	nop
	BANKSEL RCREG
	movf	RCREG,W

	BANKSEL PIR1
	btfss   PIR1,RCIF
	 goto	$-1
    .assert "rcreg == 0x69, '*** FAILED p16f1939 USART string tx i'"
	nop
	BANKSEL RCREG
	movf	RCREG,W
	BANKSEL PIR1
	btfss   PIR1,RCIF
	 goto	$-1
    .assert "rcreg == 0x21, '*** FAILED p16f1939 USART string tx !'"
	nop
	BANKSEL RCREG
	movf	RCREG,W
	BANKSEL PIR1
	btfss   PIR1,RCIF
	 goto	$-1
    .assert "rcreg == 0x0d, '*** FAILED p16f1939 USART string tx \\r'"
	nop
	BANKSEL RCREG
	movf	RCREG,W
	BANKSEL PIR1
	btfss   PIR1,RCIF
	 goto	$-1
    .assert "rcreg == 0x0a, '*** FAILED p16f1939 USART string tx \\n'"
	nop
	BANKSEL RCREG
	movf	RCREG,W

	.command "rcreg"
	nop
	BANKSEL PIE1
	bsf	PIE1,RCIE	; Enable Rx interrupts
	;bcf	STATUS,RP0
	call	TransmitNextByte
   .assert "U1.rx == 0x31, '*** FAILED p16f1939 sending 0x31'"
	nop
	call	TransmitNextByte
   .assert "U1.rx == 0x32, '*** FAILED p16f1939 sending 0x32'"
	nop
	call	TransmitNextByte
   .assert "U1.rx == 0x33, '*** FAILED p16f1939 sending 0x33'"
	call	TransmitNextByte
   .assert "U1.rx == 0x34, '*** FAILED p16f1939 sending 0x34'"
	call	TransmitNextByte
   .assert "U1.rx == 0x35, '*** FAILED p16f1939 sending 0x35'"
	call	TransmitNextByte
   .assert "U1.rx == 0x36, '*** FAILED p16f1939 sending 0x36'"
	call	TransmitNextByte
   .assert "U1.rx == 0x37, '*** FAILED p16f1939 sending 0x37'"
	call	TransmitNextByte
   .assert "U1.rx == 0x38, '*** FAILED p16f1939 sending 0x38'"
	call	TransmitNextByte
   .assert "U1.rx == 0x39, '*** FAILED p16f1939 sending 0x39'"
	call	TransmitNextByte
   .assert "U1.rx == 0x41, '*** FAILED p16f1939 sending 0x41'"
	call	TransmitNextByte
   .assert "U1.rx == 0x42, '*** FAILED p16f1939 sending 0x42'"
	call	TransmitNextByte
   .assert "U1.rx == 0x43, '*** FAILED p16f1939 sending 0x43'"
	call	TransmitNextByte
   .assert "U1.rx == 0x44, '*** FAILED p16f1939 sending 0x44'"
	nop

;
; setup tmr0
;
	BANKSEL OPTION_REG
        movlw  0x05          ; Tmr0 internal clock prescaler 64
        movwf  OPTION_REG

	BANKSEL PIR1
        movlw   0x55
        btfss   PIR1,TXIF       ;check for TXREG ready
         goto   $-1
	BANKSEL TMR0
        clrf    TMR0

	BANKSEL TXSTA
        movwf   TXREG
        btfss   TXSTA,TRMT ;Wait 'til through transmitting
         goto   $-1
;
	BANKSEL TMR0
;  At 28800  baud each bit takes 34.7 usec. So 9 bits is 0.3125 msec
;  10 bits 0.347 msec.  FOSC/4 = 5MHZ and TMR0 / 64 so TMR0 between
;  24 (0x18) and 27 (0x1b),

	movf	TMR0,W

  .assert "tmr0 > 24 && tmr0 < 27, '*** FAILED p16f1939 baud rate'"
	nop
	clrf	rxFlag
        call rx_loop

done:
  .assert  "'*** PASSED Usart p16f1939'"
	goto $


TransmitNextByte:	
	clrf	rxFlag
	call	tx_message
	BANKSEL PIR1
	btfss	PIR1,TXIF
	 goto	$-1
	BANKSEL TXREG
	movwf	TXREG

rx_loop:

	btfss	rxFlag,0
	 goto	rx_loop

	BANKSEL PIR1
	return

tx_message
	incf	tx_ptr,w
	andlw	0x0f
	movwf	tx_ptr
	addlw	TX_TABLE
	skpnc
	 incf	PCLATH,f
	movwf	PCL
TX_TABLE
	dt	"0123456789ABCDEF",0



delay	
	decfsz	temp1,f
	 goto 	$+2
	decfsz	temp2,f
	 goto   delay
	return

	end
