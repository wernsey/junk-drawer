/* See sbasic.h for information */
#include <stdio.h>
#include <setjmp.h>
#include <math.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>
#include <assert.h>

#include "sbasic.h"

#define NUM_LAB		100
#define LAB_LEN		10
#define FOR_NEST	25
#define SUB_NEST	25
#define MAX_FUNCTIONS	64
#define MAX_ARGS	16
#define TOKEN_SIZE  80

#define NAME_LEN     16
#define NUM_VARS     64

/* FIXME: There's a better way to deal with this */
#define SNPRINTF 0

/* Total number of bytes available for
 * string expressions and variables */
#ifndef STRINGS_SIZE
#  define STRINGS_SIZE 16384
#endif

/* Bytes reserved for each individual string
 * variable */
#ifndef STRING_LEN
#  define STRING_LEN   128
#endif

/* Do we truncate strings if they're too
 * long to fit into a variable, or raise
 * an error */
#ifndef TRUNCATE_STRINGS
#  define TRUNCATE_STRINGS 0
#endif

#ifndef PRINT_STMT
#define PRINT_STMT 1
#endif
#ifndef INPUT_STMT
#define INPUT_STMT 1
#endif

enum {
/* 0 is reserved */
 FINISHED = 1,
 IDENTIFIER,
 FUNCTION,
 NUMBER,
 STRING,
#if PRINT_STMT
 PRINT,
#endif
#if INPUT_STMT
 INPUT,
#endif
 IF,
 THEN,
 FOR,
 NEXT,
 TO,
 GOTO,
 EOL,
 GOSUB,
 RETURN,
 END,
 REM,
 NE, LE, GE,
 AND, OR, NOT,
 ON
};

struct label {
	char name[LAB_LEN];
	char *p;
	int line;
};

struct for_stack {
	struct variable *var;
	int target;
	char *loc;
	int line;
};

struct gloc {
	char *loc;
	int line;
};

struct variable {
	char name[NAME_LEN];
	value_t value;
};

static struct commands {
	const char *command;
	char tok;
} table[] = { /* Commands must be entered in lowercase */
#if PRINT_STMT
  {"print", PRINT},
#endif
#if INPUT_STMT
  {"input", INPUT},
#endif
  {"if", IF},
  {"then", THEN},
  {"goto", GOTO},
  {"for", FOR},
  {"next", NEXT},
  {"to", TO},
  {"gosub", GOSUB},
  {"return", RETURN},
  {"end", END},
  {"rem", REM},
  {"and", AND},
  {"or", OR},
  {"not", NOT},
  {"on", ON},
  {"", END}
};

static struct  {
	const char *name;
	sb_function_t fun;
} functions[MAX_FUNCTIONS];
static int nfuns = 0;
sb_function_t tocall;

static char *prog = NULL, *prog_save;

static jmp_buf e_buf;
static int has_jmp = 0;

static char strings[STRINGS_SIZE];
static int string_base = 0, string_bump;

static struct variable variables[NUM_VARS];
static int nvars = 0;

static char token[TOKEN_SIZE], token_type, *str_ptr;
static int curr_line = 0;

static struct label label_table[NUM_LAB];

/* For-loop stack */
static struct for_stack fstack[FOR_NEST];
static int ftos;

/* Gosub stack */
static struct gloc gstack[SUB_NEST];
static int gtos;

static struct label *find_label(const char *s);
static int get_token();
static void get_exp(value_t *result);
static void putback();
static void find_eol();
static int get_next_label(char *s);

#define PRINT_FUN(NAME, FILE) static void NAME(const char *fmt, ...) {	\
	va_list arg;					\
    va_start(arg, fmt);				\
    vfprintf(FILE, fmt, arg);    	\
    va_end(arg);					\
}
PRINT_FUN(default_print, stdout)
PRINT_FUN(default_print_error, stderr)

sb_print_t sb_print = default_print;
sb_print_t sb_print_error = default_print_error;

char *load_program(const char *fname) {
	FILE *fp;
	unsigned long len;
	char *p;

	fp = fopen(fname, "rb");
	if(!fp) return NULL;

	fseek(fp, 0, SEEK_END);
	len = ftell(fp);

	p = malloc(len+1);
	if(!p) goto error;

	fseek(fp, 0, SEEK_SET);
	if(fread(p, 1, len, fp) != len) goto error;

	p[len] = '\0';

error:
	fclose(fp);
	return p;
}

