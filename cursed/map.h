
#define DEFAULT_WIDTH 20
#define DEFAULT_HEIGHT 20

#define APP_NAME "CURSED"
#define VERSION 1

#define MAX_META    16

struct tile {
    char c;
    char index;
    char state;
    char ctr;
};

struct Map {
    int w, h;
    struct tile *tiles; 
    
    // Meta data
    struct {
        char *key;
        char *value;
    } meta[MAX_META];
    int nmeta;
};

#define MAP_TILE(m,x,y) m->tiles[(y) * m->w + (x)]

struct Map *create_map(int w, int h);

void free_map(struct Map *m);

struct tile *get_tile(struct Map *m, int x, int y);

struct Map *open_map(const char *filename);

int save_map(struct Map *m, const char *filename);
