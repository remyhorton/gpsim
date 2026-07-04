
   ;;  18F2550 HLVDCON tests
   ;;
   ;; This regression test, tests the following HLVDCON functions
   ;;   default reset value
   ;;	HLVDCON turned on by LVDEN bit sets IRVST
   ;;	HLVDCON turned off by LVDEN bit clears IRVST



   list p=18f2550

include "p18f2550.inc"
include <coff.inc>

	ORG	0

  .assert "(hlvdcon) == 0x05,'*** FAILED 18f2550 hlvdcon reset value'"
    nop
    bsf LVDCON,LVDEN
    nop
  .assert "(hlvdcon & 0x20) == 0x00,'*** FAILED 18f2550 hlvdcon IRVST delay'"
    nop
    movlw 0
delay
    addlw 1
    btfss STATUS,Z
    goto delay
  .assert "(hlvdcon & 0x20) == 0x20,'*** FAILED 18f2550 hlvdcon IRVST set'"
    nop
    bcf LVDCON,LVDEN
  .assert "(hlvdcon & 0x20) == 0x00,'*** FAILED 18f2550 hlvdcon IRVST clear'"
    nop
  .assert "'*** PASSED p18f2550 hlvdcon'"
    nop
    goto $

  end
