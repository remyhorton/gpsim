
all :  lcd_mod.cod

ifdef EXTENDED_INSTRUCTIONS
  ASM_FLAGS = -c --extended
else
  ASM_FLAGS = -c
endif


%.o : %.asm
	gpasm $(ASM_FLAGS) $<

%.cod : %.o
	gplink --map  -q -o $@ $<

