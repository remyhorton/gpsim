
   ;;  18F2525 LVDCON tests
   ;;
   ;; This regression test, tests the following LVDCON functions
   ;;   default reset value
   ;;	LVDCON turned on by LVDEN bit sets IRVST
   ;;	LVDCON turned off by LVDEN bit clears IRVST



   list p=18f2525

include "p18f2525.inc"
include <coff.inc>

	ORG	0

  .assert "(hlvdcon) == 0x05,'*** FAILED 18f2525 hlvdcon reset value'"
    nop
    bsf LVDCON,LVDEN
    nop
  .assert "(hlvdcon & 0x20) == 0x00,'*** FAILED 18f2525 lvdcon IRVST delay'"
    nop
    movlw 0
delay
    addlw 1
    btfss STATUS,Z
    goto delay
  .assert "(hlvdcon & 0x20) == 0x20,'*** FAILED 18f2525 lvdcon IRVST set'"
    nop
    bcf LVDCON,LVDEN
  .assert "(hlvdcon & 0x20) == 0x00,'*** FAILED 18f2525 lvdcon IRVST clear'"
    nop
  .assert "'*** PASSED p18f2525 lvdcon'"
    nop
    goto $

  end
