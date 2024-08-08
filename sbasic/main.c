#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* -std=c89 does not like getopt
#if !defined(_WIN32)
#  include <unistd.h>
#else
#  include "getopt.h"
#endif
*/
#include "getopt.h"

#include "sbasic.h"

#define PPDB_IMPLEMENTATION
#include "ppdb.h"

#define SEQIO_IMPL
#include "seqio.h"

#define MAX_SUBS	16

/* See `listfuns.c` */
void add_list_library();

static void usage(const char *name, FILE *f) {
	fprintf(f, "usage: %s [options] <filename>\n", name);
	fprintf(f, "where [options] can be:\n");
	fprintf(f, "   -s var=value  : set variable before executing\n");
	fprintf(f, "   -g var        : get variable after executing\n");
	fprintf(f, "   -d dbfile     : specify database file\n");
	fprintf(f, "   -u subroutine : call subroutine after execing\n");
}

static ppdb_t DB;
static char db_buffer[8192];

/**
 * Database Functions
 * ------------------
 *
 * `poke(key, value)`
 * :    Stores the `value` associated with `key` in the built-in
 * :    key-value database.
 */
static void poke_function(struct value *result, int argc, struct value argv[]) {
	if(argc < 2) return;
	pp_poke(&DB, as_string(&argv[0]), as_string(&argv[1]));
}

/**
 * `peek(key)`
 * :    Retrieves the value associated with `key` from the built-in
 * :    key-value database.
 */
static void peek_function(struct value *result, int argc, struct value argv[]) {
	const char *v;
	if(argc < 1) return;
	v = pp_peek(&DB, as_string(&argv[0]));
	if(v)
		*result = make_str(v);
}

/**
 * Sequential IO Functions
 * -----------------------
 *
 * `open(filename, mode)`
 * :    Opens a file named `filename` for reading (`mode` = "r") or 
 * :    writing (`mode` = "w") sequential data.
 * :
 * :    It returns a file number `file#` that is used with subsequent
 * :    calls.
 */
#define MAX_FILES 4
static SeqIO files[MAX_FILES];
static int nfiles = 0;
 
static void open_function(struct value *result, int argc, struct value argv[]) {
	const char *n, *m;
	int i = 0, mode;
	if(argc < 2) sb_error("OPEN requires a filename and a mode");
	n = as_string(&argv[0]);
	m = as_string(&argv[1]);	
	
	mode = tolower(m[0]);
	if(mode != 'r' && mode != 'w') 
		sb_error("OPEN(): Mode must be 'r' or 'w'");
	
	if(nfiles < MAX_FILES) 
		i = nfiles++;
	else {
		for(i = 0; i < MAX_FILES && files[i].data; i++);
		if(i == MAX_FILES)
			sb_error("Too many open files");
	}
	if(mode == 'r') {
		if(!seq_infile(&files[i], n))
			sb_error("unable to open file");
	} else {
		if(!seq_outfile(&files[i], n))
			sb_error("unable to open file");
	}
	*result = make_int(i);
}

/**
 * `close(file#)`
 * :    Closes a file previously opened with `open()`
 */
static void close_function(struct value *result, int argc, struct value argv[]) {
	int i;
	if(argc < 1) sb_error("CLOSE requires a file#");
	i = as_int(&argv[0]);
	if(i < 0 || i >= MAX_FILES || !files[i].data)
		sb_error("CLOSE: invalid file#");	
	seq_close(&files[i]);
}

/**
 * `read(file#)`
 * :    Reads a value from a file
 */
static void read_function(struct value *result, int argc, struct value argv[]) {
	int i;
	const char *val, *err;
	SeqIO *file;
	if(argc < 1) sb_error("READ requires a file#");
	i = as_int(&argv[0]);
	if(i < 0 || i >= MAX_FILES || !files[i].data)
		sb_error("READ: invalid file#");
	file = &files[i];
	if(argc > 1) {
		for(i = 1; i < argc; i++) {
			val = seq_read(file);
			if((err = seq_error(file)))
				sb_error(err);
			if(!set_variable(as_string(&argv[i]), val))
				sb_error("Unable to set variable");			
		}
	} else {
		val = seq_read(file);
		if((err = seq_error(file)))
			sb_error(err);
	}
	*result = make_str(val);
}

