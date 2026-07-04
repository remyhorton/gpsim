
   ;;  18F2520 LVDCON tests
   ;;
   ;; This regression test, tests the following LVDCON functions
   ;;   default reset value
   ;;	LVDCON turned on by LVDEN bit sets IRVST
   ;;	LVDCON turned off by LVDEN bit clears IRVST



   list p=18f2520

include "p18f2520.inc"
include <coff.inc>

	ORG	0

  .assert "(hlvdcon) == 0x05,'*** FAILED 18f2520 hlvdcon reset value'"
    nop
    bsf LVDCON,LVDEN
    nop
  .assert "(hlvdcon & 0x20) == 0x00,'*** FAILED 18f2520 lvdcon IRVST delay'"
    nop
    movlw 0
delay
    addlw 1
    btfss STATUS,Z
    goto delay
  .assert "(hlvdcon & 0x20) == 0x20,'*** FAILED 18f2520 lvdcon IRVST set'"
    nop
    bcf LVDCON,LVDEN
  .assert "(hlvdcon & 0x20) == 0x00,'*** FAILED 18f2520 lvdcon IRVST clear'"
    nop
  .assert "'*** PASSED p18f2520 lvdcon'"
    nop
    goto $

  end
