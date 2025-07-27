
CFLAGS= -c -Wall -g -O0
LDFLAGS= -lhpdf


# Add your source files here:
SOURCES=zine.c
OBJECTS=$(SOURCES:.c=.o)
EXECUTABLE=zine
ifeq ($(OS),Windows_NT)
	EXECUTABLE := $(EXECUTABLE).exe
else
endif

all: $(EXECUTABLE)

debug:
	make BUILD=debug

$(EXECUTABLE): $(OBJECTS)
	$(CC) -o $@ $^ $(LDFLAGS)

.c.o:
	$(CC) $(CFLAGS) $< -o $@

zine.o: zine.c

.PHONY : clean

clean:
	-rm -f $(EXECUTABLE)
	-rm -f *.o
	-rm -f *~ *.bak *.log *.ppm
