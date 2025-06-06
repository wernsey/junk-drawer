EXECUTABLE= dungeon

# Add your source files here:
SOURCES=dungeon.c main.c poisson.c

CFLAGS= -Wall
LDFLAGS=

ifeq ($(OS),Windows_NT)
	EXECUTABLE := $(EXECUTABLE).exe
else
	UNAME_S := $(shell uname -s)
	ifeq ($(UNAME_S),Darwin)
		LDFLAGS+=
	else
		LDFLAGS+=
	endif
endif

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

all: $(EXECUTABLE)

debug:
	make BUILD=debug

$(EXECUTABLE): $(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@ 

.c.o:
	$(CC) $(CFLAGS) -c $< -o $@

dungeon.o: dungeon.c
main.o: main.c dungeon.c poisson.c
poisson.o: poisson.c

.PHONY : clean

clean:
	-rm -f *.o $(EXECUTABLE)
	-rm -f *~ *.svg
