#include <stdio.h> 
#include <string.h> 
#include <errno.h> 
#include <stdarg.h> 
#include <assert.h> 

#if !defined(_WIN32)
#  include <unistd.h>
#else
#  include "getopt.h"
#endif

#include "editor.h"

#include "fenster.h"

#include "image.h"

#define CSV_IMPLEMENTATION
#include "csvstrm.h"

#define STB_TILEMAP_EDITOR_IMPLEMENTATION
void get_spacing(int *spacing_x, int *spacing_y, int *palette_spacing_x, int *palette_spacing_y);

#define WIDTH  800
#define HEIGHT 560

#define TILE_WIDTH  16
#define TILE_HEIGHT 16

#define KEY_UP		17
#define KEY_DOWN	18
#define KEY_RIGHT	19
#define KEY_LEFT	20

/* I wanted to go with names like MOD_ALT and MOD_SHIFT but 
it turns out some of them are already defined in `winuser.h` */
#define KEYMOD_CTRL		0x1
#define KEYMOD_SHIFT	0x2
#define KEYMOD_ALT		0x4
#define KEYMOD_META		0x8

int lo_res = 0, vwidth, vheight;

static uint32_t *screen;

int zoom = 8, pal_zoom = 8;

typedef struct {
	unsigned int row, col;
} map_tile_t;

#define MAX_TILES	256
map_tile_t tiles[MAX_TILES];
int num_tiles;

static FILE *outfile;

static struct fenster *F;

image_t *walls, *icons;

const char *layer_names[] =  {"Floor",
								"Walls",
								"Decals",
								"Upper",
								"Entities"
							};
#define NUM_LAYERS (sizeof layer_names/ sizeof layer_names[0])

static int layer_index(const char *layer_name) {
	for(int i = 0; i < NUM_LAYERS; i++) {
		if(!strcasecmp(layer_names[i], layer_name)) {
			return i;
		}
	}
	return -1;
}

void putpix(int x, int y, unsigned int color) {
	screen[y * vwidth + x] = color;
}

void STBTE_DRAW_RECT(int x0, int y0, int x1, int y1, unsigned int color) {
	for(int y = y0; y < y1; y++) {	
		if(y < 0) continue;
		if(y >= vheight) break;
		for(int x = x0; x < x1; x++) {
			if(x < 0) continue;
			if(x >= vwidth) break;
			screen[y * vwidth + x] = color;
		}
	}
}

void STBTE_DRAW_TILE(int x0, int y0, unsigned short id, int highlight, float *data) {	
	unsigned int color = 0;
	int spacing_x, spacing_y, palette_spacing_x, palette_spacing_y;
	int w, h;
	get_spacing(&spacing_x, &spacing_y, &palette_spacing_x, &palette_spacing_y);
	if(data) {
		w = spacing_x;
		h = spacing_y;
	} else  {
		w = palette_spacing_x;
		h = palette_spacing_y;
	}
	
	if(id == 0 || id > num_tiles) {
		/* TODO id > num_tiles when pasting. what does it mean? */
		STBTE_DRAW_RECT(x0, y0, x0 + w, y0 + h, !id ? 0x000000 : 0x00FF00);
		return;
	}	
	
	map_tile_t *tile = &tiles[id - 1]; 
	
	blit_image(walls, x0, y0, w, h, tile->col * 32, tile->row * 32, 32, 32);
	
	if(highlight == -1) { // STBTE_drawmode_deemphasize = -1,
		for(int y = y0; y < y0 + h; y++)
			for(int x = x0; x < x0 + w; x++) 
				if((x + y) & 1) 
					putpix(x,y, 0xFFFFFF);
		
	} else if(highlight == 1) { // STBTE_drawmode_emphasize   =  1,
		//STBTE_DRAW_RECT(x0, y0, x0 + w, y0 + h, 0x555555);
		for(int x = x0; x < x0+w; x++) {
			putpix(x, y0, 0x00FF00);
			putpix(x, y0+h, 0x00FF00);
		}
		for(int y = y0; y < y0+h; y++) {
			putpix(x0, y, 0x00FF00);
			putpix(x0+w, y, 0x00FF00);
		}
	}
		
}

static int stbte_prop_type(int n, short *tiledata, float *params);
static char *stbte_prop_name(int n, short *tiledata, float *params);
static float stbte_prop_min(int n, short *tiledata, float *params);
static float stbte_prop_max(int n, short *tiledata, float *params);

