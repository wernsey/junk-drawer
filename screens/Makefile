CFLAGS= -DNO_FONTS -c -I../common
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

SOURCES= screens.c

OBJECTS=$(SOURCES:.c=.o)

screens : $(OBJECTS) screens.res
	gcc -o $@ $^ $(LFLAGS)

screens.res : screens.rc little.ico enable.ico disable.ico
	windres $< -O coff -o $@

screens.o : screens.c screens.h

clean:
	-rm -rf screens.exe xtract.exe
	-rm -rf *.o
	-rm -rf *.res