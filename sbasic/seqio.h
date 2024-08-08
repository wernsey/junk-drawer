/**
 * # Sequential IO
 *
 * Functions for working with Visual Basic-style Sequential Text Files.
 *
 * To use this library, define `SEQIO_IMPL` before including **seqio.h** in _one_
 * of your C files (other C files include **seqio.h** normally), like so:
 *
 * ```
 * #define SEQIO_IMPL
 * #include "seqio.h"
 * ```
 *
 * The only reason for this code's existence is because I was bored and it
 * scratched a certain nostalgia itch: In my youth I used these types of files
 * many times for custom file formats that could be created by a tool written
 * in one language (Visual Basic 6) and then consumed by a program written in
 * another (in my case, a game written in Visual C++), while having the benefit
 * of being human readable and easy to parse with my limited experience at the
 * time.
 *
 * This was before I knew what XML was (maybe not such a bad thing) and before
 * JSON became a thing.
 *
 * You might want to look at [this VB6 guide][vb-files] for more information
 * about how these files were used in practice.
 *
 * There is a test program at the bottom of this file. You can compile it like so:
 *
 * ```sh
 * gcc -DSEQIO_TEST -Wall -std=c89 -xc -g -O0 seqio.h
 * ```
 *
 * I wanted some form of Visual Basic compatibility. Unfortunately there does
 * not seem to be an _official_ specification; The best I could find seems to be
 * the documentation on VBA's [WRITE][vba-write] and [INPUT][vba-input] statements.
 * 
 * This implementation takes some inspiration from [RFC 4180][rfc4180]
 * for escaping quotes and dealing with newlines in values (though that
 * RFC is meant for something else).
 *
 * [vb-files]: https://www.informit.com/articles/article.aspx?p=20992&seqNum=4#
 * [vba-write]: https://learn.microsoft.com/en-us/office/vba/language/reference/user-interface-help/writestatement
 * [vba-input]: https://learn.microsoft.com/en-us/office/vba/language/reference/user-interface-help/inputstatement
 * [rfc4180]: https://www.ietf.org/rfc/rfc4180.txt
 */
#ifndef SEQIO_H

