
CFLAGS= -c -Wall -g -O0
LDFLAGS= -lhpdf


# Add your source files here:
SOURCES=modder.c
OBJECTS=$(SOURCES:.c=.o)
EXECUTABLE=modder
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

modder.o: modder.c

.PHONY : clean

clean:
	-rm -f $(EXECUTABLE)
	-rm -f *.o
	-rm -f *~ *.bak *.log *.ppm
