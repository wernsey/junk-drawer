/**
 *    **A tiny BASIC interpreter**
 *
 * From Chapter 7 of "C Power User's Guide" by Herbert Schildt, 1988.
 * The book can be found on [the Internet Archive](https://archive.org/details/cpowerusersguide00schi_0/mode/2up).
 *
 * This version has been modified to add several features:
 *
 * - Converted it to C89 standard
 * - Extracted the code as an API so you can script other programs with it
 * - Call C-functions
 * - Long variable names
 * - Identifiers as labels, denoted as `@label`
 * - `string$` variables...
 *   - ...and string literals in expressions
 * - Logical operators `AND`, `OR` and `NOT`
 * - `ON` statements
 * - Comments with the `REM` keyword and `'` operator
 * - Additional comparison operators `<>`, `<=` and `>=`
 * - Escape sequences in string literals
 */

#ifndef SBASIC_H
#define SBASIC_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * C API
 * =====
 *
 * Types
 * -----
 *
 * `value_t`
 * :    A structure representing an integer or string value.
 */

typedef struct value {
	enum {V_INT, V_STR} type;
	union {
		int i;
		char *s;
	} v;
} value_t;

/**
 * Globals
 * -------
 *
 */

typedef void (*sb_print_t)(const char *, ...);

extern sb_print_t sb_print_error;
extern sb_print_t sb_print;

/**
 * Functions
 * ---------
 *
 * ### Binding C functions
 *
 * * `void add_function(const char *name, sb_function_t fun);`
 *
 * Adds a function named `name` implemented by the function pointed to
 * by `fun` to the interpreter.
 *
 * `sb_function_t` is defined like so:
 *
 * ```c
 * typedef void (*sb_function_t)(value_t *result, int argc, value_t argv[]);
 * ```
 *
 * where `result` is a pointer to the `value` structure where the
 * result will be stored, `argc` is the number of arguments and
 * `argv` is an array of the aruments' values themselves
 *
 * * `void add_std_library()`
 *
 * Adds all the standard functions to the interpreter.
 *
 */

typedef void (*sb_function_t)(value_t *result, int argc, value_t argv[]);

void add_function(const char *name, sb_function_t fun);

void add_std_library();

/**
 * ### Utility functions
 *
 */
void sb_error(const char *error);
char *sb_talloc(int len);
char *sb_strdup(const char *s);

/**
 * ### Manipulating Values
 */
int as_int(value_t *val);
const char *as_string(value_t *val);

value_t make_int(int i);
value_t make_strn(const char *s, size_t len);
value_t make_str(const char *s);

value_t *get_variable(const char *var);

value_t *set_variable(const char *var, const char *val);
value_t *set_variablei(const char *name, int val);

/**
 * ### Loading Programs
 *
 * `char *load_program(char *fname)`
 */
char *load_program(const char *fname);

/**
 * ### Executing the interpreter
 *
 * `int execute(char *program);`
 *
 * Executes `program`.
 *
 * `int sb_gosub(const char *sub);`
 *
 * Finds `sub` and executes it.
 *
 */
int execute(char *program);

int sb_gosub(const char *sub);

int sb_line(void);

void sb_clear(void);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* SBASIC_H */
