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
static const unsigned char fontbytes[] = {
#include "oldschool.x.h"
};

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

You might be curious as to why I'm bothering with this when
`../common/bmp.h` already has a cromulent built-in bitmap font.
Apparently I'm cursed to write the same computer programs over and
over again.