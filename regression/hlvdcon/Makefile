# LVDCON module regression test
#
#

include ../make.regression

CPUS =  18c242 18c252 18c442 18c452 18f242 18f252 18f442 18f248 18f258 \
 18f448 18f452 18f458 18f1220 18f1320 18f2221 18f2321 18f2420 18f2455 \
 18f2550 18f2520 18f2525 18f2620 18f4221 18f4321 18f4420 18f4455 18f4550 \
 18f4520 18f4620 18f6520

all :   $(addsuffix .cod,$(addprefix lvdcon_,$(CPUS)))

%.cod : %.o
	gplink --map  -o $@  $<



sim: $(addprefix sim_lvdcon_,$(CPUS))
