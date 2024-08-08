CC=gcc

# Run `make debug` for a debug build
# BUILD=debug

CFLAGS = -c -Wall
LDFLAGS = -lm

# Add your source files here:
SOURCES=test.c

EXECUTABLE=picol

ifeq ($(BUILD),debug)
# Debug
CFLAGS += -O0 -g
LDFLAGS +=
else
# Release mode
CFLAGS += -O2 -DNDEBUG
LDFLAGS += -s
endif

OBJECTS=$(SOURCES:.c=.o)

ifeq ($(OS),Windows_NT)
  EXECUTABLE:=$(EXECUTABLE).exe
endif

all: $(EXECUTABLE)

debug:
	make BUILD=debug

$(EXECUTABLE): $(OBJECTS)
	$(CC) $^ $(LDFLAGS) -o $@

.c.o:
	$(CC) $(CFLAGS) $< -o $@

# Add header dependencies here
test.o: test.c picol.h

.PHONY : clean

clean:
	-rm -f $(EXECUTABLE)
	-rm -f *.o
