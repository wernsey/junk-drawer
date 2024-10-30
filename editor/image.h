typedef struct {
	int w, h;
	int palcount;
	unsigned char palette[ 768 ];
	uint8_t pixels[0];
} image_t;

image_t *img_create(int w, int h);
image_t *loadpcx( char const* filename);

void blit_image(image_t *img, int dx, int dy, int dw, int dh, int sx, int sy, int sw, int sh);
