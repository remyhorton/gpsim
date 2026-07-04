
EXTENDED_INSTRUCTIONS=1

include ../make.regression

all : instructions.cod extended_instructions.cod reset.cod bug.cod

%.cod : %.o
	gplink --map -o $@  $<

