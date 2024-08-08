#CC=C:\Tools\tcc\tcc
CC=gcc

BUILD=debug

CFLAGS = -c -Wall
LDFLAGS =

# Add your source files here:
SOURCES=bas.c ast.c hash.c compile.c

OBJECTS=$(SOURCES:.c=.o)

DISTFILE=mybas.zip

ifeq ($(BUILD),debug)
# Debug
CFLAGS += -O0 -g
LDFLAGS +=
else
# Release mode
CFLAGS += -O2 -DNDEBUG
LDFLAGS += -s
endif

all: bas.exe

debug:
	make BUILD=debug

bas.exe: $(OBJECTS)
	$(CC) $^ $(LDFLAGS) -o $@

.c.o:
	$(CC) $(CFLAGS) $< -o $@

# Add header dependencies here
bas.o : bas.c bas.h ast.h hash.h
ast.o : ast.c ast.h bas.h
hash.o : hash.c hash.h
compile.o : compile.c bas.h ast.h hash.h

.PHONY : clean

clean:
	-rm -f *.exe $(DISTFILE)
	-rm -f *.o
	-rm -f *.b.c *.bas.c *~

dist: clean
	zip $(DISTFILE) *