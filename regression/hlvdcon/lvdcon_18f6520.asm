
   ;;  18F6520 WDT tests
   ;;
   ;; This regression test, tests the following LVDCON functions
   ;;   default reset value
   ;;	LVDCON turned on by LVDEN bit sets IRVST
   ;;	LVDCON turned off by LVDEN bit clears IRVST



   list p=18f6520

include "p18f6520.inc"
include <coff.inc>

	ORG	0

  .assert "(lvdcon) == 0x05,'*** FAILED 18f6520 lvdcon reset value'"
    nop
    bsf LVDCON,LVDEN
    nop
  .assert "(lvdcon & 0x20) == 0x00,'*** FAILED 18f6520 lvdcon IRVST delay'"
    nop
    movlw 0
delay
    addlw 1
    btfss STATUS,Z
    goto delay
  .assert "(lvdcon & 0x20) == 0x20,'*** FAILED 18f6520 lvdcon IRVST set'"
    nop
    bcf LVDCON,LVDEN
  .assert "(lvdcon & 0x20) == 0x00,'*** FAILED 18f6520 lvdcon IRVST clear'"
    nop
  .assert "'*** PASSED p18f6520 lvdcon'"
    nop
    goto $

  end