#ifdef __cplusplus
extern "C" {
#endif

/* Length of the internal buffer where tokens are kept as they're read */
#ifndef SEQIO_MAXLEN
#  define SEQIO_MAXLEN  256
#endif

/* I can't find any reference of the BASICs using comments in sequential 
 * text files, but I thought it'd be a useful feature nonetheless.
 */
#if !defined(SEQIO_NOCOMMENT) && !defined(SEQIO_COMMENT_CHAR)
#  define SEQIO_COMMENT_CHAR    ';'
#endif

/* Should quotes in string values be escaped?
 * The BASICs don't do it, but IMHO it is quite a 
 * shortcoming, so I allow for it.
 */
#ifndef SEQIO_ESCAPE_QUOTES
#  define SEQIO_ESCAPE_QUOTES 1
#endif

#ifndef SEQIO_NO_FLOAT
/* I actually tried this on Turbo C, which
does not enable floating point by default. */
#  define SEQIO_NO_FLOAT 0
#endif

/* Which `printf()` format string to use for floating point numbers? */
#ifndef SEQIO_FLOAT_FORMAT
#  define SEQIO_FLOAT_FORMAT "%g"
#endif

/* VB6 would store dates like so: `#1998-01-01#`
 * There is also `#NULL#`, `#TRUE#` and `#FALSE#`.
 * Defining `SEQIO_HASH_QUOTES` treats the 
 * `#` symbols the same way as quotes
 */
#ifndef SEQIO_HASH_QUOTES
#  define SEQIO_HASH_QUOTES 1
#endif

/* I tried to keep the code C89, but snprintf() is just too useful */
#ifndef SEQIO_HAS_SNPRINTF
#  define SEQIO_HAS_SNPRINTF 0
#endif

/**
 * ## Definitions
 *
 * ### Function types
 *
 * * `typedef int (*seq_getchar_fun)(void *)`
 * * `typedef int (*seq_putchar_fun)(int, void *)`
 * * `typedef int (*seq_close_fun)(void *)`
 * * `typedef int (*seq_eof_fun)(void *)`
 */
typedef int (*seq_getchar_fun)(void *);
typedef int (*seq_putchar_fun)(int, void *);
typedef int (*seq_close_fun)(void *);
typedef int (*seq_eof_fun)(void *);

/**
 * ### `typedef struct SeqIO`
 *
 * It has these members
 *
 * * `seq_getchar_fun getc`
 * * `seq_putchar_fun putc`
 * * `seq_close_fun close`
 * * `seq_eof_fun eof`
 * * `int error, ateof`
 * * `unsigned int pos`
 * * `char buffer[SEQIO_MAXLEN]`
 */
typedef struct {
    void *data;
    seq_getchar_fun getc;
    seq_putchar_fun putc;
    seq_close_fun close;
    seq_eof_fun eof;
    int error, ateof;
    unsigned int pos;
    char buffer[SEQIO_MAXLEN];
	int c, unget;
} SeqIO;

/**
 * ## Open/Close Functions
 *
 * These functions opens and close streams.
 */

/**
 * ### `int seq_istream(SeqIO *a, void *data, seq_getchar_fun get_fn)`
 */
int seq_istream(SeqIO *S, void *data, seq_getchar_fun get_fn);

/**
 * ### `int seq_ostream(SeqIO *S, void *data, seq_putchar_fun put_fn)`
 */
int seq_ostream(SeqIO *S, void *data, seq_putchar_fun put_fn);

#ifdef EOF
/**
 * ### `int seq_infilep(SeqIO *S, FILE *f)`
 */
int seq_infilep(SeqIO *S, FILE *f);

/**
 * ### `int seq_outfilep(SeqIO *S, FILE *f)`
 */
int seq_outfilep(SeqIO *S, FILE *f);
#endif

/**
 * ### `int seq_infile(SeqIO *S, const char *name)`
 */
int seq_infile(SeqIO *S, const char *name);

/**
 * ### `int seq_outfile(SeqIO *S, const char *name)`
 */
int seq_outfile(SeqIO *S, const char *name);

/**
 * ### `void seq_close(SeqIO *S)`
 *
 * Closes the stream `S` by calling the `S->close` function
 * (if any) and then setting `S->data` to `NULL`
 */
void seq_close(SeqIO *S);

/**
 * ### `int seq_eof(SeqIO *S)`
 *
 * Checks whether the end of an (input) stream has been reached by
 * first calling the `S->eof` function (if any) and then checking
 * the `S->ateof` flag.
 */
int seq_eof(SeqIO *S);

/**
 * ### `const char *seq_error(SeqIO *S)`
 *
 * Checks the `S->error` flag after reading or writing
 * operations to see if an error occured.
 *
 * If an error occured it returns the error message (which
 * is stored internally in `S->buffer`).
 */
const char *seq_error(SeqIO *S);

/**
 * ## Input Functions
 */

/**
 * ### `const char *seq_read(SeqIO *S)`
 */
const char *seq_read(SeqIO *S);

/**
 * ### `int seq_read_int(SeqIO *S)`
 */
int seq_read_int(SeqIO *S);

#if !SEQIO_NO_FLOAT
/**
 * ### `double seq_read_float(SeqIO *S)`
 */
double seq_read_float(SeqIO *S);
#endif

/**
 * ### `int seq_read_rec(SeqIO *S, const char *fmt, ...)`
 *
 * Reads a record from the file.
 *
 * A record is just a sequence of values in this context.
 * The `fmt` parameter determines how the values are organised:
 *
 * - `%s` for string values. The corresponding arg should be a pointer to a `char[]` that can
 *        hold at least `SEQIO_MAXLEN` bytes.
 *    - Alternatively `%*s` can be used to specify the size of the buffer in the args.
 * - `%d` or `%d` for decimal integer values. The corresponding arg should be an `int*`
 * - `%f` or `%g` for floating point values. The corresponding arg should be a `double*`
 * - `%b` for boolean values. The corresponding arg should be an `int*`
 */
int seq_read_rec(SeqIO *S, const char *fmt, ...);

/**
 * ## Ouput Functions
 */

/**
 * ### `void seq_write(SeqIO *S, const char *str)`
 */
void seq_write(SeqIO *S, const char *str);

/**
 * ### `void seq_write_int(SeqIO *S, int value)`
 */
void seq_write_int(SeqIO *S, int value);

#if !SEQIO_NO_FLOAT
/**
 * ### `void seq_write_float(SeqIO *S, double value)`
 */
void seq_write_float(SeqIO *S, double value);
#endif

/**
 * ### `void seq_write_special(SeqIO *S, const char *value)`
 */
void seq_write_special(SeqIO *S, const char *value);

/**
 * ### `void seq_write_bool(SeqIO *S, int value)`
 */
void seq_write_bool(SeqIO *S, int value);

/**
 * ### `void seq_endl(SeqIO *S)`
 */
void seq_endl(SeqIO *S);

/**
 * ### `int seq_write_rec(SeqIO *S, const char *fmt, ...)`
 */
int seq_write_rec(SeqIO *S, const char *fmt, ...);

/**
 * ### `void seq_comment(SeqIO *S, const char *str)`
 */
void seq_comment(SeqIO *S, const char *str);

#if defined(SEQIO_IMPL) || defined(SEQIO_TEST)

#ifdef _MSC_VER
#  define _CRT_SECURE_NO_WARNINGS
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <assert.h>
#include <stdarg.h>

static int _s_set_error(SeqIO *S, const char *msg) {
    size_t len = strlen(msg);
    if(len >= SEQIO_MAXLEN)
        len = SEQIO_MAXLEN - 1;
    memcpy(S->buffer, msg, len);
    S->buffer[len] = '\0';
    S->pos = len;
    S->error = 1;
    return 0;
}

int seq_istream(SeqIO *S, void *data, seq_getchar_fun get_fn) {
    S->data = data;
    S->getc = get_fn;
    S->putc = NULL;
    S->close = NULL;
    S->eof = NULL;
    S->error = 0;
    S->ateof = 0;
    S->pos = 0;
	S->c = '\0';
	S->unget = 0;
    return 1;
}

#ifdef feof
/* feof() is a macro */
static int _s_file_eof(void *fp) {
    FILE *f = fp;
    return feof(f);
}
#define FEOF _s_file_eof
#else
/* feof() is a function */
#define FEOF feof
#endif

int seq_infilep(SeqIO *S, FILE *f) {
    if(!seq_istream(S, f, (seq_getchar_fun)fgetc))
        return 0;
    S->eof = (seq_eof_fun)FEOF;
    return 1;
}

int seq_infile(SeqIO *S, const char *name) {
    int result;
	FILE *f = fopen(name, "r");
    if(!f)
        return _s_set_error(S, strerror(errno));
    result = seq_infilep(S, f);
	S->close = (seq_close_fun)fclose;
    return result;
}

int seq_ostream(SeqIO *S, void *data, seq_putchar_fun put_fn) {
    S->data = data;
    S->getc = NULL;
    S->putc = put_fn;
    S->close = NULL;
    S->eof = NULL;
    S->error = 0;
    S->ateof = 0;
    S->pos = 0;
	S->c = '\0';
	S->unget = 0;
    return 1;
}

int seq_outfilep(SeqIO *S, FILE *f) {
    if(!seq_ostream(S, f, (seq_putchar_fun)fputc))
        return 0;
    S->eof = (seq_eof_fun)FEOF;
    return 1;
}

int seq_outfile(SeqIO *S, const char *name) {
	int result;
    FILE *f = fopen(name, "w");
    if(!f)
        return _s_set_error(S, strerror(errno));
    result = seq_outfilep(S, f);
	S->close = (seq_close_fun)fclose;
    return result;
}

void seq_close(SeqIO *S) {
    if(S->close)
        (S->close)(S->data);
    S->data = NULL;
    S->ateof = 1;
}

int seq_eof(SeqIO *S) {
    if(S->eof)
        return (S->eof)(S->data);
    return S->ateof;
}

static int _s_getc(SeqIO *S) {
    return (S->getc)(S->data);
}

static int _s_putc(SeqIO *S, int c) {
    return (S->putc)(c, S->data);
}

static int _s_read(SeqIO *S) {
    int q = 0;
    enum {pre, word, quote, post, error, comment} state = pre;
	
	if(S->error) return 0;
    S->buffer[0] = '\0';
    S->pos = 0;

    if(!S->getc)
        return _s_set_error(S, "not an input stream");

    if(S->ateof)
        return _s_set_error(S, "end of stream");
	
    for(;;) {
        if(S->unget) {
            S->unget = 0;
        } else {
            S->c = _s_getc(S);
        }
        if(S->c == EOF || S->c == '\0') {
            if(state == quote)
                _s_set_error(S, "unterminated string");
            S->ateof = 1;
            break;
        }

        if(S->pos >= SEQIO_MAXLEN) {
            _s_set_error(S, "token too long");
            state = error;
        }

        switch(state) {
            case error:
                if(S->c == '\n' || S->c == ',')
                    goto end;
            break;
            case comment:
                if(S->c == '\n')
                    state = pre;
            break;
            case pre:
                if(S->c == '\n' || S->c == ',')
                    goto end;
#ifdef SEQIO_COMMENT_CHAR
                else if(S->c == SEQIO_COMMENT_CHAR)
                    state = comment;
#endif
                else if(isspace(S->c))
                    continue;
                else if(S->c == '"') {
					q = '"';
                    state = quote;
#if SEQIO_HASH_QUOTES
				} else if(S->c == '#') {
					q = '#';
                    state = quote;
#endif					
                } else {
                    state = word;
                    S->unget = 1;
                }
            break;
            case word:
                if(S->c == '\n' || S->c == ',')
                    goto end;
#if SEQIO_COMMENT_CHAR
				else if(S->c == SEQIO_COMMENT_CHAR) {
					S->unget = 1;
					goto end;
				}
#endif				
                else if(isspace(S->c)) {
                    state = post;
                } else {
                    S->buffer[S->pos++] = S->c;
                }
            break;
            case quote:
                if(S->c == q) {
#if SEQIO_ESCAPE_QUOTES
                    S->c = _s_getc(S);
                    if(S->c == q) {
                        S->buffer[S->pos++] = q;
                    } else {
                        S->unget = 1;
                        state = post;
                    }
#else
                    state = post;
#endif
                } else {
                    S->buffer[S->pos++] = S->c;
                }
            break;
            case post:                
				if(S->c == '\n' || S->c == ',')
                    goto end;
#ifdef SEQIO_COMMENT_CHAR
				else if(S->c == SEQIO_COMMENT_CHAR) {
					S->unget = 1;
					goto end;
				}
#endif				
                else if(isspace(S->c))
                    continue;
                else {
                    _s_set_error(S, "expected ',' or EOL");
                    state = error;
                }
            break;
        }
    }

end:
    S->buffer[S->pos] = '\0';
    return S->pos;
}

const char *seq_read(SeqIO *S) {
    _s_read(S);
    if(S->error)
        return "";
    return S->buffer;
}

int seq_read_int(SeqIO *S) {
    _s_read(S);
    if(S->error)
        return 0;
    return strtol(S->buffer, NULL, 0);
}

#if !SEQIO_NO_FLOAT
double seq_read_float(SeqIO *S) {
    _s_read(S);
    if(S->error)
        return 0.0;
    return atof(S->buffer);
}
#endif

int seq_read_bool(SeqIO *S) {
	_s_read(S);
    if(S->error)
        return 0;
    return !strcmp(S->buffer,"TRUE");
}

int seq_read_rec(SeqIO *S, const char *fmt, ...) {
    va_list arg;
    int s_len = SEQIO_MAXLEN - 1, n = 0;

	if(S->error) return 0;
    if(!S->getc)
        return _s_set_error(S, "not an input stream");

    va_start(arg, fmt);
    while(*fmt) {
        switch(*fmt++) {
            case ' ':
            case '\n':
            case '\t': continue;
            case '%': {
                if(isdigit(*fmt)) {
                    char *e;
                    s_len = strtol(fmt, &e, 10);
                    fmt = e;
                } else if(*fmt == '*') {
                    s_len = va_arg(arg, int);
                    fmt++;
                }
                if(!strchr("sdigfb", *fmt)) {
                    _s_set_error(S, "bad % format");
                }
            } break;
            case 's': {
                /* Note about safety: It assumes the buffer is big enough to hold the
                token. To ensure that it is, it must be at least SEQIO_MAXLEN bytes,
                or you can use the %*s or %[len]s form */
                const char *s = seq_read(S);
                char *dest = va_arg(arg, char *);
				if(S->error) return 0;				
				strncpy(dest, s, s_len - 1);
				dest[s_len-1] = '\0';
				s_len = SEQIO_MAXLEN - 1; /* reset s_len */         
				n++;                
            } break;
            case 'd':
            case 'i': { /* '%d' and '%i' are equivalent */
                int i = seq_read_int(S);
                if(S->error) return 0;				
                *va_arg(arg, int *) = i;
                n++;
            } break;
#if !SEQIO_NO_FLOAT
            case 'g':
            case 'f': {
                double f = seq_read_float(S);
                if(S->error) return 0;
				*va_arg(arg, double *) = f;
				n++;
            } break;
#endif
			case 'b': {
                int i = seq_read_bool(S);
                if(S->error) return 0;				
                *va_arg(arg, int *) = i;
                n++;
            } break;
            default:
                _s_set_error(S, "bad format");
                return 0;
        }
    }
    va_end(arg);
    return n;
}

static void _s_write(SeqIO *S, const char *str) {
    if(S->error) return;
    if(!S->putc) {
        _s_set_error(S, "not an output stream");
        return;
    }
    if(S->pos++)
        _s_putc(S, ',');
    while(*str)
        _s_putc(S, *str++);
    S->pos++;
}

void seq_write(SeqIO *S, const char *str) {
    if(S->error) return;
    if(!S->putc) {
        _s_set_error(S, "not an output stream");
        return;
    }
    if(S->pos++)
        _s_putc(S, ',');
    _s_putc(S, '"');
    while(*str) {
        int c = *str++;
#if SEQIO_ESCAPE_QUOTES
        if(c == '"')
            _s_putc(S, '"');
#endif
        _s_putc(S, c);
    }
    _s_putc(S, '"');
}

void seq_write_int(SeqIO *S, int value) {
    if(S->error) return;
    sprintf(S->buffer, "%d", value);
    _s_write(S, S->buffer);
}

#if !SEQIO_NO_FLOAT
void seq_write_float(SeqIO *S, double value) {
    if(S->error) return;
    sprintf(S->buffer, SEQIO_FLOAT_FORMAT, value);
    _s_write(S, S->buffer);
}
#endif

void seq_write_special(SeqIO *S, const char *value) {
#if SEQIO_HASH_QUOTES
#  define RAW_FMT	"#%s#"
#else
#  define RAW_FMT	"\"%s\""
#endif
    if(S->error) return;
#if !SEQIO_HAS_SNPRINTF
	if(strlen(value) >= sizeof S->buffer - 3) {
		_s_set_error(S, "value is too long");
        return;
	}
	sprintf(S->buffer, RAW_FMT, value);
#else
    snprintf(S->buffer, sizeof S->buffer, RAW_FMT, value);
#endif
    _s_write(S, S->buffer);
#undef RAW_FMT
}

void seq_write_bool(SeqIO *S, int value) {
    if(S->error) return;
#if SEQIO_HASH_QUOTES
    sprintf(S->buffer, "#%s#", value ? "TRUE" : "FALSE");
#else
	sprintf(S->buffer, "%s", value ? "TRUE" : "FALSE");
#endif
    _s_write(S, S->buffer);
}

int seq_write_rec(SeqIO *S, const char *fmt, ...) {
    va_list arg;
    int n = 0;

	if(S->error) return 0;
	
    if(!S->putc)
        return _s_set_error(S, "not an output stream");

    va_start(arg, fmt);
    while(*fmt) {
		if(S->error) return 0;
        switch(*fmt++) {
			case '\n': seq_endl(S); continue;
			case ' ':
			case '\t': continue;
			case '%': 
				/* % is optional; length specifiers are discarded, unlike seq_read_rec() */
				if(*fmt == '*') {
					fmt++;
					va_arg(arg, int);
				} else
					while(isdigit(*fmt)) fmt++;
				continue; 
            case 's':
                seq_write(S, va_arg(arg, const char *));
                n++;
                continue;
            case 'd':
            case 'i': /* '%d' and '%i' are equivalent */
                seq_write_int(S, va_arg(arg, int));
                n++;
                continue;
#if !SEQIO_NO_FLOAT
            case 'g':
            case 'f':
                seq_write_float(S, va_arg(arg, double));
                n++;
                continue;
#endif
			case 'b':
                seq_write_bool(S, va_arg(arg, int));
                n++;
                continue;
            default:
                return _s_set_error(S, "bad format");
        }
    }
    va_end(arg);
    _s_putc(S, '\n');
    S->pos = 0;
    return n;
}

void seq_comment(SeqIO *S, const char * str) {
    if(!S->putc) {
        _s_set_error(S, "not an output stream");
        return;
    }
	if(S->pos) {
		_s_putc(S, '\n');
		S->pos = 0;
	}
    _s_putc(S, SEQIO_COMMENT_CHAR);
    _s_putc(S, ' ');
    while(*str) {
        _s_putc(S, *str++);
    }
    _s_putc(S, '\n');
    S->pos = 0;
}

void seq_endl(SeqIO *S) {
    if(!S->putc) {
        _s_set_error(S, "not an output stream");
        return;
    }
    _s_putc(S, '\n');
    S->pos = 0;
}

const char *seq_error(SeqIO *S) {
    if(!S->error)
        return NULL;
    return S->buffer;
}

/* ## TEST PROGRAM ############################################################## */

#  ifdef SEQIO_TEST
int main(int argc, char *argv[]) {
    const char *filename = NULL;
    char *strings[] = {"foo", "bar", "baz", "fred", "quux"};
    int i, count =  sizeof strings / sizeof strings[0];

    SeqIO stream;

    if(argc > 1)
        filename = argv[1];

    if(filename) {
        if(!seq_infile(&stream, filename)) {
            fprintf(stderr, "error opening input file: %s\n", seq_error(&stream));
            return 1;
        }

        while(!seq_eof(&stream)) {
            const char *s = seq_read(&stream);
            if(seq_error(&stream)) {
                fprintf(stderr, "error: %s\n", seq_error(&stream));
                break;
            }
            printf("token: '%s'\n", s);
        }

        seq_close(&stream);
		return 0;
    }

    if(!seq_outfile(&stream, "seqio.out")) {
        fprintf(stderr, "error opening output file: %s\n", seq_error(&stream));
        return 1;
    }

    seq_comment(&stream, "Number of records:");
	seq_write_int(&stream, count);
    seq_comment(&stream, "Examples using seq_write_rec:");
    for(i = 0; i < count; i++) {
        seq_write_rec(&stream, "%i %s %f %s %b", i, strings[i], (float)(i*i)/3.0, strings[(i+1) % count], i & 0x01);
        if(seq_error(&stream)) {
            fprintf(stderr, "error: %s\n", seq_error(&stream));
            break;
        }
    }

    seq_comment(&stream, "Examples using the other functions:");
    for(i = 0; i < count; i++) {
        seq_write_int(&stream, i+count);
        seq_write(&stream, strings[(i+2) % count]);
        seq_write_float(&stream, (i*i + 7)/3.0);
        seq_write(&stream, strings[(i+4) % count]);
        seq_endl(&stream);
        if(seq_error(&stream)) {
            fprintf(stderr, "error: %s\n", seq_error(&stream));
            break;
        }
    }

    /* If you don't use SEQIO_ESCAPE_QUOTES you're in for a world
    of hurt if you try to read this file */
    seq_write(&stream, "A string with embedded \"quote\" values" );
	
	seq_endl(&stream);
	seq_write_bool(&stream, 1);
	seq_write_bool(&stream, 0);
	seq_write_special(&stream, "NULL");

    seq_close(&stream);

#if 1
    /* Demo of how to use seq_read_rec */
    if(!seq_infile(&stream, "seqio.out")) {
        fprintf(stderr, "error opening input file: %s\n", seq_error(&stream));
        return 1;
    }
	count = seq_read_int(&stream);
	printf("%d records:\n", count);
    for(i = 0; i < count; i++) {
        char a[SEQIO_MAXLEN], b[SEQIO_MAXLEN];
        int x, odd;
        double y;
        /*seq_read_rec(&stream, "i%*sfs", &x, 3, a, &y, b);*/
        /*seq_read_rec(&stream, "i%3sfs", &x, a, &y, b);*/
        seq_read_rec(&stream, "%i %s %f %s %b", &x, a, &y, b, &odd);
        if(seq_error(&stream)) {
            fprintf(stderr, "error: %s\n", seq_error(&stream));
            break;
        }
        printf("** x=%d; y=%g a='%s'; b='%s'; odd=%d\n", x, y, a, b, odd);
        fflush(stdout);
    }
    seq_close(&stream);
#endif

    return 0;
}
#  endif /* SEQIO_TEST */

#endif /* SEQIO_IMPL */

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* SEQIO_H */
