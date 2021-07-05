TARGET = i8080e

CC = gcc
CFLAGS = -Wall -Werror -Wextra

LINKER = gcc
LFLAGS = -Wall -Werror -Wextra
LFLAGS += `sdl2-config --libs` -lSDL2_mixer -lSDL2_image -lSDL2_ttf -lm

#CXXFLAGS += `sdl2-config --cflags`
#CXXFLAGS += -g -lefence

SRCDIR   = src
OBJDIR   = obj
BINDIR   = bin

SOURCES  := $(wildcard $(SRCDIR)/*.c)
INCLUDES := $(wildcard $(SRCDIR)/*.h)
OBJECTS  := $(SOURCES:$(SRCDIR)/%.c=$(OBJDIR)/%.o)

default: debug
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
