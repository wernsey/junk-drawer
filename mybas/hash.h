#define HASH_SIZE 31 // Must be prime

typedef struct hash_element {
	char *name;
	void *data;
	struct hash_element *next;
} hash_element;

typedef hash_element *hash_table[HASH_SIZE];

void ht_init(hash_table tbl);
void ht_destroy(hash_table tbl, void (*cfun)(void *));
void *ht_get(hash_table tbl, const char *name);
void ht_put(hash_table tbl, const char *name, void *val);

const char *ht_next(hash_table tbl, const char *key);