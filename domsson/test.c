#include <stdio.h>

#define BMPH_IMPLEMENTATION
#include "bmph.h"

static const unsigned char oldschool[] = {
#include "oldschool.x.h"
};
static const unsigned char futuristic[] = {
#include "futuristic.x.h"
};
static const unsigned char cellphone[] = {
#include "cellphone.x.h"
};

static Bitmap *b;

static unsigned int color = 0xFFFFFF;

const unsigned char *font = oldschool;

static void printchar(int x, int y, int c) {
	if(c < ' ' || c >= 128) c = '*';

	BmRect clip = bm_get_clip(b);
	int xs = x, ye = y + 7;
	const uint8_t *p = &font[ (c - 0x20) * 7 ];
	for(; y < ye; y++, p++) {
		if(y < clip.y0) continue;
		if(y >= clip.y1) break;
		for(int i = 0x10, x = xs; i; i >>= 1, x++) {
			if(x < clip.x0) continue;
			if(x >= clip.x1) break;
			if(i & *p) {
				bm_set(b, x, y, color);
			}
		}
	}
}

void printstr(int x, int y, const char *s) {
	int xs = x;
	for(; *s; s++) {
		if(*s == '\n') {
			x = xs;
			y += 8;
		} else if(*s == '\r') {
			x = xs;
		} else if(*s == '\t') {
			x += 4 * 5;
		} else {
			printchar(x, y, *s);
			x += 6;
		}
	}
}

int main(int argc, char *argv[]) {
	b = bm_create(100, 100);
	printstr(10, 10, "Hello World!");
	font = futuristic;
	printstr(10, 20, "Hello World!");
	font = cellphone;
	printstr(10, 30, "Hello World!");
	bm_save(b, "test.gif");
	return 0;
}