#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// https://github.com/rxi/microui
#include "microui.h"
#include "atlas.inl"

// https://github.com/zserge/fenster
#define FENSTER_HEADER
#include "fenster.h"

#include "fenmui.h"

#ifndef BUILTIN_FONT
#  define BUILTIN_FONT 1
#endif

#if !BUILTIN_FONT
// https://damieng.com/typography/zx-origins/cushion/
#include "Cushion.h"
#define TEXT_W	7
static const uint8_t *font = FONT_CUSHION_BITMAP;
#endif

static struct fenster *F;
static mu_Context *MC;

void r_clear(mu_Color color) {
	uint32_t c = (color.r << 16) + (color.g << 8) + (color.b);
	for(int y = 0; y < F->height; y++)
		for(int x = 0; x < F->width; x++)
			fenster_pixel(F, x, y) = c;
}

static mu_Rect clip;

void r_set_clip_rect(mu_Rect rect) {
	// Somehow the rect gets set a bit larger than needs be
	if(rect.w > F->width) rect.w = F->width;
	if(rect.h > F->height) rect.h = F->height;
	clip = rect;	
}

void r_draw_rect(mu_Rect rect, mu_Color color) {
	uint32_t c = (color.r << 16) + (color.g << 8) + (color.b);
	for(int y = rect.y; y < rect.y + rect.h; y++) {
		if(y < clip.y) continue;
		if(y >= clip.y + clip.h) break;
		for(int x = rect.x; x < rect.x + rect.w; x++) {			
			if(x < clip.x) continue;
			if(x >= clip.x + clip.w) break;
			fenster_pixel(F, x, y) = c;
		}
	}
}

static uint32_t c_lerp(uint32_t color1, uint32_t color2, int t) {
	if(t < 0) t = 0;
	if(t > 255) t = 255;
	int r1 = (color1 >> 16) & 0xFF, r2 = (color2 >> 16) & 0xFF;
	int g1 = (color1 >> 8) & 0xFF, g2 = (color2 >> 8) & 0xFF;
	int b1 = (color1 >> 0) & 0xFF, b2 = (color2 >> 0) & 0xFF;
	
	r1 = (r1 + (r2 - r1) * t / 255) & 0xFF;
	g1 = (g1 + (g2 - g1) * t / 255) & 0xFF;
	b1 = (b1 + (b2 - b1) * t / 255) & 0xFF;
	return (r1 << 16) + (g1 << 8) + b1;
}

static void draw_quad(mu_Rect src, mu_Rect rect, mu_Color color) {
	uint32_t col = (color.r << 16) + ( color.g << 8) + ( color.b );
	
	int x = rect.x + (rect.w - src.w) / 2;
	int y = rect.y + (rect.h - src.h) / 2;
	rect = mu_rect(x, y, src.w, src.h);
		
	int v = src.y;	
	int cy = 0;
	for(int y = rect.y; y < rect.y + rect.h; y++) {
		if(y >= clip.y + clip.h) break;
		if(y >= clip.y) {
						
			int u = src.x;
			int cx = 0;
			for(int x = rect.x; x < rect.x + rect.w; x++) {
				if(x >= clip.x + clip.w) break;
				if(x >= clip.x) {
					uint8_t p = atlas_texture[u + v * ATLAS_WIDTH];					
					fenster_pixel(F, x, y) = c_lerp(fenster_pixel(F, x, y), col, p);
				}
				cx = cx + src.w;
				while(cx >= rect.w) {
					u = u + 1;
					cx = cx - rect.w;
				}
			}
			
		}		
		cy = cy + src.h;
		while(cy >= rect.h) {
			v = v + 1;
			cy = cy - rect.h;
		}
	}	 
}

void r_draw_icon(int id, mu_Rect rect, mu_Color color) {
	mu_Rect src = atlas[id];
	draw_quad(src, rect, color);
}

#if BUILTIN_FONT
static int text_width(mu_Font font, const char *text, int len) {
  	int w = 0;
	for(; *text && len--; text++) {
		if((*text & 0xC0) == 0x80) continue;
		int chr = mu_min((unsigned char)*text, 127);
		w += atlas[ATLAS_FONT + chr].w;		
	}
	return w;
}

static int text_height(mu_Font font) {
  return 18;
}

void r_draw_text(const char *text, mu_Vec2 pos, mu_Color color) {
	mu_Rect dst = {pos.x, pos.y, 0, 0};
	for(; *text; text++) {
		if((*text & 0xC0) == 0x80) continue;
		int chr = mu_min((unsigned char)*text, 127);
		mu_Rect src = atlas[ATLAS_FONT + chr];
		dst.w = src.w;
		dst.h = src.h;
		draw_quad(src, dst, color);
		dst.x += dst.w;
	}
}

#else

