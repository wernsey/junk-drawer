/**
 * PPDB
 * ====
 *
 * _The Peek 'n Poke Database._
 * A _very_ simple in-memory key-value store that can be persisted to a file.
 * 
 * It was created for a retro-computer gaming project and is only designed for very 
 * small data sets, like keeping track of NPCs the player spoke to, quests completed
 * and dialog options chosen in an adventure/RPG game. It was written in C89 to
 * be compatible with very old C compilers.
 * 
 * The idea is that the database gets a block of _working memory_ at start-up where
 * it stores the key-value pairs.
 * 
 * The key-value pairs are stored in a [Red-Black tree][] inside the working memory. 
 * Strings are also kept inside this block. Memory allocation happens through a 
 * simple bump allocator.
 *
 * The working memory is managed by a Mark-compact garbage collector, based on the 
 * [LISP2 algorithm][LISP2_algorithm]. When the working memory is full, the
 * garbage collector is invoked to reclaim unused memory.
 *
 * To persist a database, the block is just written to a file. The block can then
 * be loaded from disk.
 * 
 * **ppdb.h** is provided under the [FSF All-permissive License][FSF-APL].
 * See the License subsection for details.
 * 
 * Usage
 * -----
 *
 * In _one_ of your C source files, define `PPDB_IMPLEMENTATION` and
 * include **ppdb.h**, like so:
 *
 * ```c
 * #define PPDB_IMPLEMENTATION
 * #include "ppdb.h"
 * ```
 * 
 * All your other source files then use `#include "ppdb.h"`.
 *
 * Create a `ppdb_t` object somewhere. Point it to a blob of memory where it
 * will keep its data through `pp_init()`.
 *
 * From there, use `pp_poke()` to store a key-value pair, and `pp_peek()` to
 * retrieve the value associated with a key.
 *
 * Use `pp_save()` to write the database to a `FILE`, and `pp_load()` to
 * retrieve the database.
 *
 * The functions `pp_next()` and `pp_foreach()` are provided to iterate through
 * all the key-value pairs in the database.
 *
 * `pp_compact()` is provided to invoke the garbage collector to compact the
 * database.
 *
 * Links
 * -----
 *
 * * The Red-Black Tree implementation here is based on one from another 
 *   project on my drive, which I'm sure was based on the one on the
 *   [Wikipedia][Red-Black tree].
 *   That article has changed a lot in the meantime, so you have to look at
 *   [an older version](https://en.wikipedia.org/w/index.php?title=Red%E2%80%93black_tree&oldid=933596226).
 * * The garbage collector was inspired by [lisp2-gc][] by munificent.
 * * The Wikipedia page for the [Mark-compact garbage collector][LISP2_algorithm].
 * * The code for `pp_next()` was based on the Wikipedia article on 
 *   [Binary search trees][BST]' successor and predecessor section
 * * I found the ideas in these articles intriguing, but ended up not using them:
 *   * [Hash based trees and tries](https://nrk.neocities.org/articles/hash-trees-and-tries)
 *   * [Implementing and simplifying Treap in C](https://nrk.neocities.org/articles/simple-treap)
 * 
 * [Red-Black tree]: https://en.wikipedia.org/wiki/Red%E2%80%93black_tree
 * [BST]: https://en.wikipedia.org/wiki/Binary_search_tree#Successor_and_predecessor
 * [LISP2_algorithm]: https://en.wikipedia.org/wiki/Mark-compact_algorithm#LISP2_algorithm
 * [lisp2-gc]: https://github.com/munificent/lisp2-gc
 *
 */

/*
Note about the documentation
----------------------------
You can use the below script from a Linux shell to generate a HTML document from the 
Markdown comments in this file. It uses [Markdeep](https://casual-effects.com/markdeep/)
to format the document:

	echo '<meta charset="utf-8">' > ppdb.md.html
	awk '/\/\*[*]/{sub(/\/\*\*[[:space:]][*]/,"");incomment=1}\
	  incomment && /\*\//{incomment=0;sub(/[[:space:]]*\*\/(.*)/,"");print} \
	  incomment && /^[[:space:]]*[*]/{sub(/^[[:space:]]*\*[[:space:]]?/,""); print}' ppdb.h >> ppdb.md.html
	echo '<!-- Markdeep: -->' >> ppdb.md.html
	echo '<style class="fallback">body{visibility:hidden;white-space:pre;font-family:monospace}</style>' >> ppdb.md.html
	echo '<script>markdeepOptions={tocStyle:"auto"};</script>' >> ppdb.md.html
	echo '<script src="https://morgan3d.github.io/markdeep/latest/markdeep.min.js" charset="utf-8"></script>' >> ppdb.md.html
	echo '<script>window.alreadyProcessedMarkdeep||(document.body.style.visibility="visible")</script>' >> ppdb.md.html
*/