static struct variable *find_var(const char *name, int create) {
	int i;
	struct variable *var;
	for(i = 0; i < nvars; i++)
		if(!strcmp(variables[i].name, name))
			return &variables[i];
	if(!create)
		return NULL;
	if(nvars == NUM_VARS)
		sb_error("too many variables");
	var = &variables[nvars++];
	if(strlen(name) >= NAME_LEN)
		sb_error("variable name too long");
	strcpy(var->name, name);
	if(strchr(name, '$')) {
		var->value.type = V_STR;
		var->value.v.s = &strings[string_base];
		string_base += STRING_LEN;
		if(string_base > STRINGS_SIZE)
			sb_error("too many strings");
		var->value.v.s[0] = '\0';
	} else {
		var->value.type = V_INT;
		var->value.v.i = 0;
	}
	return var;
}

value_t *get_variable(const char *name) {
	struct variable *var = find_var(name, 0);
	if(!var) return NULL;
	return &var->value;
}

value_t *set_variable(const char *name, const char *val) {
	size_t len;
	struct variable *var = find_var(name, 1);
	if(!var)
		return NULL;
	len = strlen(val);
	assert(len < STRING_LEN);
	if(var->value.type == V_STR)
		memmove(var->value.v.s, val, len + 1);
	else
		var->value.v.i = atoi(val);
	return &var->value;
}

value_t *set_variablei(const char *name, int val) {
	size_t len = 16;
	struct variable *var = find_var(name, 1);
	if(!var)
		return NULL;
	assert(len < STRING_LEN);
	if(var->value.type == V_STR) {
		var->value.v.s = sb_talloc(len);
#if SNPRINTF
		snprintf(var->value.v.s, len, "%d", val);
#else
		sprintf(var->value.v.s, "%d", val);
#endif
	} else
		var->value.v.i = val;
	return &var->value;
}

char *sb_talloc(int len) {
	char *s;
	if(len & 0x1) len++;
	if(string_bump + len >= STRINGS_SIZE)
		sb_error("too complex strings");
	s = &strings[string_bump];
	string_bump += len;
	return s;
}

char *sb_strdup(const char *s) {
	char *o;
	size_t len = strlen(s);
	o = sb_talloc(len + 1);
	memmove(o, s, len);
	o[len] = '\0';
	return o;
}

int as_int(value_t *val) {
	if(val->type == V_STR)
		return atoi(val->v.s);
	return val->v.i;
}

const char *as_string(value_t *val) {
	char *s;
	if(val->type == V_INT) {
		s = sb_talloc(16);
		sprintf(s, "%d", val->v.i);
		return s;
	}
	return val->v.s;
}

value_t make_int(int i) {
	value_t v;
	v.type = V_INT;
	v.v.i = i;
	return v;
}

value_t make_strn(const char *s, size_t len) {
	value_t v;
	v.type = V_STR;
	v.v.s = sb_talloc(len+1);
	memcpy(v.v.s, s, len);
	v.v.s[len] = '\0';
	return v;
}

value_t make_str(const char *s) {
	return make_strn(s, strlen(s));
}

void add_function(const char *name, sb_function_t fun) {
	if(nfuns == MAX_FUNCTIONS) {
		sb_print_error("error: too many functions\n");
		abort();
	}
	functions[nfuns].name = name;
	functions[nfuns].fun = fun;
	nfuns++;
}

/* Assign a variable a value. */
static void assignment() {
	value_t value;
	struct variable *var;
	const char *s;
	int len;

	get_token();
	if(token_type != IDENTIFIER) {
		sb_error("not a variable");
		return;
	}

	var = find_var(token, 1);

	get_token();
	if(*token != '=') {
		sb_error("equals sign expected");
		return;
	}

	get_exp(&value);
	if(var->value.type == V_INT)
		var->value.v.i = as_int(&value);
	else {
		s = as_string(&value);
		len = strlen(s);
		if(len > STRING_LEN - 1) {
#if TRUNCATE_STRINGS
			len = STRING_LEN - 1;
#else
			sb_error("string too long");
#endif
		}
		memmove(var->value.v.s, s, len);
		var->value.v.s[len] = '\0';
	}
}

