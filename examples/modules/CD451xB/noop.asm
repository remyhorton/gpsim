;   noop.asm - No-operation loop firmware
;
;   Copyright (C) Codethink, 2026.
;   SPDX-License-Identifier: BSD-3-Clause

PROCESSOR 12f1840
#include <p12f1840.inc>
    RADIX dec
    CONFIG FOSC=INTOSC, WDTE=OFF
    CONFIG MCLRE=ON
    CONFIG LVP=OFF
    CONFIG PLLEN=OFF

; Entrypoints
STARTUP     CODE    0x0000
    GOTO    entry
    GOTO    $

entry:
    CALL    setupClock
begin:
    NOP
    NOP
    NOP
    GOTO    begin

setupClock:
    BANKSEL OSCCON
    MOVLW   b'01100010'     ; 2MHz
    MOVWF   OSCCON
    RETURN

    END
