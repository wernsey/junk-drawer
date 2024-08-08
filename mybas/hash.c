#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "hash.h"

void ht_init(hash_table tbl) {
    int i;
    for(i = 0; i < HASH_SIZE; i++)
        tbl[i] = NULL;
}

static void free_element(hash_element* v, void (*cfun)(void *)) {
    if(!v) return;
    free_element(v->next, cfun);
    if(cfun) cfun(v->data);
    free(v->name);
    free(v);
}

void ht_destroy(hash_table tbl, void (*cfun)(void *)) {
    int i;
    for(i = 0; i < HASH_SIZE; i++)
        free_element(tbl[i], cfun);
}

static unsigned int hash(const char *s) {
    unsigned int i = 0x5555;
    for(;s[0];s++)
        i = i << 3 ^ (s[0] | (s[0] << 8));
    return i % HASH_SIZE;
}

static hash_element *search(hash_table tbl, const char *name, int *bucket) {
    hash_element* v;
    unsigned int h = hash(name);
    if(tbl[h]) {
        for(v = tbl[h]; v; v = v->next)
            if(!strcmp(v->name, name)) {
                if(bucket) *bucket = h;
                return v;
            }
    }
    return NULL;
}

void *ht_get(hash_table tbl, const char *name) {
    struct hash_element* v = search(tbl, name, NULL);
    if(v)
        return v->data;
    return NULL;
}

void ht_put(hash_table tbl, const char *name, void *val) {
    hash_element* v = search(tbl, name, NULL);
    if(v) /* duplicates not allowed */
        return;
    v = malloc(sizeof *v);
    unsigned int h = hash(name);
    v->name = strdup(name);
    v->data = val;
    v->next = tbl[h];
    tbl[h] = v;
}

const char *ht_next(hash_table tbl, const char *key) {
    int f;
    hash_element *i;

    if (key == NULL) {
        for (f = 0; f < HASH_SIZE && !tbl[f]; f++);
        if (f >= HASH_SIZE)
            return NULL;
        assert (tbl[f]);
        return tbl[f]->name;
    }
    i = search(tbl, key, &f);
    if (!i)
        return NULL;
    if (i->next)
        return i->next->name;
    else {
        f++;
        while (f < HASH_SIZE && !tbl[f])
            f++;
        if (f >= HASH_SIZE)
            return NULL;
        assert (tbl[f]);
        return tbl[f]->name;
    }
}
