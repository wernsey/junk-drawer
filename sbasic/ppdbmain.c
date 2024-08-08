#include <stdio.h>
#include <errno.h>

#define PP_USE_PP_RED_PP_BLACK 0

#define PPDB_IMPLEMENTATION
#include "ppdb.h"

#define MEM_SIZE  8192
char memory[MEM_SIZE];

static void showfun(const char *key, const char *value, void *cookie) {
  (void)cookie;
  printf("'%s' => '%s'\n", key, value);
}

int main(int argc, char *argv[]) {
  int save = 0, i;
  ppdb_t DB;
  FILE *f;
  
  pp_init(&DB, memory, MEM_SIZE);

  if(argc < 2) {
    fprintf(stderr, "usage: %s database.db commands...\n", argv[0]);
    fprintf(stderr, "where commands is a sequence of:\n");
    fprintf(stderr, "  * `poke {key} {value}`\n");
    fprintf(stderr, "  * `peek {key}`\n");
    fprintf(stderr, "  * `list`\n");
    fprintf(stderr, "  * `tree`\n");
    fprintf(stderr, "  * `compact`\n");
    return 1;
  }

  f = fopen(argv[1], "rb");
  if(f) {
    if(pp_load(&DB, f) != PP_OK) {
      fprintf(stderr, "Unable to load DB");
      return 1;
    }
    fclose(f);
  } else {
#ifdef __VBCC__
    fprintf(stderr, "File %s does not exist. Creating it.\n", argv[1]);
#else
    if(errno == ENOENT) {
      fprintf(stderr, "File %s does not exist. Creating it.\n", argv[1]);
    } else {
      fprintf(stderr, "error: Unable to open %s: %s\n", argv[1], strerror(errno));
      return 1;
    }
#endif
  }

  for(i = 2; i < argc; i++) {
    if(!strcmp(argv[i], "peek")) {
      const char *value;
      if(++i == argc) {
        fprintf(stderr, "error: `peek` expects a key\n");
        return 1;
      }
      value = pp_peek(&DB, argv[i]);
      if(value) {
        printf("%s\n", value);
      } else
        fprintf(stderr, "error: key `%s` not found\n", argv[i]);
    } else if(!strcmp(argv[i], "poke")) {
      if(i+2 >= argc) {
        fprintf(stderr, "error: `poke` expects a key and a value\n");
        return 1;
      }
      pp_poke(&DB, argv[i+1], argv[i+2]);
      save = 1;
      i += 2;
    } else if(!strcmp(argv[i], "list")) {
      pp_foreach(&DB, showfun, NULL);
    } else if(!strcmp(argv[i], "compact")) {
      pp_compact(&DB);
      save = 1;
    } else if(!strcmp(argv[i], "tree")) {
      pp_tree(&DB);
    } else {
      fprintf(stderr, "error: unknown command `%s`\n", argv[i]);
    }
  }

  if(save) {
    f = fopen(argv[1], "wb");
    if(!f) {
      fprintf(stderr, "error: Unable to save %s: %s", argv[1], strerror(errno));
    }
    if(pp_save(&DB, f) != PP_OK) {
      fprintf(stderr, "Unable to save DB");
      return 1;
    }
    fclose(f);
  }

  return 0;
}

