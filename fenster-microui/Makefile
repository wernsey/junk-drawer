EXECUTABLE=ui

CFLAGS= -c -Wall -g -O0

# Add your source files here:
SOURCES = microui.c demo.c fenmui.c

ifeq ($(OS),Windows_NT)
	LDFLAGS = -lgdi32
	EXECUTABLE := $(EXECUTABLE).exe
else
	UNAME_S := $(shell uname -s)
	ifeq ($(UNAME_S),Darwin)
		LDFLAGS = -framework Cocoa
	else
		LDFLAGS = -lX11 -lm
	endif
endif

OBJECTS=$(SOURCES:.c=.o)


all: $(EXECUTABLE)

debug:
	make BUILD=debug

$(EXECUTABLE): $(OBJECTS)
	$(CC) -o $@ $(OBJECTS)  $(LDFLAGS)

.c.o:
	$(CC) $(CFLAGS) $< -o $@

# use `make deps` to refresh these dependencies:
microui.o: microui.c microui.h
demo.o: demo.c microui.h fenster.h fenmui.h
fenmui.o: fenmui.c microui.h atlas.inl fenster.h fenmui.h Cushion.h

.PHONY : clean deps

deps:
	@ $(CC) -MM $(SOURCES)

clean:
	-rm -f *.o $(EXECUTABLE)