/* Execute a simple version of the BASIC PRINT statement */
#if PRINT_STMT
static void print() {
	value_t answer;
	int len=0, spaces;
	char last_delim = '\0';

	do {
		get_token(); /* get next list item */
		if(token_type == EOL || token_type == FINISHED) break;
		putback();
		get_exp(&answer);

		if(answer.type == V_INT) {
			char buffer[STRING_LEN];
			sprintf(buffer, "%d", answer.v.i);
			len += strlen(buffer);
			sb_print("%s", buffer);
		} else {
			len += strlen(answer.v.s);
			sb_print("%s", answer.v.s);
		}

		get_token();
		last_delim = token_type;

		if(token_type == ';') {
			/* compute number of spaces to move to next tab */
			spaces = 8 - (len % 8);
			len += spaces; /* add in the tabbing position */
			while(spaces) {
				sb_print(" ");
				spaces--;
			}
		}
		else if(token_type == ',')
			/* do nothing */;
		else if(token_type != EOL && token_type != FINISHED)
			sb_error("syntax error");
	} while (token_type == ';' || token_type == ',');

	if(token_type == EOL || token_type == FINISHED) {
		if(last_delim != ';' && last_delim != ',')
			sb_print("\n");
	}
	else
		sb_error("syntax error");

	fflush(stdout);
}
#endif

/* Find all labels. */
static void scan_labels() {
	int addr, t;
	char *temp;

	curr_line = 1;
	for(t = 0; t < NUM_LAB; ++t) label_table[t].name[0] = '\0';

	temp = prog;   /* save pointer to top of program */

	do {
		get_token();
		if(token_type == NUMBER) {
			addr = get_next_label(token);
			strcpy(label_table[addr].name, token);
			label_table[addr].p = prog;
			label_table[addr].line = curr_line;
		} else if(token_type == '@') {
			get_token();
			if(token_type != IDENTIFIER)
				sb_error("identifier expected");
			addr = get_next_label(token);
			strcpy(label_table[addr].name, token);
			label_table[addr].p = prog;
			label_table[addr].line = curr_line;
		}
		if(token_type != EOL)
			find_eol();
	} while(token_type != FINISHED);
	prog = temp;
}

/* find the start of the next line. */
static void find_eol() {
	while(*prog != '\n' && *prog != '\0') ++prog;
	if(*prog) {
		curr_line++;
		prog++;
	}
}

/* Return index of next free position in label array. */
static int get_next_label(char *s) {
	int t;
	for(t = 0; t < NUM_LAB; ++t) {
		if(label_table[t].name[0] == 0)
			return t;
		if(!strcmp(label_table[t].name,s))
			sb_error("duplicate label");
	}
	sb_error("label table full");
	return 0;
}

static void or_expr(int *result), and_expr(int *result), not_expr(int *result), cond_expr(int *result);

/* Execute an IF statement. */
static void exec_if() {
	int cond;
	or_expr(&cond);
	if(cond) {
		get_token();
		if(token_type != THEN)
			sb_error("THEN expected");
	} else
		find_eol();
}

static void or_expr(int *result) {
	int hold;
	and_expr(result);
	if(get_token() != EOL) putback();
	while(get_token() == OR) {
		and_expr(&hold);
		*result = *result || hold;
	}
	putback();
}

static void and_expr(int *result) {
	int hold;
	not_expr(result);
	if(get_token() != EOL) putback();
	while(get_token() == AND) {
		not_expr(&hold);
		*result = *result && hold;
	}
	putback();
}

static void not_expr(int *result) {
	if(get_token() == NOT) {
		cond_expr(result);
		*result = !*result;
	} else {
		putback();
		cond_expr(result);
	}
}

static void cond_expr(int *result) {
	value_t lhs, rhs;
	int op, comp;

	get_exp(&lhs);
	op = get_token();
	if(op == THEN || op == AND || op == OR) {
		putback();
		*result = as_int(&lhs);
		return;
	}
	get_exp(&rhs);

	if(lhs.type == V_INT)
		comp = as_int(&lhs) - as_int(&rhs);
	else
		comp = strcmp(as_string(&lhs), as_string(&rhs));
	switch(op) {
		case '<': *result = comp < 0; break;
		case '>': *result = comp > 0; break;
		case '=': *result = comp == 0; break;
		case NE: *result  = comp != 0; break;
		case LE: *result  = comp <= 0; break;
		case GE: *result  = comp >= 0; break;
		default: sb_error("syntax error");
	}
}

