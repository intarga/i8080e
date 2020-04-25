CC = gcc $(CFLAGS)
CFLAGS = -Wall -Werror -Wextra

all: 8080-emulator

debug: CFLAGS += -O0 -g
debug: all
release: CFLAGS += -O3
release: all

8080-emulator: emulator.o disassembler.o
	$(CC) $^ -o $@

emulator.o: emulator.c disassembler.h
	$(CC) -c $<

disassembler.o: disassembler.c
	$(CC) -c $<

clean:
	rm *.o 8080-emulator