#ifndef PPDB_H
#define PPDB_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * API
 * ---
 *
 * ### Types
 * 
 * `typedef unsigned short pp_index`
 * 
 * The type for indexes into the memory.
 *
 * `typedef struct ppdb_t ppdb_t`
 *
 * The structure containing the database information.
 */

typedef unsigned short pp_index;

typedef struct ppdb_t {
  pp_index bump, mem_size;
  pp_index root;
  pp_index k, v;
  char *memory;
} ppdb_t;

/** 
 * `typedef enum pp_err_t`
 *
 * Errors that can occur throughout the module. There are these
 * values:
 *
 * * `PP_OK` - no error.
 * * `PP_ERROR` - unspecified error.
 * * `PP_MEMORY` - insufficient memory. The memory block passed
 *   to `pp_init()` is not large enough to hold the database.
 * * `PP_FWRITE` - An error happened during a `fwrite()` call.
 * * `PP_FREAD` - An error happened during a `fread()` call.
 * * `PP_FILE` - A file is not a valid PPDB file.
 *
 * In the case of `PP_FWRITE` and `PP_FREAD` you might get more
 * information from your system's `errno` and `strerror()` 
 */
typedef enum {
  PP_OK = 0,
  PP_ERROR,
  PP_MEMORY,
  PP_FWRITE,
  PP_FREAD,
  PP_FILE
} pp_err_t;

/**
 * ### Initialization
 *
 * `void pp_init(ppdb_t *db, char *memory, pp_index mem_size);`
 *
 * Initializes a database. 
 *
 * `memory` is a pointer to an array of bytes where the data will be kept.
 * `mem_size` is the size of this memory area.
 */
void pp_init(ppdb_t *db, char *memory, pp_index mem_size);

/**
 * ### Storing and retrieving values
 *
 * * `pp_err_t pp_poke(ppdb_t *db, const char *key, const char *value);`
 * * `const char *pp_peek(ppdb_t *db, const char *key);`
 *
 * `pp_poke()` stores a value `value` associated with a key `key` in
 * the database. It will return `PP_OK` on success, or `PP_MEMORY` if
 * there's not enough memory available to store the value.
 *
 * `pp_peek()` is then used to retrieve the value associated with `key`
 * the database. It returns `NULL` if the key is not in the database.
 *
 * Note that `pp_poke()` may trigger a garbage collection which may in
 * turn move values around in the working memory. Therefore the pointer
 * returned by `pp_peek()` should not be held after subsequent `pp_poke()`
 * calls.
 *
 */
pp_err_t pp_poke(ppdb_t *db, const char *key, const char *value);

const char *pp_peek(ppdb_t *db, const char *key);

/**
 * ### Garbage Collection
 *
 * `void pp_compact(ppdb_t *db);`
 *
 * Invokes the garbage collector to compact the database.
 */
void pp_compact(ppdb_t *db);

#ifdef EOF
/**
 * ### Saving and loading the database
 *
 * * `pp_err_t pp_save(ppdb_t *db, FILE *f);`
 * * `pp_err_t pp_load(ppdb_t *db, FILE *f);`
 *
 * Saves and loads the database `db` to a file `f` respectively. 
 *
 * Both functions return `PP_OK` on success.
 *
 * `pp_save()` may return `PP_FWRITE` if an error occurs while
 * writing to the file.
 *
 * `pp_load()` may return `PP_FILE` if the file being loaded is
 * not a valid PPDB file (a file saved by `pp_save()`), `PP_FREAD`
 * if an error occurs while reading the data, or `PP_MEMORY` if
 * the data in the file won't fit into the memory allocated to 
 * the database through `pp_init()`
 *
 * The file must be `fopen()`ed in write binary mode ("wb") for saving
 * and read binary mode ("rb") when loading it. 
 * `<stdio.h>` must be `#include`d before "ppdb.h" to use these functions.
 */