#define STBTE_PROP_TYPE(n, t, p) stbte_prop_type(n,t,p)
#define STBTE_PROP_NAME(n,t,p) stbte_prop_name(n,t,p)
#define STBTE_PROP_MIN(n,t,p) stbte_prop_min(n,t,p)
#define STBTE_PROP_MAX(n,t,p) stbte_prop_max(n,t,p)

#include "stb_tilemap_editor.h"

static int stbte_prop_type(int n, short *tiledata, float *params) {
	switch(n) {
		case 0: return STBTE_PROP_int;
	}
	return 0;
}
static char *stbte_prop_name(int n, short *tiledata, float *params) {
	switch(n) {
		case 0: return "tag";
	}
	return "";
}

static float stbte_prop_min(int n, short *tiledata, float *params) {
	switch(n) {
		case 0: return 0;
	}
	return 0;
}
static float stbte_prop_max(int n, short *tiledata, float *params) {
	switch(n) {
		case 0: return 10;
	}
	return 0;
}

stbte_tilemap *Map;

void get_spacing(int *spacing_x, int *spacing_y, int *palette_spacing_x, int *palette_spacing_y)
{
   if(spacing_x) *spacing_x = Map->spacing_x;
   if(spacing_y) *spacing_y = Map->spacing_y;
   if(palette_spacing_x) *palette_spacing_x = Map->palette_spacing_x;
   if(palette_spacing_y) *palette_spacing_y = Map->palette_spacing_y;
}

int load_tiles(const char *filename) {
	CsvContext csv;
	FILE *f = fopen(filename, "r");
	if(!f) {
		error("unable to open '%s': %s", filename, strerror(errno));
		return 0;
	}
	csv_context_file(&csv, f);
	
	/* Slurp the header row */
	if(!csv_read_record(&csv)) {
		error("Empty CSV file '%s'", filename);
		return 0;
	}
	
	num_tiles = 0;
	while(csv_read_record(&csv)) {
		if(csv_count(&csv) < 4) {
			error("%s: not enough columns at record %d",filename, csv_records);
			return 0;
		}
		if(num_tiles == MAX_TILES) {
			error("%s: too many tiles (max %d)", filename, MAX_TILES);
			return 0;
		}
		map_tile_t *tile = &tiles[num_tiles];
		tile->row = strtoul(csv_field(&csv, 0), NULL, 0);
		tile->col = strtoul(csv_field(&csv, 1), NULL, 0);
		unsigned int layers = strtoul(csv_field(&csv, 2), NULL, 0);
		const char *category = csv_field(&csv, 3);
		int i = layer_index(category);
		if(i < 0) 
			category = NULL;
		else
			category = layer_names[i];
		
		stbte_define_tile(Map, num_tiles + 1, layers, category);
		num_tiles++;
	}
	
	return 1;
}

int stbte_init() {
	
	Map = stbte_create_map(20, 20, NUM_LAYERS, zoom, zoom, MAX_TILES + 1);

	stbte_set_display(0, 0, vwidth, vheight);

	stbte_set_background_tile(Map, 0);
	 
	stbte_set_sidewidths(105, 5);
	
	stbte_set_spacing(Map, zoom, zoom, pal_zoom, pal_zoom);
	
	for(int i = 0; i < NUM_LAYERS; i++)
		stbte_set_layername(Map, i, layer_names[i]);

	// NOTE TO SELF: There is a STBTE_EMPTY defined as -1...

	// TILE ID 0 is meant for the empty/undefined tile
	stbte_define_tile(Map, 0, 0xFF, NULL);
	stbte_set_background_tile(Map, 0);

	if(!load_tiles("tiles.csv")) return 0;
	
	// scroll the map a bit
	if(lo_res) {
		Map->scroll_x -= 17 * Map->spacing_x;
		Map->scroll_y -= 4 * Map->spacing_y;
	} else {
		Map->scroll_x -= 4 * Map->spacing_x;
		Map->scroll_y -= 1 * Map->spacing_y;
	}
	return 1;
}

void save_map(const char *filename) {
	info("Save map: %s", filename);
}

void load_map(const char *filename) {
	info("Load map: %s", filename);
}