/* Execute a FOR loop. */
static void exec_for() {
	struct for_stack i;
	int nxt_cnt;
	value_t initial, target;

	get_token(); /* read the control variable */
	if(!isalpha(*token))
		sb_error("not a variable");

	i.var = find_var(token, 1);
	if(i.var->value.type == V_STR)
		sb_error("for-loop needs integer variable");

	get_token();
	if(*token != '=')
		sb_error("equals sign expected");

	get_exp(&initial);

	i.var->value.v.i = as_int(&initial);

	get_token();
	if(token_type != TO)
		sb_error("TO expected");

	get_exp(&target);
	i.target = as_int(&target);

	/* if loop can execute at least once, push info on stack */
	if(i.target >= i.var->value.v.i) {
		i.loc = prog;
		i.line = curr_line;
		if(ftos == FOR_NEST)
			sb_error("too many nested FOR loops");
		fstack[ftos++] = i;
	} else {
		/* otherwise skip the loop altogether, dealing with nested FORs in the process */
		nxt_cnt = 1;
		while(nxt_cnt > 0) {
			get_token();
			if(token_type == NEXT) nxt_cnt--;
			else if(token_type == FOR) nxt_cnt++;
			else if(token_type == REM)
				find_eol();
			else if(token_type == FINISHED)
				break;
		}
	}
}

static void next() {
	struct for_stack i;
	int j;

	if(ftos == 0)
		sb_error("NEXT without FOR");
	i = fstack[--ftos];

	j = i.var->value.v.i + 1;

	if(j > i.target) return;
	i.var->value.v.i = j;
	ftos++;
	prog = i.loc; /* loop */
	curr_line = i.line;
}

/* Execute a simple form of the BASIC INPUT command */
#if INPUT_STMT
static void input() {
	char s[STRING_LEN];
	struct variable *var;
	int i;

	get_token();
	if(token_type == STRING) {
		sb_print("%s", str_ptr);
		get_token();
		if(*token != ',')
			sb_error("',' expected");
		get_token();
	} else
		sb_print("? ");
	fflush(stdout);

	var = find_var(token, 1);
	if(!fgets(s, sizeof s, stdin))
		sb_error("no INPUT");
	for(i = 0; s[i]; i++)
		if(strchr("\r\n", s[i])) {
			s[i] = '\0';
			break;
		}
	if(var->value.type == V_INT) {
		var->value.v.i = atoi(s);
	} else {
		strcpy(var->value.v.s, s);
	}
}
#endif

/* Find location of given label.  A null is returned if
   label is not found; otherwise a pointer to the position
   of the label is returned
*/
static struct label *find_label(const char *s) {
	int t;
	for(t = 0; t < NUM_LAB; ++t)
		if(!strcmp(label_table[t].name,s))
			return &label_table[t];
	return NULL;
}

/* Execute a GOTO statement. */
static void exec_goto(const char *label_text) {
	struct label *label = find_label(label_text);
	if(!label)
		sb_error("undefined label");
	prog = label->p;
	curr_line = label->line;
}

/* Execute a GOSUB command. */
static void exec_gosub(const char *label_text) {
	struct label *label = find_label(label_text);
	if(!label)
		sb_error("undefined label");
	else {
		if(gtos == SUB_NEST)
			sb_error("too many nested GOSUBs");
		gstack[gtos].loc = prog;
		gstack[gtos++].line = curr_line;
		prog = label->p;
		curr_line = label->line;
	}
}

static void level2(value_t *), level3(value_t *);
static void level4(value_t *), level5(value_t *);
static void level6(value_t *), primitive(value_t *);

/* Entry point into parser. */
static void get_exp(value_t *result) {
	get_token();
	level2(result);
	putback();
}

/* Add or subtract two terms. */
static void level2(value_t *result) {
	value_t hold;
	int op;
	level3(result);
	while((op = token_type) == '+' || op == '-') {

		do {
			get_token();
		} while(token_type == EOL); /* Allow '\n' after the operator */

		level3(&hold);
		if(op == '+') {
			if(result->type == V_STR) {
				const char *s1 = as_string(result), *s2 = as_string(&hold);
				char *s;
				size_t l1 = strlen(s1), l2 = strlen(s2);
				s = sb_talloc(l1+l2+1);
				memcpy(s, s1, l1);
				memcpy(s + l1, s2, l2);
				s[l1+l2] = '\0';
				result->v.s = s;
			} else
				*result = make_int(as_int(result) + as_int(&hold));
		} else
			*result = make_int(as_int(result) - as_int(&hold));
	}
}

/* Multiply or divide two factors. */
static void level3(value_t *result) {
	value_t hold;
	int rhs, op;
	level4(result);
	while((op = token_type) == '*' || op == '/' || op == '%') {
		do {
			get_token();
		} while(token_type == EOL);
		level4(&hold);
		if(op == '*') {
			*result = make_int(as_int(result) * as_int(&hold));
		} else {
			if((rhs = as_int(&hold)) == 0)
				sb_error("divide by zero");
			if(op == '/')
				*result = make_int(as_int(result) / rhs);
			else
				*result = make_int(as_int(result) % rhs);
		}
	}
}

