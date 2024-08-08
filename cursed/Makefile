CC=gcc
CFLAGS= -c -Wall 
LDFLAGS=

# For building with PDCurses in MinGW:
CURSES_PATH=/c/libs/PDCurses-3.9/
CFLAGS += -I $(CURSES_PATH)
CURSES_LIB = $(CURSES_PATH)/wincon/pdcurses.a

# Add your source files here:
SOURCES=cursed.c map.c
OBJECTS=$(SOURCES:.c=.o)
EXECUTABLE=cursed

ifeq ($(BUILD),debug)
# Debug
CFLAGS += -O0 -g
LDFLAGS +=
else
# Release mode
CFLAGS += -O2 -DNDEBUG
LDFLAGS += -s
endif

all: $(EXECUTABLE)

debug:
	make BUILD=debug

$(EXECUTABLE): $(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) $(CURSES_LIB) -o $@ 
	
.c.o:
	$(CC) $(CFLAGS) $< -o $@

# Add header dependencies here, like so:
cursed.o : map.h
map.o : map.h

.PHONY : clean

clean: wipe
	-rm -f *.o $(EXECUTABLE) $(EXECUTABLE).exe

wipe:
	-rm -f *~ *.wip
