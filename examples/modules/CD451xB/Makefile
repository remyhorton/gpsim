BIN=noop.cod

all: $(BIN)

%.o : %.asm
	gpasm -c $<

%.cod : %.o
	gplink --map  -q -o $@ $<

clean::
	rm $(BIN:.cod=.hex)
	rm $(BIN:.cod=.cod)
	rm $(BIN:.cod=.lst)
	rm $(BIN:.cod=.map)


cd4510::
	gpsim noop.cod -c cd4510.stc

cd4516::
	gpsim noop.cod -c cd4516.stc