pp_err_t pp_save(ppdb_t *db, FILE *f);
pp_err_t pp_load(ppdb_t *db, FILE *f);
#endif

/**
 * ### Iteration
 *
 * `const char *pp_next(ppdb_t *db, const char *key);`
 *
 * Given a key `key` returns the next key in the database.
 *
 * If `key` is `NULL`, it will return the first key in the database.
 * If `key` is the last key in the database it will return `NULL`.
 *
 * It can be used to iterate through a database in a for-loop, like so:
 *
 * ```c
 * const char *key, *value;
 * for(key = pp_next(&DB, NULL); key; key = pp_next(&DB, key)) {
 *   value = pp_peek(&DB, key);
 *   ...
 * }
 * ```
 */
const char *pp_next(ppdb_t *db, const char *key);

/**
 * `void pp_foreach(ppdb_t *db, pp_iterfun_t fun, void *cookie);`
 *
 * Iterates through the entire database and calls the function `fun` for
 * each key-value pair in the database.
 * 
 * The function `fun` must match this prototype:
 *
 * ```
 * typedef void (*pp_iterfun_t)(const char *key, const char *value, void *cookie);
 * ```
 *
 * The pointer `cookie` is passed unaltered to `fun`.
 */
typedef void (*pp_iterfun_t)(const char *key, const char *value, void *cookie);
void pp_foreach(ppdb_t *db, pp_iterfun_t fun, void *cookie);

/**
 * `void pp_tree(ppdb_t *db);`
 *
 * Shows the internal binary tree on `stdout`.
 *
 * Meant for testing/development purposes only. 
 */
void pp_tree(ppdb_t *db);

#if defined(PPDB_TEST) && !defined(PPDB_IMPLEMENTATION)
#  define PPDB_IMPLEMENTATION
#endif

#ifdef PPDB_IMPLEMENTATION

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

#define PP_NIL  0xFFFF

#define DEBUG_GC 0

#ifndef PP_USE_PP_RED_PP_BLACK
#  define PP_USE_PP_RED_PP_BLACK 1
#endif

#ifndef PP_CASE_SENSITIVE
#  define PP_CASE_SENSITIVE 0
#endif

void pp_init(ppdb_t *db, char *memory, pp_index mem_size) {
  db->bump = 0;  
  db->memory = memory;
  db->mem_size = mem_size;
  db->root = db->k = db->v = PP_NIL;
  /* memset(memory, 0xFF, mem_size);  */
}

typedef enum {PP_STRING = 0xF0, PP_NODE} pp_type_t;

typedef enum {PP_LEFT, PP_RIGHT} pp_dir_t;
enum {PP_RED, PP_BLACK};

#pragma pack(push, 1)
typedef struct {
  unsigned char type; /* Actually a pp_type_t */
  unsigned char xtra; /* Snuck the node color in here to save 2 bytes */
  pp_index mark;
  pp_index size;
} pp_object_t;

typedef struct {
  pp_object_t obj;
  /* pp_index len; */
  char str[1];
} pp_string_t;

typedef struct {
  pp_object_t obj;
  pp_index parent, child[2];
  pp_index key, value;
} pp_node_t;
#pragma pack(pop)

#define PP_NODE(DB, index) ((pp_node_t *)((DB)->memory + (index)))
#define left child[PP_LEFT]
#define right child[PP_RIGHT]
#define color obj.xtra

#define PP_PTR(DB,INDEX) ((void *)((DB)->memory + (INDEX)))

#if DEBUG_GC
static void pp_show_memory(ppdb_t *db) {
  pp_index ptr = 0;
  while(ptr < db->bump) {
    pp_object_t *obj = PP_PTR(db, ptr);
    printf("  %04X [%04X]: ", ptr, obj->mark);
    switch(obj->type) {
      case PP_STRING: {
        pp_string_t *str = (pp_string_t*)obj;
        printf("string ...: \"%s\"\n", str->str);
      } break;
      case PP_NODE: {
        pp_node_t *N = (pp_node_t*)obj;
        printf("node ....: P:%04X; L:%04X; R:%04X; K:%04X; V:%04X;\n", N->parent, N->left, N->right, N->key, N->value);
      } break;
    }
    ptr += obj->size;
  }
}
#endif

