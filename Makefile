# Makefile for macOS compatibility

# Compiler and flags
CC = cc
CFLAGS = -Wall -O3 -D_DARWIN_C_SOURCE -Wno-deprecated-declarations

# Source directories
SRCDIR = ./src

# Source files
SOURCES = $(SRCDIR)/basics.c $(SRCDIR)/bitrankw32int.c $(SRCDIR)/dacs.c $(SRCDIR)/test.c

# Object files
OBJECTS = $(SOURCES:.c=.o)

# Executable
TARGET = test

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $^ -lm

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(SRCDIR)/*.o $(TARGET)

run: $(TARGET)
	./$(TARGET) $(ARGS)