CFLAGS= -DNO_FONTS -c
LFLAGS= -mwindows -lwtsapi32

ifeq ($(BUILD),debug)
# Debug mode: Unoptimized and with debugging symbols
CFLAGS += -O0 -g
LFLAGS +=
else
# Release mode: Optimized and stripped of debugging info
CFLAGS += -O2 -DNDEBUG
LFLAGS += -s
endif

all: screens

SOURCES= screens.c bmp.c

OBJECTS=$(SOURCES:.c=.o)

screens : $(OBJECTS) screens.res
	gcc -o $@ $^ $(LFLAGS)

screens.res : screens.rc little.ico enable.ico disable.ico
	windres $< -O coff -o $@

screens.o : screens.c bmp.h screens.h
bmp.o : bmp.c bmp.h

clean:
	-rm -rf screens.exe xtract.exe
	-rm -rf *.o
	-rm -rf *.res