static void pp_mark_object(ppdb_t *db, pp_index ptr) {
  pp_object_t *obj;
  if(ptr == PP_NIL) 
    return;
  obj = PP_PTR(db, ptr);
  if(obj->mark != PP_NIL)
    return;
  obj->mark = 0; /* any non-NIL value? */
  if(obj->type == PP_NODE) {
    pp_node_t *N = (pp_node_t*)obj;
    pp_mark_object(db, N->parent);
    pp_mark_object(db, N->left);
    pp_mark_object(db, N->right);
    pp_mark_object(db, N->key);
    pp_mark_object(db, N->value);
  }
}

static void pp_update_pointer(ppdb_t *db, pp_index *ptr) {
  pp_object_t *obj;
  if(*ptr == PP_NIL) return;
  obj = PP_PTR(db, *ptr);
  *ptr = obj->mark;
}

void pp_compact(ppdb_t *db) {
  pp_index ptr, bump = 0, size;
  pp_object_t *obj, *dest;

  pp_mark_object(db, db->root); 
  pp_mark_object(db, db->k); 
  pp_mark_object(db, db->v); 

#if DEBUG_GC
  printf("Marked Objects:\n");
  pp_show_memory(db);
#endif

  /* Step 1: Compute the forwarding pointers */
  for(ptr = 0; ptr < db->bump; ptr += obj->size) {
    obj = PP_PTR(db, ptr);
    if(obj->mark != PP_NIL) {
      obj->mark = bump;
      bump += obj->size;
    }
  }

#if DEBUG_GC
  printf("Step 1: forwarding pointers\n");
  pp_show_memory(db);
#endif

  /* Step 2: Update pointers */
  pp_update_pointer(db, &db->root);
  pp_update_pointer(db, &db->k);
  pp_update_pointer(db, &db->v);
  for(ptr = 0; ptr < db->bump; ptr += obj->size) {
    obj = PP_PTR(db, ptr);
    if(obj->mark == PP_NIL) continue;
    if(obj->type == PP_NODE) {
      pp_node_t *N = (pp_node_t*)obj;
      pp_update_pointer(db, &N->parent);
      pp_update_pointer(db, &N->left);
      pp_update_pointer(db, &N->right);
      pp_update_pointer(db, &N->key);
      pp_update_pointer(db, &N->value);
    }
  }
  
#if DEBUG_GC
  printf("Step 2: updated pointers\n");
  pp_show_memory(db);
#endif

  for(ptr = 0; ptr < db->bump; ptr += size) {
    obj = PP_PTR(db, ptr);
    size = obj->size;
    if(obj->mark == PP_NIL) continue;
    dest = PP_PTR(db, obj->mark);
    obj->mark = PP_NIL;
    memmove(dest, obj, size);
  }
  db->bump = bump;
  
#if DEBUG_GC  
  printf("Step 3: moved objects\n");
  pp_show_memory(db);
#endif  
}

#define PP_ALIGN(x)  (((x) + 1) & 0xFFFE)

static pp_index pp_alloc(ppdb_t *db, pp_type_t type, pp_index size) {
  pp_index ptr;
  pp_index tsize = PP_ALIGN(size);
  pp_object_t *obj;

  if(db->bump + tsize > db->mem_size) {
    pp_compact(db);
    if(db->bump + tsize > db->mem_size)
      return PP_NIL;
  }

  ptr = db->bump;
  db->bump += tsize;

  obj = (pp_object_t *)(db->memory + ptr);
  obj->size = tsize;
  obj->type = type;
  obj->mark = PP_NIL;
  
  return ptr;
}