/**
 * `write(file#, val1, val2, ...valn)`
 * :    Writes the values `val1` to `valn` sequentially to the file.
 */
static void write_function(struct value *result, int argc, struct value argv[]) {
	int i, n;
	const char *err;
	if(argc < 1) sb_error("READ requires a file#");
	i = as_int(&argv[0]);
	if(i < 0 || i >= MAX_FILES || !files[i].data)
		sb_error("READ: invalid file#");
	for(n = 1; n < argc; n++) {
		if(argv[n].type == V_INT)
			seq_write_int(&files[i], as_int(&argv[n]));
		else
			seq_write(&files[i], as_string(&argv[n]));
		if((err = seq_error(&files[i])))
			sb_error(err);
	}		
	seq_endl(&files[i]);
}

/**
 * Additional Functions
 * --------------------
 *
 * `call(subroutine)`
 * :    Calls the specified `subroutine`
 * :
 * :    `CALL &sub` is functionally equivalent to `GOSUB sub` (albeit
 * :    with a bit more overhead).
 */
static void call_function(struct value *result, int argc, struct value argv[]) {
	int res;
	if(argc < 1) sb_error("CALL requires a subroutine");
	res = sb_gosub(as_string(&argv[0]));
	*result = make_int(res);
}

int main(int argc, char *argv[]) {
	char *p_buf;	
	int c, i;
	const char *getter = NULL, *dbfile = NULL;
	const char *subs[MAX_SUBS];
	int nsubs = 0;

	while ((c = getopt(argc, argv, "s:g:d:u:")) != -1) {
		switch (c) {
			case 's': {
				char *var = optarg, *val;
				val = strchr(var, '=');
				if(val) {
					*val = '\0';
					val++;
				} else
					val = "";
				if(!set_variable(var, val)) {
					fprintf(stderr, "Unable to set `%s`\n", var);
					return 1;
				}					
			} break;
			case 'g': getter = optarg; break;
			case 'd':
				dbfile = optarg;
				break;
			case 'u':
				if(nsubs >= MAX_SUBS) {
					fprintf(stderr, "Too many subs (max %d)\n", MAX_SUBS);
					return 1;
				}
				subs[nsubs++] = optarg;
				break;
			default: usage(argv[0], stderr); return 1;
		}
	}		

	if(optind == argc) {
		usage(argv[0], stderr);
		return 1;
	}
	
	add_std_library();
	add_list_library();
	
	add_function("peek", peek_function);
	add_function("poke", poke_function);
	add_function("open", open_function);
	add_function("close", close_function);
	add_function("read", read_function);
	add_function("write", write_function);
	add_function("call", call_function);

	pp_init(&DB, db_buffer, sizeof db_buffer);
	if(dbfile) {
		FILE *f = fopen(dbfile, "rb");
		if(f) {
			if(pp_load(&DB, f) != PP_OK) {
				fprintf(stderr, "error: couldn't read database");
				return 1;
			}
			fclose(f);
		}
	}

	if(!(p_buf = load_program(argv[optind]))) {		
		fprintf(stderr, "couldn't load %s\n", argv[optind]);
		return 1;
	}

	if(!execute(p_buf)) {
		fprintf(stderr, "execution failed.\n");
		return 1;	
	}
	
	for(i = 0; i < nsubs; i++) {
		int result = sb_gosub(subs[i]);
		if(!result) 
			break;
	}
	
	if(getter) {		
		struct value *val = get_variable(getter);
		if(val)
			printf("%s = '%s'\n", getter, as_string(val));
		else
			printf("No variable `%s` in script\n", getter);
	}
	
	if(dbfile) {
		FILE *f = fopen(dbfile, "wb");
		if(f) {
			if(pp_save(&DB, f) != PP_OK) {
				fprintf(stderr, "error: couldn't write database");
			}
			fclose(f);
		} else {
			fprintf(stderr, "error: couldn't open database for output");
		}
	}
	
	free(p_buf);

	return 0;
}
