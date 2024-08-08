#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "sbasic.h"

/**
 * List Functions
 * --------------
 * 
 * The `listfuns.c` module provides some functions for
 * representing lists using strings of comma-separated values. 
 * 
 * For example, `"foo,bar,baz"` is a list of three values.
 * 
 * !!! warning
 *     I'm obligated to tell you that these functions
 *     are $O(n)$ in complexity.
 * 
 */
#define FS "," 

/**
 * `list(item1, item2...)`
 * :    Creates a list of all the items in its arguments
 */
static void list_function(struct value *result, int argc, struct value argv[]) {
	int len = 0, i, p = 0;
	if(!argc) return;
	for(i = 0; i < argc; i++)
		len += strlen(as_string(&argv[i])) + 1;
	assert(len > 0);
	result->type = V_STR;
	result->v.s = sb_talloc(len);
	for(i = 0; i < argc; i++) {
		const char *s = as_string(&argv[i]);
		int t = strlen(s);
		memcpy(result->v.s + p, s, t);
		p += t;
		result->v.s[p++] = FS[0];
	}
	assert(p == len);
	result->v.s[len - 1] = '\0';
}

/**
 * `ladd(list$, item)`
 * :    Appends `item` to the end of `list$`
 */
static void ladd_function(struct value *result, int argc, struct value argv[]) {
	int len = 0, l1, l2;
	if(argc < 2) 
		sb_error("LADD expects two arguments");
	l1 = strlen(as_string(&argv[0]));
	l2 = strlen(as_string(&argv[1]));
	len = l1 + l2 + 1;
	result->type = V_STR;
	result->v.s = sb_talloc(len + 1);
	memcpy(result->v.s, as_string(&argv[0]), l1);
	result->v.s[l1] = FS[0];
	memcpy(result->v.s + l1 + 1, as_string(&argv[1]), l2);
	result->v.s[len] = '\0';
}

/**
 * `llen(list$)`
 * :    Returns the number of items in `list$`
 */
static void llen_function(struct value *result, int argc, struct value argv[]) {
	int i = 0;
	char *s, *t;
	if(argc < 1) return;
	s = sb_strdup(as_string(&argv[0]));
	for(t = strtok(s, FS); t; t = strtok(NULL, FS), i++);
	*result = make_int(i);
}

/**
 * `lget(list$, n)`
 * :    Returns the `n`th item in `list$`
 */
static void lget_function(struct value *result, int argc, struct value argv[]) {
	int i = 1, n;
	char *s, *t;
	if(argc < 2) return;
	s = sb_strdup(as_string(&argv[0]));
	n = as_int(&argv[1]);
	for(t = strtok(s, FS); t; t = strtok(NULL, FS), i++) {
		if(i == n) {	
			*result = make_str(t);
			return;
		}	
	}
}

/**
 * `lfind(list$, i$)`
 * :    Returns the index of `i$` in `list$`, or 0 if not found
 */
static void lfind_function(struct value *result, int argc, struct value argv[]) {
	int i = 1;
	char *s, *t;
	const char *n;
	if(argc < 2) return;
	s = sb_strdup(as_string(&argv[0]));
	n = as_string(&argv[1]);
	for(t = strtok(s, FS); t; t = strtok(NULL, FS), i++) {
		if(!strcmp(t, n)) {	
			*result = make_int(i);
			return;
		}	
	}
	*result = make_int(0);
}

/**
 * `lhead(list$)`
 * :    Returns the first item in `list$`
 */
static void lhead_function(struct value *result, int argc, struct value argv[]) {
	char *s, *t;
	if(argc < 1) return;
	s = sb_strdup(as_string(&argv[0]));
	t = strchr(s, FS[0]);
	if(t)
		*t = '\0';
	*result = make_str(s);
}

/**
 * `ltail(list$)`
 * :    Returns all items in `list$` except the first.
 */
static void ltail_function(struct value *result, int argc, struct value argv[]) {
	char *s, *t;
	if(argc < 1) return;
	s = sb_strdup(as_string(&argv[0]));
	t = strchr(s, FS[0]);
	if(t) {
		t++;
		*result = make_str(t);
	}
}

void add_list_library() {
	add_function("list", list_function);
	add_function("ladd", ladd_function);
	add_function("llen", llen_function);
	add_function("lget", lget_function);
	add_function("lfind", lfind_function);
	add_function("lhead", lhead_function);
	add_function("ltail", ltail_function);
}