static int text_width(mu_Font font, const char *text, int len) {
  if (len == -1) { len = strlen(text); }
  return len * TEXT_W;
}

static int text_height(mu_Font font) {
  return 8;
}

static void printchar(int x, int y, char c, uint32_t color) {
	if(c < ' ' || c >= 128) c = '*';
	
	int xs = x, ye = y + 8;
	const uint8_t *p = &font[ (c - 0x20) * 8 ];
	for(; y < ye; y++, p++) {
		if(y < clip.y) continue;
		if(y >= clip.y + clip.h) break;
		for(int i = 0x80, x = xs; i; i >>= 1, x++) {
			if(x < clip.x) continue;
			if(x >= clip.x + clip.w) break;
			if(i & *p) {
				fenster_pixel(F, x, y) = color;
			}
		}
	}
}

void r_draw_text(const char *text, mu_Vec2 pos, mu_Color color) {
	int x = pos.x, y = pos.y;	
	uint32_t c = (color.r << 16) + (color.g << 8) + (color.b);
	for(; *text; text++) {
		if(*text == '\n') {
			x = pos.x;
			y += TEXT_W;
		} else if(*text == '\r') {
			x = pos.x;
		} else if(*text == '\t') {
			x += 4 * TEXT_W;
		} else {
			printchar(x, y, *text, c);
			x += TEXT_W;
		}
	}
}

#endif

static int lastx, lasty, lastm;
static int lastkeys[256], lastmod;

void fenmui_init(struct fenster *fp, mu_Context * ctxp) {
	F = fp;
	MC = ctxp;
	MC->text_width = text_width;
	MC->text_height = text_height;
	
	clip.x = 0;
	clip.y = 0;
	clip.w = F->width;
	clip.h = F->height;
	
	lastx = F->x;
	lasty = F->y;
	lastm = F->mouse;
	lastmod = F->mod;
	memcpy(lastkeys, F->keys, sizeof lastkeys);
}

static const char key_map[256] = {
  [ 10 ] = MU_KEY_RETURN,
  [ 8 ] = MU_KEY_BACKSPACE,
};

void fenmui_events() {
	if(lastx != F->x || lasty != F->y) {
		mu_input_mousemove(MC, F->x, F->y);
		lastx = F->x;
		lasty = F->y;
	}
	if(lastm != F->mouse) {
		if(F->mouse)
			mu_input_mousedown(MC, F->x, F->y, MU_MOUSE_LEFT);
		else
			mu_input_mouseup(MC, F->x, F->y, MU_MOUSE_LEFT);
			
		lastm = F->mouse;
	}
	for(int i = 0; i < 256; i++) {
		if(F->keys[i] != lastkeys[i]) {
			if(F->keys[i] && isprint(i)) {
				int c = i;
				if(isalpha(i) && !(F->mod & 0x2))
					c = tolower(i);
				else if(F->mod & 0x2) {						
					char *from = "`1234567890-=,./;'[]\\";
					char *to   = "~!@#$%^&*()_+<>?:\"{}|";
					for(int j = 0; from[j]; j++)							
						if(c == from[j]) {
							c = to[j];
							break;
						}							
				}					
				char text[2] = {c, '\0'};
				mu_input_text(MC, text);
			}
			if(key_map[i]) {
				if(F->keys[i]) 
					mu_input_keydown(MC, key_map[i]);
				else
					mu_input_keyup(MC, key_map[i]);
			}
		}
		lastkeys[i] = F->keys[i];
	}
	
	if(lastmod != F->mod) {
		int diff = lastmod ^ F->mod;
		if(diff & 0x1) {
			if(F->mod & 0x1) 
				mu_input_keydown(MC, MU_KEY_CTRL);
			else
				mu_input_keyup(MC, MU_KEY_CTRL);
		}
		if(diff & 0x2) {
			if(F->mod & 0x2) 
				mu_input_keydown(MC, MU_KEY_SHIFT);
			else
				mu_input_keyup(MC, MU_KEY_SHIFT);
		}
		if(diff & 0x4) {
			if(F->mod & 0x4) 
				mu_input_keydown(MC, MU_KEY_ALT);
			else
				mu_input_keyup(MC, MU_KEY_ALT);
		}				
		lastmod = F->mod;
	}
}

void fenmui_draw() {
	mu_Command *cmd = NULL;
	while (mu_next_command(MC, &cmd)) {
	  switch (cmd->type) {
		case MU_COMMAND_TEXT: r_draw_text(cmd->text.str, cmd->text.pos, cmd->text.color); break;
		case MU_COMMAND_RECT: r_draw_rect(cmd->rect.rect, cmd->rect.color); break;
		case MU_COMMAND_ICON: r_draw_icon(cmd->icon.id, cmd->icon.rect, cmd->icon.color); break;
		case MU_COMMAND_CLIP: r_set_clip_rect(cmd->clip.rect); break;
	  }
	}
}