static pp_index pp_strdup(ppdb_t *db, const char *str) {
  size_t len = strlen(str);
  size_t size = sizeof(pp_string_t) - 1 + len + 1; /* -1 for the str[1], +1 for the '\0' */
  pp_index index;
  pp_string_t *sobj;
  if(size > PP_NIL)
    return PP_NIL;
  index = pp_alloc(db, PP_STRING, size);
  if(index == PP_NIL)
    return PP_NIL;
  sobj = (pp_string_t *)(db->memory + index);
  /*sobj->len = len;*/
  memcpy(sobj->str, str, len);
  sobj->str[len] = 0;
  return index;
}

static const char *pp_svalue(ppdb_t *db, pp_index idx) {
  pp_string_t *str = (pp_string_t *)(db->memory + idx);
  assert(str->obj.type == PP_STRING);
  return str->str;
}

#if !PP_CASE_SENSITIVE
static int pp_stricmp(const char *p, const char *q) {
    for(;*p && tolower(*p) == tolower(*q); p++, q++);
    return tolower(*p) - tolower(*q);
}
#define pp_strcmp(P,Q) pp_stricmp(P,Q)
#else
#define pp_strcmp(P,Q) strcmp(P,Q)
#endif

static pp_index pp_find(ppdb_t *db, const char *key, pp_index Ni) {
  int comp;
  pp_node_t *node;
  if(Ni == PP_NIL) {
    return PP_NIL;
  }
  node = PP_NODE(db, Ni);
  comp = pp_strcmp(key, pp_svalue(db, node->key));
  if(!comp) 
    return Ni;
  else if(comp < 0)
    return pp_find(db, key, node->left);
  else
    return pp_find(db, key, node->right);
}

#if PP_USE_PP_RED_PP_BLACK
static pp_index pp_uncle(ppdb_t *db, pp_index Ni) {
  pp_node_t *N, *P, *G;
  assert(Ni != PP_NIL);
  N = PP_NODE(db, Ni);
  if(N->parent != PP_NIL) {
      pp_index Pi = N->parent;
      P = PP_NODE(db, Pi);
      if(P->parent != PP_NIL) {
          G = PP_NODE(db, P->parent);
          return Pi == G->left ? G->right : G->left;
      }
  }
  return PP_NIL;
}

static void pp_rotate_dir_root(ppdb_t *db, pp_index Ni, pp_dir_t dir) {
  pp_node_t *N, *P, *S;
  pp_index Pi, Si, Ci;
  
  assert(Ni != PP_NIL);
  N = PP_NODE(db, Ni);

  Si = N->child[1-dir]; S = PP_NODE(db, Si);
  Pi = N->parent; P = PP_NODE(db, Pi);
  
  N->child[1-dir] = S->child[dir];
  S->child[dir] = Ni;
  N->parent = Si;
  Ci = N->child[1-dir];
  if(Ci != PP_NIL)
    PP_NODE(db, Ci)->parent = Ni;

  if(Pi != PP_NIL) {
    if(Ni == P->left)
      P->left = Si;
    else
      P->right = Si;
  } else {
    /* Sibling is now the root... */
    db->root = Si; 
  }
  S->parent = Pi;
}

#define pp_rotate_left(DB, NI) pp_rotate_dir_root(DB, NI, PP_LEFT)
#define pp_rotate_right(DB, NI) pp_rotate_dir_root(DB, NI, PP_RIGHT)

static void pp_rebalance(ppdb_t *db, pp_index Ni) {
  pp_node_t *N, *P, *U, *G;
  pp_index Pi, Ui, Gi;
  assert(Ni != PP_NIL);
  
  N = PP_NODE(db, Ni);
  Pi = N->parent;
  if(Pi == PP_NIL) {
    /* case 1 */
    N->color = PP_BLACK;
  } else if((P = PP_NODE(db, Pi))->color == PP_BLACK) {
    /* case 2 */
    return;
  } else {
    Ui = pp_uncle(db, Ni);
    Gi = P->parent;
    assert(Gi != PP_NIL);
    G = PP_NODE(db, Gi);
    if(Ui != PP_NIL && (U = PP_NODE(db, Ui))->color == PP_RED) {
      /* case 3 */
      P->color = PP_BLACK;
      U->color = PP_BLACK;
      G->color = PP_RED;
      pp_rebalance(db, Gi);
    } else {
      /* case 4 */
      if(Ni == P->right && Pi == G->left) {
        pp_rotate_left(db, Pi);
        Ni = N->left; N = PP_NODE(db, Ni);        
        Pi = N->parent; P = PP_NODE(db, Pi);        
        Gi = P->parent; G = PP_NODE(db, Gi);        
      } else if(Ni == P->left && Pi == G->right) {
        pp_rotate_right(db, Pi);
        Ni = N->right; N = PP_NODE(db, Ni);        
        Pi = N->parent; P = PP_NODE(db, Pi);        
        Gi = P->parent; G = PP_NODE(db, Gi);        
      }
      /* case 4, step 2 */
      assert(Pi != PP_NIL && Gi != PP_NIL);
      if(Ni == P->left) 
        pp_rotate_right(db, Gi);
      else
        pp_rotate_left(db, Gi);
      P->color = PP_BLACK;
      G->color = PP_RED;
    }
  }
}
#endif /* PP_USE_PP_RED_PP_BLACK */