static void level4(value_t *result) {
	value_t hold;
	int ex, h;
	level5(result);
	while(token_type == '^') {
		do {
			get_token();
		} while(token_type == EOL);
		level4(&hold);
		ex = as_int(result);
		*result = make_int(1);
		for(h = as_int(&hold); h > 0; h--)
			result->v.i *= ex;
	}
}

static void level5(value_t *result) {
	char op = token_type;
	if(op == '+' || op == '-') {
		do {
			get_token();
		} while(token_type == EOL);
	}
	level6(result);
	if(op == '-')
		*result = make_int(-as_int(result));
}

static void level6(value_t *result) {
	if(token_type == '(') {
		do {
			get_token();
		} while(token_type == EOL);
		level2(result);
		if(token_type != ')')
			sb_error("unbalanced parentheses");
		get_token();
	} else
		primitive(result);
}

static void primitive(value_t *result) {
	struct variable *var;
	switch(token_type) {
	case IDENTIFIER:
		var = find_var(token, 0);
		if(var)
			*result = var->value;
		else if(strchr(token, '$'))
			*result = make_str("");
		else
			*result = make_int(0);
		get_token();
		return;
	case STRING:
		/* *result = make_str(str_ptr); */
		result->type = V_STR;
		result->v.s = str_ptr;
		get_token();
		return;
	case NUMBER:
		*result = make_int(atoi(token));
		get_token();
		return;
	case FUNCTION: {
		int argc = 0;
		value_t argv[MAX_ARGS];
		sb_function_t fun = tocall;
		*result = make_str("");
		if(get_token() != '(')
			sb_error("'(' expected");
		if(get_token() == ')')
			goto do_call;
		putback();
		do {
			get_exp(&argv[argc++]);
		} while (get_token() == ',');
do_call:
		if(token_type != ')')
			sb_error("')' expected");
		fun(result, argc, argv);
		get_token();
	} return;
	case '&': {
		if(get_token() != IDENTIFIER)
			sb_error("identifier expected");
		*result = make_str(token);
		get_token();
	} return;
	default:
		sb_error("bad expression");
	}
}

void sb_error(const char *error) {
	sb_print_error("error:%d: %s\n", curr_line, error);
	if(has_jmp)
		longjmp(e_buf, 1);
	abort();
}

/* Get a token. */
static int get_token() {
	char *temp;
	int i;

	prog_save = prog;
	token_type = 0;
	temp = token;
	if(*prog == '\0') {
		*token = '\0';
		return (token_type = FINISHED);
	}

	while(isspace(*prog) && *prog != '\n')
		++prog;

	prog_save = prog;
	if(*prog == '\n') {
		++prog;
		*temp++='\n';
		*temp='\0';
		curr_line++;
		return (token_type = EOL);
	} else if(strchr("+-*^/%=;(),><@&", *prog)) {
		if(!strncmp(prog, "<>", 2)) {
			*temp++ = *prog++;
			token_type = NE;
		} else if(!strncmp(prog, "<=", 2)) {
			*temp++ = *prog++;
			token_type = LE;
		} else if(!strncmp(prog, ">=", 2)) {
			*temp++ = *prog++;
			token_type = GE;
		} else {
			token_type = prog[0];
		}
		*temp++ = *prog++;
		*temp = '\0';
		return token_type;
	} else if(*prog=='\'') {
		*temp++ = *prog++;
		*temp = '\0';
		return (token_type = REM);
	} else if(*prog=='"') {

		str_ptr = temp = &strings[string_bump];

		prog++;
		while(*prog != '"') {
			if(*prog == '\\') {
				prog++;
				switch(*prog++) {
					case '\0':
					case '\r':
					case '\n': sb_error("unterminated string"); break;
					case 'a' : *temp++ = '\a'; break;
					case 'b' : *temp++ = '\b'; break;
					case 'e' : *temp++ = 0x1B; break;
					case 'f' : *temp++ = '\f'; break;
					case 'n' : *temp++ = '\n'; break;
					case 'r' : *temp++ = '\r'; break;
					case 't' : *temp++ = '\t'; break;
					case 'v' : *temp++ = '\v'; break;
					case 'x' : {
						for(*temp = 0, i = 0; i < 2 && isxdigit(*prog); i++, prog++)
							*temp = (*temp << 4) + (isdigit(*prog) ? *prog - '0' : tolower(*prog) - 'a' + 0xA);
						temp++;
					} break;
					default: *temp++ = *(prog - 1); break;
				}
			} else if(!*prog || strchr("\r\n", *prog)) {
				sb_error("unterminated string");
			} else
				*temp++ = *prog++;
			if(temp - strings >= STRINGS_SIZE)
				sb_error("strings too complicated");
		}
		prog++;
		*temp++ = '\0';

		string_bump = temp - strings;
		if(string_bump & 0x1) string_bump++;

		return (token_type = STRING);
	} else if(isdigit(*prog)) {
		while(isdigit(*prog)) *temp++ = *prog++;
		*temp = '\0';
		return (token_type = NUMBER);
	} else if(isalpha(*prog)) {
		while(isalnum(*prog) || *prog == '_')
			*temp++ = tolower(*prog++);
		if(*prog == '$')
			*temp++ = *prog++;
		*temp = '\0';
		for(i = 0; !token_type && *table[i].command; i++)
			if(!strcmp(table[i].command, token))
				token_type = table[i].tok;
		for(i = 0; !token_type && i < nfuns; i++)
			if(!strcmp(functions[i].name, token)) {
				token_type = FUNCTION;
				tocall = functions[i].fun;
			}
		if(!token_type)
			token_type = IDENTIFIER;
		return token_type;
	}
	sb_error("invalid token");
	return 0;
}