int stbte_update(double elapsedSeconds, const char *filename) {
	stbte_tick(Map, elapsedSeconds);
	
	/* Ctrl+click needs to substitute for right click */
	int mx = F->x, my = F->y;
	if(lo_res) {
		mx /= 2;
		my /= 2;
	}
	stbte_mouse_move(Map, mx, my, F->mod & 2, F->mod & 1);
	stbte_mouse_button(Map, mx, my, (F->mod & KEYMOD_CTRL) ? 1 : 0, 
		F->mouse, F->mod & KEYMOD_SHIFT, F->mod & KEYMOD_CTRL);

	if(F->keys['S'] &&  (F->mod & KEYMOD_CTRL)) {
		save_map(filename);
		F->keys['S'] = 0;
	}
	if(F->keys['R'] &&  (F->mod & KEYMOD_CTRL)) {
		load_map(filename);
		F->keys['R'] = 0;
	}
	if(F->keys['N'] &&  (F->mod & KEYMOD_CTRL)) {
		stbte_clear_map(Map);
		F->keys['N'] = 0;
	}
	if(F->keys['Q'] &&  (F->mod & KEYMOD_CTRL)) {
		F->keys['Q'] = 0;
		return 0;
	}

	struct {
		uint8_t key;
		int mod;
		enum stbte_action action;
	} shortcuts[] = {
		{'S', 0, STBTE_tool_select},
		{'B', 0, STBTE_tool_brush},
		{'E', 0, STBTE_tool_erase},
		{'R', 0, STBTE_tool_rectangle},
		{'D', 0, STBTE_tool_eyedropper},
		{'L', KEYMOD_CTRL, STBTE_act_toggle_links},
		{'L', 0, STBTE_tool_link},
		{'G', KEYMOD_CTRL, STBTE_act_toggle_grid},
		{'Z', KEYMOD_CTRL | KEYMOD_SHIFT, STBTE_act_redo},
		{'Z', KEYMOD_CTRL, STBTE_act_undo},
		{'X', KEYMOD_CTRL, STBTE_act_cut},
		{'C', KEYMOD_CTRL, STBTE_act_copy},
		{'V', KEYMOD_CTRL, STBTE_act_paste},
		{KEY_UP, 0, STBTE_scroll_up},
		{KEY_DOWN, 0, STBTE_scroll_down},
		{KEY_LEFT, 0, STBTE_scroll_left},
		{KEY_RIGHT, 0, STBTE_scroll_right},
		{0,0,0}
	};
	
	for(int i = 0; shortcuts[i].key; i++) {
		if(F->keys[shortcuts[i].key]) {
			if(!shortcuts[i].mod || ((shortcuts[i].mod & F->mod) == shortcuts[i].mod)) {
				stbte_action(Map, shortcuts[i].action); 
				F->keys[shortcuts[i].key] = 0;
				break;
			}
		}
	}
	
	// Use '+' and '-' keys for zooming.
	// TODO: Is there a good way to keep the screen centered?
	if(F->keys['='] && zoom < 64) {
		zoom <<= 1;
		stbte_set_spacing(Map, zoom, zoom, pal_zoom, pal_zoom);
		F->keys['='] = 0;
	}
	if(F->keys['-'] && zoom > 8) {
		zoom >>= 1;
		stbte_set_spacing(Map, zoom, zoom, pal_zoom, pal_zoom);
		F->keys['-'] = 0;
	}
	
	stbte_draw(Map);
	
	return 1;
}

static uint32_t buf[WIDTH * HEIGHT];

static void usage(const char *name, FILE *f) {
	fprintf(f, "usage: %s [options] <filename>\n", name);
	fprintf(f, "where [options] can be:\n");
	fprintf(f, "   -h           : hi-res mode\n");
}