static void pp_insert(ppdb_t *db, const char *key, pp_index *Ri, pp_index Ni, pp_index Pi) {
  pp_node_t *N;
  if(*Ri == PP_NIL) {
    *Ri = Ni;
    N = PP_NODE(db, Ni);
    N->parent = Pi;

#if PP_USE_PP_RED_PP_BLACK
    N->color = PP_RED;
    pp_rebalance(db, Ni);
    /*
    while(N->parent != PP_NIL) {
      Ni = N->parent;
      N = PP_NODE(db, Ni);
    }
    db->root = Ni;
    */
#endif    

  } else {
    int comp;  
    N = PP_NODE(db, *Ri);
    comp = pp_strcmp(key, pp_svalue(db, N->key));
    assert(comp != 0); /* should've been caught earlier */
    if(comp < 0)
      pp_insert(db, key, &N->left, Ni, *Ri);
    else
      pp_insert(db, key, &N->right, Ni, *Ri);
  }
}


pp_err_t pp_poke(ppdb_t *db, const char *key, const char *value) {
  pp_node_t *N;
  pp_index Ni;
  pp_err_t r = PP_ERROR;

  assert(db->k == PP_NIL && db->v == PP_NIL);
  
  db->v = pp_strdup(db, value);
  if(db->v == PP_NIL)
    return PP_MEMORY;

  Ni = pp_find(db, key, db->root);
  if(Ni != PP_NIL) {
    N = PP_NODE(db, Ni);
    assert(N->obj.type == PP_NODE);    
    N->value = db->v;
  } else {  
    db->k = pp_strdup(db, key);
    if(db->k == PP_NIL) {
      r = PP_MEMORY;
      goto finally;    
    }
    
    Ni = pp_alloc(db, PP_NODE, sizeof *N);
    if(Ni == PP_NIL) {
      r = PP_MEMORY;
      goto finally;
    }
      
    N = PP_NODE(db, Ni);
    N->left = N->right = PP_NIL;
    N->key = db->k;
    N->value = db->v;
    pp_insert(db, key, &db->root, Ni, PP_NIL);    
  }
  r = PP_OK;
finally:
  db->k = db->v = PP_NIL;
  return r;
}

const char *pp_peek(ppdb_t *db, const char *key) {
  pp_node_t *N;
  pp_index Ni = pp_find(db, key, db->root);
  if(Ni == PP_NIL) 
    return NULL;
  N = PP_NODE(db, Ni);
  assert(N->obj.type == PP_NODE);    
  return pp_svalue(db, N->value);  
}

#pragma pack(push, 1)
typedef struct {
  char magic[4];
  pp_index bom, root;
  pp_index bump, mem_size;
} pp_header_t;
#pragma pack(pop)

pp_err_t pp_save(ppdb_t *db, FILE *f) {
  pp_header_t header;
  strncpy(header.magic, "PPD", 4);
  header.bom = 0xFEFF;
  header.root = db->root;
  header.bump = db->bump;
  header.mem_size = db->mem_size;
  if(fwrite(&header, sizeof header, 1, f) != 1)
    return PP_FWRITE;
  if(fwrite(db->memory, db->bump, 1, f) != 1)
    return PP_FWRITE;
  return PP_OK;
}

static pp_index pp_fix_uint(pp_index in) {
    return ((in >> 8) & 0xFF) + ((in & 0xFF) << 8);
}