static void putback() {
	prog = prog_save;
	if(token_type == EOL)
		curr_line--;
}

static int execute_lines() {

	do {
		string_bump = string_base;
		/*printf("on line %d:\n", curr_line);*/
		switch(get_token()) {
		case '@': get_token();
			/* fallthrough */
		case NUMBER:
			break;
		case IDENTIFIER:
			putback();
			assignment();
			break;
#if PRINT_STMT
		case PRINT:
			print();
			break;
#endif
#if INPUT_STMT
		case INPUT:
			input();
			break;
#endif
		case IF:
			exec_if();
			break;
		case FOR:
			exec_for();
			break;
		case NEXT:
			next();
			break;
		case GOTO: {
			get_token();
			if(token_type != IDENTIFIER && token_type != NUMBER)
				sb_error("goto destination expected");
			exec_goto(token);
		} break;
		case GOSUB: {
			get_token();
			if(token_type != IDENTIFIER && token_type != NUMBER)
				sb_error("gosub destination expected");
			exec_gosub(token);
		} break;
		case RETURN:
			if(gtos == 0)
				sb_error("RETURN without GOSUB");
			prog = gstack[--gtos].loc;
			curr_line = gstack[gtos].line;
			break;
		case ON: {
			char dest[80] = {'\0'};
			value_t condition;
			int operation, check, i = 1;
			get_exp(&condition);
			check = as_int(&condition);
			operation = get_token();
			if(operation != GOTO && operation != GOSUB)
				sb_error("`goto` or `gosub` expected");
			do {
				get_token();
				if(token_type != IDENTIFIER && token_type != NUMBER)
					sb_error("destination expected");
				if(i++ == check)
					strcpy(dest, token);
			} while(get_token() == ',');
			if(token_type != EOL && token_type != FINISHED)
				sb_error("expected end of line");
			if(dest[0]) {
				if(operation == GOTO)
					exec_goto(dest);
				else
					exec_gosub(dest);
			}
		} break;
		case FUNCTION: {
			int parens = 0, argc = 0;
			value_t result, argv[MAX_ARGS];
			sb_function_t fun = tocall;
			get_token();
			if(token_type == '(') {
				parens = 1;
				if(get_token() == ')') {  /* empty parens? */
					get_token();
					goto do_call;
				}
			} else if(token_type == EOL || token_type == FINISHED)
				goto do_call; /* no args */

			putback();
			do {
				get_exp(&argv[argc++]);
			} while (get_token() == ',');

			if(parens) {
				if(token_type != ')')
					sb_error("')' expected");
				get_token();
			}
do_call:
			if(token_type != EOL && token_type != FINISHED)
				sb_error("expected end of line");
			result.type = V_STR;
			result.v.s = "";
			fun(&result, argc, argv);
		} break;
		case REM:
			find_eol();
			/* fallthrough */
		case FINISHED: /* fallthrough */
		case EOL:
			break;
		case END: return 1;
		default: sb_error("unexpected token");
		}
	} while (token_type != FINISHED);
	return 1;
}

int execute(char *program) {
	int result = 0;

	prog = program;

	curr_line = 1;
	scan_labels();

	ftos = 0;
	gtos = 0;
	curr_line = 1;

	assert(!has_jmp); /* don't call recursively */

	if(!setjmp(e_buf)) {
		has_jmp = 1;
		result = execute_lines();
	}
	has_jmp = 0;
	curr_line = 0;
	return result;
}

