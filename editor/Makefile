EXECUTABLE=editor

CFLAGS= -c -Wall -g -O0

# Add your source files here:
SOURCES=editor.c image.c

ifeq ($(OS),Windows_NT)
	SOURCES += ../snippets/getopt.c
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

editor.o: editor.c editor.h fenster.h image.h stb_tilemap_editor.h
image.o: image.c editor.h image.h

editor.o: CFLAGS += -Wno-unused-function -Wno-unused-variable \
                    -Wno-misleading-indentation -Wno-comment \
                    -Wno-unused-but-set-variable

.PHONY : clean deps

deps:
	@ $(CC) -MM $(SOURCES)

clean:
	-rm -f *.o $(EXECUTABLE)
	-rm -f *~ *.log *.ppm