static void pp_fix_endianess(ppdb_t *db) {
  pp_index ptr;
  pp_object_t *obj;
  for(ptr = 0; ptr < db->bump; ptr += obj->size) {
    obj = PP_PTR(db, ptr);
    obj->mark = pp_fix_uint(obj->mark);
    obj->size = pp_fix_uint(obj->size);
    if(obj->type == PP_NODE) {
      pp_node_t *N = (pp_node_t*)obj;
      N->parent = pp_fix_uint(N->parent);
      N->left = pp_fix_uint(N->left);
      N->right = pp_fix_uint(N->right);
      N->key = pp_fix_uint(N->key);
      N->value = pp_fix_uint(N->value);
    }
  }
}

pp_err_t pp_load(ppdb_t *db, FILE *f) {
  pp_header_t header;
  int endian = 0;

  if(fread(&header, sizeof header, 1, f) != 1) {
    return PP_FREAD;
  }

  if(strcmp(header.magic, "PPD")) {
    return PP_FILE;
  }

  if(header.bom != 0xFEFF) {
    if(header.bom == 0xFFFE) {
      header.bump = pp_fix_uint(header.bump);
      header.root = pp_fix_uint(header.root);
      endian = 1;
    } else {
      return PP_FILE;
    }
  }

  if(header.bump > db->mem_size) {
    return PP_MEMORY;
  }

  /* Reinitialize the database */
  db->bump = 0;  
  db->root = db->k = db->v = PP_NIL;

  if(fread(db->memory, header.bump, 1, f) != 1) {
    return PP_FREAD;
  }

  db->bump = header.bump;
  db->root = header.root;

  if(endian) pp_fix_endianess(db);
  
  return PP_OK;
}

static pp_index pp_leftmost(ppdb_t *db, pp_index Ni) {
  pp_node_t *N = PP_NODE(db, Ni);
  while(N->left != PP_NIL) {
    Ni = N->left;
    N = PP_NODE(db, Ni);
  }
  return Ni;
}

const char *pp_next(ppdb_t *db, const char *key) {
  pp_node_t *N;
  pp_index x, y;
  if(db->root == PP_NIL) 
    return NULL;
  if(!key)
    x = pp_leftmost(db, db->root);
  else {
    x = pp_find(db, key, db->root);
    N = PP_NODE(db, x);
    if(N->right != PP_NIL) {
      x = pp_leftmost(db, N->right);
    } else {
      for(y = N->parent; y != PP_NIL; y = N->parent) {
        N = PP_NODE(db, y);
        if(x != N->right) break;
        x = y;
      }
      x = y;
    }
  }
  if(x == PP_NIL)
    return NULL;
  N = PP_NODE(db, x);
  return pp_svalue(db, N->key);
}

static void pp_iterate(ppdb_t *db, pp_index Ni, pp_iterfun_t fun, void *cookie) {  
  pp_node_t *node;
  if(Ni == PP_NIL) return;
  node = PP_NODE(db, Ni);
  pp_iterate(db, node->left, fun, cookie);
  fun(pp_svalue(db, node->key), pp_svalue(db, node->value), cookie);
  pp_iterate(db, node->right, fun, cookie);
}

void pp_foreach(ppdb_t *db, pp_iterfun_t fun, void *cookie) {
  pp_iterate(db, db->root, fun, cookie);
} 

static void pp_show_tree(ppdb_t *db, pp_index Ni, int level, char c) {
  pp_node_t *node;
  if(Ni == PP_NIL) return;
  node = PP_NODE(db, Ni);
  printf("%*c- %s: %s\n", level * 4, c, pp_svalue(db, node->key), pp_svalue(db, node->value));
  pp_show_tree(db, node->left, level + 1, '<');
  pp_show_tree(db, node->right, level + 1, '>');
}

void pp_tree(ppdb_t *db) {
  pp_show_tree(db, db->root, 0, '*');
}

#undef left
#undef right
#undef color

#endif /* defined(PPDB_IMPLEMENTATION) */

#if defined(PPDB_TEST)

static void showfun(const char *key, const char *value, void *cookie) {
  (void)cookie;
  printf("'%s' => '%s'\n", key, value);
}