int sb_gosub(const char *sub) {
	struct label *label;
	char *save_prog;
	int result, save_gtos = gtos, save_line = curr_line, save_jmp = has_jmp;

	if(!has_jmp && setjmp(e_buf))
		return 0;

	has_jmp = 1;

	putback();
	save_prog = prog;

	label = find_label(sub);
	if(!label)
		sb_error("undefined label");

	if(gtos == SUB_NEST)
		sb_error("too many nested GOSUBs");
	gstack[gtos].loc = "";
	gstack[gtos++].line = curr_line;

	prog = label->p;
	curr_line = label->line;

	result = execute_lines();

	prog = save_prog;
	get_token();

	curr_line = save_line;
	has_jmp = save_jmp;
	gtos = save_gtos;

	return result;
}

int sb_line() {
	return curr_line;
}

void sb_clear() {
	nvars = 0;
}

/**
 * Built-in Functions
 * ==================
 */

/**
 * `randomize [seed]`
 * :    Initializes the random number generator
 * :    The `seed` is optional.
 */
static void srnd_function(value_t *result, int argc, value_t argv[]) {
	(void)result;
	srand(argc > 0 ? as_int(&argv[0]) : time(NULL));
}

/**
 * `rnd(n)` or `rnd(m,n)`
 * :    Returns a random number.
 * :    If only one argument is given, the returned number is
 * :    in the range $[1,n]$. If two arguments are given, the returned
 * :    number is in the range $[m,n]$
 */
static void rnd_function(value_t *result, int argc, value_t argv[]) {
	int s = 1, e = 100, r = rand();
	if(argc > 1) {
		s = as_int(&argv[0]);
		e = as_int(&argv[1]);
	} else if(argc > 0)
		e = as_int(&argv[0]);
	*result = make_int(r % (e - s + 1) + s);
}

/**
 * `len(s$)`
 * :    returns the length of the string argument `s$`
 */
static void len_function(value_t *result, int argc, value_t argv[]) {
	if(argc > 0)
		*result = make_int(strlen(as_string(&argv[0])));
}

static void str_cut(value_t *result, const char *s, int start, int len) {
	int ilen = strlen(s);
	start--;
	if(start < 0 || start > ilen) {
		*result = make_str("");
		return;
	}
	if(len < 0) len = 0;
	if(start + len > ilen)
		len = ilen - start;
	result->type = V_STR;
	result->v.s = sb_talloc(len + 1);
	strncpy(result->v.s, s + start, len);
	result->v.s[len] = '\0';
}

/**
 * `mid(s$, start, len)`
 * :    Returns a substring of `s$` starting at `start` of length `len`
 */
static void mid_function(value_t *result, int argc, value_t argv[]) {
	if(argc < 3) return;
	str_cut(result, as_string(&argv[0]), as_int(&argv[1]), as_int(&argv[2]));
}

/**
 * `left(s$, n)`
 * :    Returns the `n` leftmost characters in `s$`
 */
static void left_function(value_t *result, int argc, value_t argv[]) {
	if(argc < 2) return;
	str_cut(result, as_string(&argv[0]), 1, as_int(&argv[1]));
}

/**
 * `right(s$, n)`
 * :    Returns the `n` rightmost characters in `s$`
 */
static void right_function(value_t *result, int argc, value_t argv[]) {
	const char *s;
	if(argc < 2) return;
	s = as_string(&argv[0]);
	str_cut(result, s, strlen(s) - as_int(&argv[1]) + 1, TOKEN_SIZE);
}

/**
 * `ucase(s$)`
 * :    Returns `s$` converted to uppercase
 */
static void upper_function(value_t *result, int argc, value_t argv[]) {
	char *p;
	if(argc < 1) return;
	result->type = V_STR;
	result->v.s = sb_strdup(as_string(&argv[0]));
	for(p = result->v.s; *p; p++)
		*p = toupper(*p);
}

/**
 * `lcase(s$)`
 * :    Returns `s$` converted to lowercase
 */
static void lower_function(value_t *result, int argc, value_t argv[]) {
	char *p;
	if(argc < 1) return;
	result->type = V_STR;
	result->v.s = sb_strdup(as_string(&argv[0]));
	for(p = result->v.s; *p; p++)
		*p = tolower(*p);
}

/**
 * `instr(haystack$, needle$)`
 * :    Searches for the string `needle$` in the string `haystack$` and
 * :    returns its index. Returns 0 if not found.
 */
