#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdint.h>
#include <assert.h>

#include "editor.h"
#include "image.h"

image_t *img_create(int w, int h) {
	image_t *img = malloc( (sizeof *img) + w * h);
	if(!img) {
		error("out of memory");
		return NULL;
	}
	img->w = w;
	img->h = h;
	return img;
}

/*
http://qzx.com/pc-gpe/pcx.txt
https://moddingwiki.shikadi.net/wiki/PCX_Format
*/
#pragma pack(push, 1)
struct PCX_Header {
	uint8_t manufacturer;
	uint8_t version;
	uint8_t encoding;
	uint8_t bpp;
	struct {
		uint16_t xmin, ymin, xmax, ymax;
	} window;
	uint16_t hdpi;
	unsigned short vdpi;
	uint8_t colormap[48];
	uint8_t reserved;
	uint8_t nplanes;
	uint16_t bytesPerLine;
	uint16_t paletteInfo;
	uint16_t hScreenSize;
	uint16_t vScreenSize;
	uint8_t filler[54];
};
#pragma pack(pop)
image_t *loadpcx( char const* filename ) {
	FILE *f = NULL;
	struct PCX_Header header;

    int w, h;
	
	info("Loading PCX `%s`", filename);

    f = fopen(filename, "rb");
	if(!f) {
		error("Couldn't open %s: %s", filename, strerror(errno));
		goto fail;
	}

	assert(sizeof(struct PCX_Header) == 128);

	if(fread(&header, 128, 1, f) != 1) {
		error("Couldn't read header %s", filename);
		goto fail;
	}

	w = header.window.xmax - header.window.xmin + 1;
	h = header.window.ymax - header.window.ymin + 1;

	if(header.bpp != 8 && header.nplanes != 1) {
		error("Unsupported PCX format %d:%d", header.bpp, header.nplanes);
		goto fail;
	}

	image_t *img = img_create(w, h);
	if(!img)
		goto fail;
	
	int i = 0;
	while(i < w * h) {
		uint8_t b = fgetc(f);
		if(b >= 192) {
			uint8_t c = fgetc(f);
			for(b &= 0x3F; b > 0; b--)
				img->pixels[i++] = c;
		} else
			img->pixels[i++] = b;
	}

	img->palcount = 256;
	fseek(f, -768, SEEK_END);
	fread(img->palette, 1, 768, f);
	
	fclose(f);

	return img;

fail:
	if(img)
		free(img);
	if(f)
		fclose(f);
	return NULL;
}

void blit_image(image_t *img, int dx, int dy, int dw, int dh, int sx, int sy, int sw, int sh) {
	int x, y, u, v;
	if(sw == 0) sw = img->w;
	if(sh == 0) sh = img->h;
	if(dw == 0) dw = sw;
	if(dh == 0) dh = sh;
	// TODO: if (sw == dw && sh == dh) special case
	
	v = sy;	
	int cy = 0;
	for(y = dy; y < dy + dh; y++) {
		if(y >= vheight) break;
		if(y >= 0) {
						
			u = sx;
			int cx = 0;
			for(x = dx; x < dx + dw; x++) {
				if(x >= vwidth) break;
				if(x >= 0) {
					uint8_t p = img->pixels[u + v * img->w] & 0x0F;
					uint8_t R = img->palette[p * 3 + 0],
							G = img->palette[p * 3 + 1],
							B = img->palette[p * 3 + 2];
					
						putpix(x, y, (R << 16) + (G << 8) + B);
				}
				cx = cx + sw;
				while(cx >= dw) {
					u = u + 1;
					cx = cx - dw;
				}
			}
			
		}		
		cy = cy + sh;
		while(cy >= dh) {
			v = v + 1;
			cy = cy - dh;
		}
	}	 
}
