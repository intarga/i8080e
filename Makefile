emulator: emulator.o disassembler.o
	gcc emulator.o disassembler.o -o emulator

emulator.o: emulator.c disassembler.h
	gcc -c emulator.c

disassembler.o: disassembler.c
	gcc -c disassembler.c

clean:
	rm *.o emulator