static void instr_function(value_t *result, int argc, value_t argv[]) {
	const char *find, *hays;
	if(argc < 2) return;
	hays = as_string(&argv[0]);
	find = strstr(hays, as_string(&argv[1]));
	if(find)
		*result = make_int((find - hays) + 1);
}

/* Simplified from Rich Salz' matcher.
 * See:https://en.wikipedia.org/wiki/Wildmat
 */
static int wildmat(const char *str, const char *pattern) {
	for(;*pattern; str++, pattern++) {
		if(*pattern == '*') {
			while(*++pattern == '*');
			if(!*pattern)
				return 1;
			while(*str)
				if(wildmat(str++, pattern))
					return 1;
			return 0;
		} else if(*pattern != '?' && *str != *pattern)
			return 0;
	}
	return !*str;
}
/**
 * `wildmat(str$, pat$)`
 * :    Returns 1 if the `str$` matches the wildcard pattern `pat$`.
 * :
 * :    A '?' matches a single character, '*' matches any sequence of
 * :    characters.
 */
static void wildmat_function(value_t *result, int argc, value_t argv[]) {
	if(argc < 2) return;
	*result = make_int(wildmat(as_string(&argv[0]), as_string(&argv[1])));
}

/**
 * `iif(cond, trueval, falseval)`
 * :    If `cond` evaluates to true, returns `trueval`
 * :    else returns `falseval`
 */
static void iif_function(value_t *result, int argc, value_t argv[]) {
	if(argc < 2) return;
	if(as_int(&argv[0]))
		*result = argv[1];
	else if(argc > 2)
		*result = argv[2];
}

/**
 * `mux(x, val1, val2, ...)`
 * :    Maps `x` to the values `val1`, `val2`, etc.
 * :
 * :    If `x` is 1, returns `val1`; if `x` is 2, returns `val2` and so on
 */
static void mux_function(value_t *result, int argc, value_t argv[]) {
	int x;
	if(argc < 1) return;
	x = as_int(&argv[0]);
	if(x < argc)
		*result = argv[x];
}

/**
 * `demux(x, val1, val2, ...)`
 * :    If `x` is `val1` it returns 1; if `x` is `val2` it returns 2 and so on
 */
static void demux_function(value_t *result, int argc, value_t argv[]) {
	int i;
	if(argc < 1) return;
	for(i = 1; i < argc; i++) {
		if(argv[0].type == V_INT) {
			if(as_int(&argv[0]) != as_int(&argv[i])) continue;
		} else {
			if(strcmp(as_string(&argv[0]), as_string(&argv[i]))) continue;
		}
		*result = make_int(i);
		return;
	}
}

/**
 * `int(val)`
 * :    Converts `val` to an integer
 */
static void int_function(value_t *result, int argc, value_t argv[]) {
	*result = make_int(argc > 0 ? as_int(&argv[0]) : 0);
}

/**
 * `str(val)`
 * :    Converts `val` to a string
 */
static void str_function(value_t *result, int argc, value_t argv[]) {
	*result = make_str(argc > 0 ? as_string(&argv[0]) : "");
}

/**
 * `error(message)`
 * :    Halts the interpreter with the given error message
 */
static void error_function(struct value *result, int argc, struct value argv[]) {
	(void)result;
	sb_error(argc > 0 ? as_string(&argv[0]) : "??");
}

void add_std_library() {
	add_function("randomize", srnd_function);
	add_function("rnd", rnd_function);
	add_function("len", len_function);
	add_function("mid", mid_function);
	add_function("left", left_function);
	add_function("right", right_function);
	add_function("ucase", upper_function);
	add_function("lcase", lower_function);
	add_function("instr", instr_function);
	add_function("wildmat", wildmat_function);
	add_function("iif", iif_function);
	add_function("mux", mux_function);
	add_function("demux", demux_function);
	add_function("int", int_function);
	add_function("str", str_function);
	add_function("error", error_function);
}

#ifdef SB_MAIN
/* Example of the bare minimum needed to run the interpreter.
 *
 * Compile like so:
 * $ gcc -DSB_MAIN sbasic.c
 */
int main(int argc, char *argv[]) {
	char *p_buf;

	if(argc != 2) {
		fprintf(stderr, "usage: %s <filename>\n", argv[0]);
		return 1;
	}

	if(!(p_buf = load_program(argv[1]))) {
		fprintf(stderr, "couldn't load %s", argv[1]);
		return 1;
	}

	add_std_library();

	if(!execute(p_buf))
		return 1;

	free(p_buf);

	return 0;
}
#endif
