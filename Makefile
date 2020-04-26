CC = gcc $(CFLAGS)
CFLAGS = -Wall -Werror -Wextra

all: 8080-emulator 8080-disassembler

debug: CFLAGS += -O0 -g
debug: all
release: CFLAGS += -O3
release: all

8080-emulator: emulator.o
	$(CC) $^ -o $@

emulator.o: emulator.c
	$(CC) -c $<

8080-disassembler: disassembler.c
	$(CC) $< -o $@

clean:
	rm *.o 8080-emulator 8080-disassembler
