/*
gcc -I /d/libs/PDCurses-3.4/ cursed.c /d/libs/PDCurses-3.4/win32/pdcurses.a
*/
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#include <curses.h>
#include <unistd.h>

#include "map.h"

#define MIN(a,b) ((a)<(b)?(a):(b))

// Reads an entire text file into memory
// You need to free the returned pointer afterwards.
char *read_text_file(const char *fname) {
	FILE *f;
	long len,r;
	char *str;

	if(!(f = fopen(fname, "rb")))
		return NULL;

	fseek(f, 0, SEEK_END);
	len = ftell(f);
	rewind(f);

	if(!(str = malloc(len+2)))
		return NULL;
	r = fread(str, 1, len, f);

	if(r != len) {
		free(str);
		return NULL;
	}

	fclose(f);
	str[len] = '\0';
	return str;
}

static void usage(const char *name) {
	fprintf(stderr, "Usage: %s [options] file\n", name);
	fprintf(stderr, "where options:\n");
	fprintf(stderr, " -w width  : Width of a new map.\n");
	fprintf(stderr, " -h height : Height of a new map.\n");
	fprintf(stderr, "    -w and -h are only applicable when creating a new map.\n");
    fprintf(stderr, " -n        : Forces a new map.\n");	
    fprintf(stderr, "    The new map will replace 'file'.\n");	
    fprintf(stderr, "    A backup of 'file' will be created as 'file~'.\n");	
    fprintf(stderr, " -b c      : Creates a default border of 'c' around the new map.\n");	
    fprintf(stderr, " -?        : Displays this help.\n");	
}

static int get_attrs(int c) {
    int a = COLOR_PAIR(1);
    if(strchr("^|-", c)) {
        a = COLOR_PAIR(4) | A_BOLD;
    } else if(islower(c) || ispunct(c)) {
        a = COLOR_PAIR(3) | A_BOLD;
    } else if(isgraph(c)) {
        a = COLOR_PAIR(2);
    }
    
    return c | a;
}

static void draw_all(struct Map *m, WINDOW *win, int scry, int scrx) {
    int h, w;
    getmaxyx(win, h, w);
    int x, y;
    for(y = 0; y < m->h; y++){
        int py = y - scry + 1;        
        if(py < 1) continue;
        if(py >= h - 1) break;
        for(x = 0; x < m->w; x++){            
            int px = x - scrx + 1;
            if(px < 1) continue;
            if(px >= w - 1) break;
            wmove(win, y - scry + 1, x - scrx + 1);
            waddch(win, get_attrs(m->tiles[(y) * m->w + (x)].c));
        }
    }
    box(win, ACS_VLINE, ACS_HLINE);
    if(m->w > w) {
        x = w * (scrx) / (m->w - w);
        if(x < 0) x = 0;
        if(x >= w) x = w - 1;
        wmove(win, h - 1 ,x);
        waddch(win, ACS_DIAMOND);
    }
    if(m->h > h) {
        y = (h ) * (scry) / (m->h - h - 2);
        if(y < 0) y = 0;
        if(y >= h) y = h - 1;
        wmove(win, y, w - 1);
        waddch(win, ACS_DIAMOND);
    }
    wrefresh(win);
}

