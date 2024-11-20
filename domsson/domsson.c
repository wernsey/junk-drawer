#include <stdio.h>

/*
OpenGameArt user domsson has a couple of 5x7 bitmap fonts:

* https://opengameart.org/content/ascii-bitmap-font-cellphone
* https://opengameart.org/content/ascii-bitmap-font-futuristic
* https://opengameart.org/content/ascii-bitmap-font-oldschool

This program just converts them to C code.

`bmph.h` comes from here: https://github.com/wernsey/bitmap/
You'll also need stb_image.h

For example: If  you assign the bytes to a unsigned char array
called `fontbytes` and have a function called `putpixel(x,y)` 
that can draw a pixel at (x,y) on the screen, then to draw a
character from this font, you can use a function like this:

```
void printchar(int x, int y, int c) {
	if(c < ' ' || c >= 128) c = '*';

	int xs = x, ye = y + 7;
	const uint8_t *p = &fontbytes[ (c - 0x20) * 7 ];
	for(; y < ye; y++, p++) {
		if(y < clip.y) continue;
		if(y >= clip.y + clip.h) break;
		for(int i = 0x10, x = xs; i; i >>= 1, x++) {
			if(x < clip.x) continue;
			if(x >= clip.x + clip.w) break;
			if(i & *p) {
				putpixel(x, y);
			}
		}
	}
}
```
*/

#define USESTB
#define BMPH_IMPLEMENTATION
#include "bmph.h"

int main(int argc, char *argv[]) {
	if(argc < 2) {
		fprintf(stderr, "usage: %s font.png\n", argv[0]);
		return 1;
	}

	Bitmap *b = bm_load(argv[1]);
	if(!b) {
		fprintf(stderr, "Couldn't open %s\n", argv[1]);
		return 1;
	}
		
	int chars_per_row = '2' - ' ';
	for(int c = ' '; c <= '~'; c++) {		
		int row = (c - ' ') / chars_per_row;
		int col = (c - ' ') % chars_per_row;
		int x = col * 7 + 1;
		int y = row * 9 + 1;
		for(int j = 0; j < 7; j++) {
			unsigned char byte = 0;
			int mask = 1 << 4;
			for(int i = 0; i < 5; i++) {
				if(bm_get(b, x + i, y + j) & 0x00FFFFFF) 
					byte |= mask;
				mask >>= 1;
			}
			printf("0x%02X, ", byte);			
		}
		printf(" /* '%c' */\n", c);
	}
	
	bm_free(b);
	
	return 0;
}