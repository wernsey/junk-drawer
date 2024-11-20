CFLAGS= -I../common
LFLAGS= 

ifeq ($(BUILD),debug)
# Debug mode: Unoptimized and with debugging symbols
CFLAGS += -O0 -g
LFLAGS +=
else
# Release mode: Optimized and stripped of debugging info
CFLAGS += -O2 -DNDEBUG
LFLAGS += -s
endif

DOMSSON=domsson
TEST=test
ifeq ($(OS),Windows_NT)
	DOMSSON := $(DOMSSON).exe
	TEST := $(TEST).exe
endif

FONTS=cellphone.x.h  futuristic.x.h  oldschool.x.h

all: $(DOMSSON) $(TEST) $(FONTS)

$(DOMSSON) : domsson.o
	gcc -o $@ $^ $(LFLAGS)

$(TEST) : test.o
	gcc -o $@ $^ $(LFLAGS)

%.x.h: %.png | $(DOMSSON)
	./$(DOMSSON) $^ > $@  

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

domsson.o: domsson.c ../common/bmph.h ../common/stb_image.h
test.o: test.c ../common/bmph.h oldschool.x.h futuristic.x.h cellphone.x.h


clean:
	-rm -rf $(DOMSSON) $(TEST)
	-rm -rf *.o
	-rm -rf *.x.h
	-rm -rf test.gif