int main(int argc, char*argv[]) {
    
    const char *filename;
    
    struct Map *m = NULL;
    
    // Before starting curses, check the command line parameters:
    int opt;
    int ow = DEFAULT_WIDTH, oh = DEFAULT_HEIGHT, forcenew = 0,
        b = '\0';
    while((opt = getopt(argc, argv, "w:h:nb:?")) != -1) {
		switch(opt) {
			case 'w': {
				ow = atoi(optarg);
			} break;
			case 'h' : {
				oh = atoi(optarg);
			} break;
			case 'n' : {
				forcenew = 1;
			} break;
			case 'b' : {
				b = optarg[0];
			} break;
			case '?' : {
				usage(argv[0]);
				return 1;
			}
		}
	}
    
    if(optind < argc) {
        filename = argv[optind];
    } else {
        fprintf(stderr, "No filename specified.\n");
        fprintf(stderr, "Run with -? for instructions.\n");
        return 1;
    }
    
    m = open_map(filename);
    if(!m) {
        forcenew = 1;
    } else {
        // Create a backup.
        char buffer[128];
        sprintf(buffer, "%s~", filename);
        save_map(m, buffer);
    }
    
    if(forcenew) {
        if(m) 
            free_map(m);
        // Create a new map
        m = create_map(ow, oh);
        if(b) {
            int i;
            for(i = 0; i < ow; i++) {
                struct tile *t = get_tile(m, i, 0);
                t->c = b;
                t = get_tile(m, i, m->h - 1);
                t->c = b;
            }
            for(i = 0; i < oh; i++) {
                struct tile *t = get_tile(m, 0, i);
                t->c = b;
                t = get_tile(m, m->w - 1, i);
                t->c = b;
            }
        }
    }
    
    //return 0;// TESTING
    
    // No printf() calls beyond this point
    initscr();
    int sh, sw;
    getmaxyx(stdscr, sh, sw);
    box(stdscr, ACS_VLINE, ACS_HLINE);
    //keypad(stdscr, TRUE);
    move(0, 2);
    printw(" %s ", filename);
    
    move(0,0);
    
    noecho();
    
    if(has_colors()) {
        start_color();
        
        init_pair(1, COLOR_WHITE, COLOR_BLACK);
        init_pair(2, COLOR_CYAN, COLOR_BLUE);
        init_pair(3, COLOR_YELLOW, COLOR_BLACK);
        init_pair(4, COLOR_CYAN, COLOR_BLACK);
    }
    
    refresh();
    
    int wh = MIN(m->h+2,sh-2), ww = MIN(m->w+2,sw-2);
    //if(wh > 20) wh = 20;
    //if(ww > 20) ww = 20;
    
    /* Apparently you shouldn't have overlapping windows, so what I did here
    is wrong. It's not really a problem though with how the program uses those
    windows. */
    // There is also the subwin() function, but the talk about touchwin() confused me.
    WINDOW *w = newwin(wh, ww,(sh-wh)/2,(sw-ww)/2);
    if(!w) {
        endwin();
        fprintf(stderr, "error: Unable to create editor window\n");
        return 1;
    }
    keypad(w, TRUE); 
    box(w, ACS_VLINE, ACS_HLINE);    
    
    int x = 0, y = 0;
    int scrx = 0, scry = 0; // scroll

    draw_all(m, w, scry, scrx);
    
    int autoright = 0;
    
    int done = 0;
    while(!done) {
        
        wmove(w, y - scry + 1, x - scrx + 1);        
        wrefresh(w);
        
        int c = wgetch(w);
        switch(c) {
            case 27: { 
                // Escape quits.
                done = 1; 
            } break;
            case KEY_UP: y--; break;
            case KEY_DOWN: y++; break;
            case KEY_LEFT: x--; break;
            case KEY_RIGHT: x++; break;
            case KEY_F(1): { 
                save_map(m, filename); 
            } break;
            case KEY_F(2): { 
                // Press F2 to toggle automatically moving the cursor
                autoright = !autoright; 
            } break;
            default : {
                if(isprint(c)) {
                    m->tiles[(y) * m->w + (x)].c = c;
                    waddch(w,get_attrs(c));
                    if(autoright) x++;
                } else if(c == ' ' || c == KEY_BACKSPACE) {                    
                    m->tiles[(y) * m->w + (x)].c = ' ';
                    waddch(w,get_attrs(' '));
                    if(autoright) x++;
                }
            } break;
        }
        if(x < 0) x = 0;
        if(x >= m->w) 
            x = m->w - 1;
        if(y < 0) y = 0;
        if(y >= m->h)
            y = m->h - 1;
                    
        // Scrolling
        int nsx = x - ww/2;
        if(nsx < 0)
            nsx = 0;
        if(nsx + (ww - 2) >= m->w)
            nsx = m->w - (ww - 2);
            
        int nsy = y - wh/2;
        if(nsy < 0)
            nsy = 0;
        if(nsy + (wh - 2) >= m->h)
            nsy = m->h - (wh - 2);
            
        if(nsx != scrx || nsy != scry) {
            scrx = nsx;
            scry = nsy;
            draw_all(m, w, scry, scrx);
        }
    }
    delwin(w);
    endwin();
    
    // Save a "work in progress" file,
    // in case they forgot to save before exit.
    char wip[128];
    sprintf(wip, "%s.wip", filename);
    save_map(m, wip);
    
    free_map(m);
    
    return 0;
}
