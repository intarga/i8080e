TARGET = 8080-emulator

CC = gcc
CFLAGS = -Wall -Werror -Wextra

LINKER = gcc
LFLAGS = -Wall -Werror -Wextra

SRCDIR   = src
OBJDIR   = obj
BINDIR   = bin

SOURCES  := $(wildcard $(SRCDIR)/*.c)
INCLUDES := $(wildcard $(SRCDIR)/*.h)
OBJECTS  := $(SOURCES:$(SRCDIR)/%.c=$(OBJDIR)/%.o)

all: $(BINDIR)/$(TARGET)

debug: CFLAGS += -O0 -g
debug: all
release: CFLAGS += -O3
release: all

$(BINDIR)/$(TARGET): $(OBJECTS)
	$(LINKER) $(OBJECTS) $(LFLAGS) -o $@

$(OBJECTS): $(OBJDIR)/%.o : $(SRCDIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY: clean
clean:
	rm $(OBJECTS)
	rm $(BINDIR)/$(TARGET)