int run(int argc, char *argv[]) {
	const char *filename = "temp.map";
	lo_res = 1;
	int c;
	while ((c = getopt(argc, argv, "h")) != -1) {
		switch (c) {
			case 'h': lo_res = 0; break;
			default: usage(argv[0], stderr); return 1;
		}
	}
	if(optind < argc) {
		filename = argv[optind];
	}

	if(lo_res) {
		vwidth = WIDTH / 2; 
		vheight = HEIGHT / 2;
		zoom = 8;
		pal_zoom = 16;
	} else {
		vwidth = WIDTH;
		vheight = HEIGHT;
		zoom = 32;
		pal_zoom = 32;
	}
	
	screen = malloc(vwidth * vheight * sizeof *screen);
		
	struct fenster f = {
		.title = "Editor",
		.width = WIDTH, .height = HEIGHT,
		.buf = buf
	};
	
	F = &f; 
	
	memset(buf, 0, sizeof buf);
		
#if 1
	outfile = fopen("editor.log", "w");
#else
	outfile = stdout;
#endif
		
	if(!stbte_init()) return 1;
	
	walls = loadpcx("walls.pcx");
	if(!walls) {
		error("unable to load walls.pcx");
		return 1;
	}
	icons = loadpcx("icons.pcx");
	if(!icons) {
		error("unable to load icons.pcx");
		return 1;
	}
		
	fenster_open(&f);
		
	int64_t deltat = 0ll;
	while(fenster_loop(&f) == 0) {
		int64_t start = fenster_time();
		
		if(!stbte_update((double)deltat/1000.0, filename)) break;
		
		// Escape quits - for now.
		if(F->keys[27]) break;
		
		blit_image(icons, vwidth-16, vheight-16, 16, 16, 0, 0, 8, 8);
		
		int v = 0;	
		int cy = 0;
		for(int y = 0; y < f.height; y++) {
							
			int u = 0;
			int cx = 0;
			for(int x = 0; x < f.width; x++) {
				
				fenster_pixel(F, x, y) = screen[v * vwidth + u];
				
				cx = cx + vwidth;
				while(cx >= f.width) {
					u = u + 1;
					cx = cx - f.width;
				}
			}
			
			cy = cy + vheight;
			while(cy >= f.height) {
				v = v + 1;
				cy = cy - f.height;
			}
		}	 
		
		int64_t time = fenster_time() - start;
		if(time < 1000 / 30) 
			fenster_sleep((1000 / 30) - time);
		deltat = fenster_time() - start;		
	}
	
	fenster_close(&f);
	
	if(outfile != stdout)
		fclose(outfile);
	
	return 0;
}

#ifdef _WIN32
/*
Alternative to CommandLineToArgvW().
I used a compiler where shellapi.h was not available,
so this function breaks it down according to the last set of rules in
http://i1.blogs.msdn.com/b/oldnewthing/archive/2010/09/17/10063629.aspx
*/
static int split_cmd_line(char *cmdl, char *argv[], int max) {

    int argc = 0;
    char *p = cmdl, *q = p, *arg = p;
    int state = 1;
    while(state) {
        switch(state) {
            case 1:
                if(argc == max) return argc;
                if(!*p) {
                    state = 0;
                } else if(isspace(*p)) {
                    *q++ = *p++;
                } else if(*p == '\"') {
                    state = 2;
                    *q++ = *p++;
                    arg = q;
                } else {
                    state = 3;
                    arg = q;
                    *q++ = *p++;
                }
            break;
            case 2:
                if(!*p) {
                    argv[argc++] = arg;
                    *q++ = '\0';
                    state = 0;
                } else if(*p == '\"') {
                    if(p[1] == '\"') {
                        state = 2;
                        *q++ = *p;
                        p+=2;
                    } else {
                        state = 1;
                        argv[argc++] = arg;
                        *q++ = '\0';
                        p++;
                    }
                } else {
                    *q++ = *p++;
                }
            break;
            case 3:
                if(!*p) {
                    state = 0;
                    argv[argc++] = arg;
                    *q++ = '\0';
                } else if(isspace(*p)) {
                    state = 1;
                    argv[argc++] = arg;
                    *q++ = '\0';
                    p++;
                } else {
                    *q++ = *p++;
                }
            break;
        }
    }
    return argc;
}
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR pCmdLine, int nCmdShow) {
	(void)hInstance, (void)hPrevInstance, (void)pCmdLine, (void)nCmdShow;
#define MAX_ARGS    32	
	char *argv[MAX_ARGS];
    LPTSTR cmdl = _strdup(GetCommandLine());
    int argc = split_cmd_line(cmdl, argv, MAX_ARGS);
	
	int rv = run(argc, argv);
	
	free(cmdl);
	
	return rv;
}
#else
int main(int argc, char *argv[]) {
	return run(argc, argv);
}
#endif

void error(const char *fmt, ...) {
	char buffer[128]; 
	va_list arg;					
    va_start(arg, fmt);		
    vsnprintf(buffer, sizeof buffer, fmt, arg);	
    va_end(arg);					
    fprintf(outfile, "error: %s\n", buffer);
}

void info(const char *fmt, ...) {
	char buffer[128]; 
	va_list arg;					
    va_start(arg, fmt);			
    vsnprintf(buffer, sizeof buffer, fmt, arg);
    va_end(arg);					
    fprintf(outfile, "info: %s\n", buffer);
}