#define MEM_SIZE  8192
char memory[MEM_SIZE];

#if !DEBUG_GC
int main(int argc, char *argv[]) {

  ppdb_t DB;
  FILE *f;
  const char *key;

  (void)argc;
  (void)argv;
  pp_init(&DB, memory, MEM_SIZE);

  pp_poke(&DB, "xavier", "1");
  pp_poke(&DB, "bob",    "2");
  pp_poke(&DB, "mallory","3");
  pp_poke(&DB, "dave",   "4");
  pp_poke(&DB, "grace",  "5");
  pp_poke(&DB, "nelly",  "6");
  pp_poke(&DB, "judy",   "7");
  pp_poke(&DB, "trudy",  "8");
  pp_poke(&DB, "chuck",  "9");
  pp_poke(&DB, "peter",  "10");
  pp_poke(&DB, "trent",  "11");
  pp_poke(&DB, "chad",   "12");
  pp_poke(&DB, "ricky",  "13");
  pp_poke(&DB, "olivia", "14");
  pp_poke(&DB, "heidi",  "15");
  pp_poke(&DB, "victor", "16");
  pp_poke(&DB, "carol",  "17");
  pp_poke(&DB, "frank",  "18");
  pp_poke(&DB, "oscar",  "19");
  pp_poke(&DB, "quintin","20");
  pp_poke(&DB, "steve",  "21");
  pp_poke(&DB, "faythe", "22");
  pp_poke(&DB, "alice",  "23");
  pp_poke(&DB, "eve",    "24");
  pp_poke(&DB, "ivan",   "25");
  pp_poke(&DB, "evelyn", "26");
  pp_poke(&DB, "faith",  "27");
  pp_poke(&DB, "fanny",  "28");
  pp_poke(&DB, "eddie",  "29");
  pp_poke(&DB, "edward", "30");
  pp_poke(&DB, "edd",    "31");
  pp_poke(&DB, "eddy",   "32");
  pp_poke(&DB, "eustice","33");

  pp_poke(&DB, "Peter",  "10*");
  pp_poke(&DB, "ivan",   "25*");
  pp_poke(&DB, "Trudy",  "8*");
  pp_poke(&DB, "Nelly",  "6*");
  pp_poke(&DB, "ricky",  "13*");
  pp_poke(&DB, "Ivan",   "25**");
  pp_poke(&DB, "judy",   "7*");
  pp_poke(&DB, "Olivia", "14*");

  pp_compact(&DB);

  f = fopen("test.db", "wb");
  pp_save(&DB, f);
  fclose(f);
  
  f = fopen("test.db", "rb");
  pp_load(&DB, f);
  fclose(f);

  printf("show:\n");
  pp_foreach(&DB, showfun, NULL);

  printf("for loop:\n");
  for(key = pp_next(&DB, NULL); key; key = pp_next(&DB, key)){
    printf(" - key: '%s';\tvalue: '%s'\n", key, pp_peek(&DB, key));
  }

  pp_tree(&DB);
  
  return 0;
}
#else
int main(int argc, char *argv[]) {
  FILE *f;
  ppdb_t DB;

  pp_init(&DB, memory, MEM_SIZE);

  pp_poke(&DB, "alice", "1");
  pp_poke(&DB, "bob", "2");
  pp_poke(&DB, "carol", "3");

  pp_poke(&DB, "alice", "4");
  pp_poke(&DB, "bob", "5");
  pp_poke(&DB, "carol", "6");
  
  pp_compact(&DB);

  f = fopen("GC.db", "wb");
  pp_save(&DB, f);
  fclose(f);

  printf("show:\n");
  pp_foreach(&DB, showfun, NULL);
  
  return 0;
}
#endif
#endif /* defined(PPDB_TEST) */

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* PPDB_H */

/**
 * License
 * -------
 *
 * Copyright (C) 2023, Werner Stoop <wstoop@gmail.com> 
 *
 * Copying and distribution of this file, with or without modification, are permitted 
 * in any medium without royalty, provided the copyright notice and this notice are 
 * preserved. This file is offered as-is, without any warranty.
 * 
 * [FSF-APL]: https://www.gnu.org/prep/maintain/html_node/License-Notices-for-Other-Files.html
 */

