EXECUTABLE= basic

# Add your source files here:
SOURCES=main.c sbasic.c listfuns.c getopt.c

CFLAGS= -Wall -std=c89
LDFLAGS=

# Test programs for the support libs
PPDB= ppdb
SEQIO= seqio

ifeq ($(OS),Windows_NT)
	EXECUTABLE := $(EXECUTABLE).exe
	PPDB := $(PPDB).exe
	SEQIO := $(SEQIO).exe
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

all: $(EXECUTABLE) $(PPDB) $(SEQIO) docs

debug:
	make BUILD=debug

$(EXECUTABLE): $(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@ 

.c.o:
	$(CC) $(CFLAGS) -c $< -o $@

main.o: main.c getopt.h sbasic.h ppdb.h seqio.h
sbasic.o: sbasic.c sbasic.h
listfuns.o: listfuns.c sbasic.h
getopt.o: getopt.c

$(PPDB): ppdbmain.c ppdb.h
	$(CC) $(CFLAGS) $< -o $@
	
$(SEQIO): seqio.h
	$(CC) $(CFLAGS) -DSEQIO_TEST -o $@ -xc $<

docs: docs.md.html

docs.md.html : sbasic.h sbasic.c listfuns.c main.c
	@echo Generating docs...
	@echo '<meta charset="utf-8">' > $@
	@awk '/\/\*\*/{sub(/\/\*\*[[:space:]]*/,"");incomment=1} incomment && /\*\//{incomment=0;sub(/[[:space:]]*\*\/.*/,"");print} incomment && /^[[:space:]]*\*/{sub(/^[[:space:]]*\*[[:space:]]?/,""); print}' $^ >> $@
	@echo '<!-- Markdeep: --><style class="fallback">body{visibility:hidden;white-space:pre;font-family:monospace}</style>' >> $@
	@echo '<script>markdeepOptions={tocStyle:"auto",definitionStyle:"long"};</script>' >> $@
	@echo '<script src="https://morgan3d.github.io/markdeep/latest/markdeep.min.js" charset="utf-8"></script>' >> $@
	@echo '<script>window.alreadyProcessedMarkdeep||(document.body.style.visibility="visible")</script>' >> $@

shar: sbasic.shar

sbasic.shar: README.md Makefile *.c *.h test/*.bas
	shar -Cgzip $^ > $@

.PHONY : clean deps shar

deps:
	@ $(CC) -MM $(SOURCES)

clean:
	-rm -f *.o $(EXECUTABLE) docs.md.html
	-rm -f  $(PPDB) $(SEQIO) sbasic.shar
	-rm -f *~ *.db seqio.out data.txt
