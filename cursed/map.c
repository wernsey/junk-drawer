#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#include "map.h"

// For writing a portable app, this function 
// comes from externally (So that I can later provide a
// SDL_RWops implementation)
extern char *read_text_file(const char *fname);

struct Map *create_map(int w, int h) {    
    struct Map *m = malloc(sizeof *m);
    m->w = w;
    m->h = h;
    m->tiles = malloc(w * h * sizeof *m->tiles);
    
    memset(m->tiles, 0, w * h * sizeof *m->tiles);    
    int i;
    for(i = 0; i < w * h; i++)
        m->tiles[i].c = ' ';
        
    m->nmeta = 0;
    
    return m;
}

void free_map(struct Map *m) {
    int i;
    if(!m) 
        return;
    for(i = 0; i < m->nmeta; i++) {
        free(m->meta[i].key);
        free(m->meta[i].value);
    }
    free(m->tiles);
    free(m);
}

struct tile *get_tile(struct Map *m, int x, int y) {
     if(x < 0 || x >= m->w || y < 0 || y >= m->h)
        return NULL;
    return &m->tiles[(y) * m->w + (x)];
}

#define SCAN_END    0
#define SCAN_NUM    1
#define SCAN_WORD   2
#define SCAN_STRING 3
static int scantok(char *in, char *tok, char **rem) {
    // TODO: Ought to guard against buffer overruns in tok, but not today.
    int t = SCAN_END;
    *tok = '\0';
    if(!in) {
        in = *rem;
    }
    
scan_start:
    while(isspace(*in))
        in++;
    
    if(!*in) 
        return SCAN_END;
    
    if(*in == '#') {
        while(*in && *in != '\n')
            in++;
        goto scan_start;
    }
    
    if(isalpha(*in)) {
        while(isalnum(*in))
            *tok++ = *in++;
        *tok = '\0';
        t = SCAN_WORD;
    } else if(isdigit(*in)) {
        while(isdigit(*in))
            *tok++ = *in++;
        *tok = '\0';
        t = SCAN_NUM;
    } else if(*in == '\"' || *in == '\'') {
        char term = *in++;
        while(*in != term) {
            if(!*in) {
                t = SCAN_END;
                goto end;
            }
            if(*in == '\\') {
                switch(*++in) {
                    case 'n' : *tok++ = '\n'; break;
                    case 't' : *tok++ = '\t'; break;
                    default : *tok++ = *in; break;
                }
                in++;
            } else
                *tok++ = *in++;
        }
        in++;
        *tok = '\0';
        t = SCAN_STRING;
    } else
        t = *in++;
end:
    *rem = in;
    return t;
}

#define TOK_SIZE 64

struct Map *open_map(const char *filename) {
    int ver, w, h;
    struct Map *m = NULL;
    
    char *text = read_text_file(filename);
    if(!text) 
        return NULL;

    char token[TOK_SIZE], *rem;
    int t = scantok(text, token, &rem);
    if(t != SCAN_WORD)
        goto end;
    if(strcmp(token, APP_NAME))
        goto end;
    
    t = scantok(NULL, token, &rem);
    if(t != SCAN_NUM)
        goto end;
    ver = atoi(token);
    
    (void)ver; // not using it at the moment, but we might later
    
    t = scantok(NULL, token, &rem);
    if(t != SCAN_NUM)
        goto end;
    w = atoi(token);
    
    t = scantok(NULL, token, &rem);
    if(t != SCAN_NUM)
        goto end;
    h = atoi(token);
    
    while(*rem != '\n') 
        rem++;
    rem++;
    
    m = create_map(w,h);
    
    int x,y;
    for(y = 0; y < m->h; y++) {
        for(x = 0; x < m->w; x++) {
            m->tiles[(y) * m->w + (x)].c = *rem++;
            if(!m->tiles[(y) * m->w + (x)].c) {
                free_map(m);
                m = NULL;
                goto end; 
            }
        }
        int c = *rem++;
        if(c == '\r')
            rem++;
    }
    
    t = scantok(rem, token, &rem);
    if(t != SCAN_END) {
        if(t == SCAN_WORD && !strcmp("meta", token)) {
            if((t = scantok(NULL, token, &rem)) != '{') {
               // printf("expected {\n");
                goto meta_error;
            }
            t = scantok(NULL, token, &rem);
            
            while(t != '}') {
                char *key, *value;            
                
                if(t == SCAN_WORD || t == SCAN_STRING) {
                    key = strdup(token);
                } else {
                 //   printf("key expected");
                    goto meta_error;
                }
                
                if(scantok(NULL, token, &rem) != ':') {
                   // printf("expected :\n");
                    goto meta_error;
                }
                
                t = scantok(NULL, token, &rem);
                if(t == SCAN_WORD || t == SCAN_NUM || t == SCAN_STRING) {
                    value = strdup(token);
                } else {
                   // printf("value expected");
                    goto meta_error;
                }
                
                int i = m->nmeta++;
                m->meta[i].key = key;
                m->meta[i].value = value;
                //printf("Meta[%d]: '%s' = '%s'\n", i, key, value);
                
                t = scantok(NULL, token, &rem);
                if(t == '}')
                    break;
                else if(t != ',') {
                    //printf("expected , or } (%d)\n", t);
                    goto meta_error;
                }
                t = scantok(NULL, token, &rem);
            }
        }
    }
    
    goto end;
    
meta_error:
    free_map(m);
    m = NULL;
    
end:        
    free(text);    
    return m;
}

static char *escape_string(const char *str, char *buf) {
    char *sav = buf;
    while(*str) {
        switch(*str) {
            case '\n' : *buf++ = '\\'; *buf++ = 'n'; break;
            case '\t' : *buf++ = '\\'; *buf++ = 't'; break;
            case '\"' : *buf++ = '\\'; *buf++ = '\"'; break;
            case '\'' : *buf++ = '\\'; *buf++ = '\''; break;
            case '\\' : *buf++ = '\\'; *buf++ = '\\'; break;
            default : *buf++ = *str;
        }
        str++;
    }
    *buf = '\0';
    return sav;
}

int save_map(struct Map *m, const char *filename) {
    FILE *f = fopen(filename, "w");
    if(!f) 
        return 0;
        
    fprintf(f, "# Cursed map file.\n");
    fprintf(f, "%s %d\n", APP_NAME, VERSION);
    fprintf(f, "%d %d\n", m->w, m->h);
    
    int x,y;
    for(y = 0; y < m->h; y++) {
        for(x = 0; x < m->w; x++) {
            fputc(m->tiles[(y) * m->w + (x)].c, f);
        }
        fputc('\n', f);
    }
    fputs("# Meta data, as key : value pairs\n", f);
    fputs("meta {\n", f);
    for(x = 0; x < m->nmeta; x++) {
        char k[TOK_SIZE], v[TOK_SIZE];
        fprintf(f, "\"%s\" : \"%s\"%s\n", 
            escape_string(m->meta[x].key, k), 
            escape_string(m->meta[x].value, v),
            x == m->nmeta - 1 ? "" : ",");
        
    }
    fputs("}\n", f);
    
    fclose(f);
    return 1;
}
