/**
 *
 * Implementation of a Pratt parser for mathematical expressions, for
 * my own enlightenment.
 *
 * * [Pratt Parsers: Expression Parsing Made Easy](https://journal.stuffwithstuff.com/2011/03/19/pratt-parsers-expression-parsing-made-easy/) by Munificent
 * * He also talks about it in [chapter 17 of Crafting Interpreters](https://craftinginterpreters.com/compiling-expressions.html#a-pratt-parser)
 * * <https://dev.to/jrop/pratt-parsing> - has a lot of pseudo code and uses
 *   the same terminology that Pratt used in his original paper.
 * * <https://langdev.stackexchange.com/a/3275> - very thorough explanation.
 *   Would upvote if I was signed in
 * * [Demystifying Pratt Parsers](https://martin.janiczek.cz/2023/07/03/demystifying-pratt-parsers.html)
 *   Example code in Elm, but nice diagrams
 * * [Simple but Powerful Pratt Parsing](https://matklad.github.io/2020/04/13/simple-but-powerful-pratt-parsing.html),
 *   in Rust, but I wont hold that against it.
 * * [On Recursive Descent and Pratt Parsing](https://chidiwilliams.com/posts/on-recursive-descent-and-pratt-parsing)
 *   also has a concise implementation
 * * [Top-Down operator precedence (Pratt) parsing](https://eli.thegreenplace.net/2010/01/02/top-down-operator-precedence-parsing)
 *   on Eli Bendersky's blog
 * * The Fredrik Lundh article he cites is now here:
 *   [Simple Top-Down Parsing in Python](https://11l-lang.org/archive/simple-top-down-parsing/)
 *
 * Author: Werner Stoop
 * CC0 This work has been marked as dedicated to the public domain.
 * https://creativecommons.org/publicdomain/zero/1.0/
 */
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <math.h>
#include <assert.h>

typedef double number_t;

typedef struct {
	const char *text;
	int i;
	number_t number;
} context_t;

number_t expression(context_t *,int);

typedef enum {
	END,
	NUMBER,
	PLUS,
	MINUS,
	MUL,
	DIV,
	EXP,
	OPEN_PAREN, CLOSE_PAREN,
} token_t;

typedef number_t (*nud_t)(context_t *, token_t);
typedef number_t (*led_t)(context_t *, number_t, token_t);

token_t tokenize(const char *text, unsigned int *ip, number_t *number);

int prec(token_t);

void error(const char *text, int i, const char *e) {
	fprintf(stderr, "error: %s\n", e);
	fprintf(stderr, "  %s\n", text);
	fprintf(stderr, "  %*c\n", i, '^');
	abort();
}

token_t Token;
token_t next_token(context_t *C) {
	Token = tokenize(C->text, &C->i, &C->number);
	return Token;
}

typedef struct {
	int lbp;
	nud_t nud;
	led_t led;
} operator_t;

number_t nud_number(context_t *C, token_t token) {
	return C->number;
}

number_t nud_negate(context_t *C, token_t token) {
	return -expression(C, 100);
}

number_t nud_parens(context_t *C, token_t token) {
	number_t result = expression(C, 0);
	if(Token != CLOSE_PAREN)
		error(C->text, C->i, "')' expected");
	next_token(C);
	return result;
}

number_t binop(context_t *C, number_t left, token_t token) {
	switch(token) {
		case MINUS: return left - expression(C, prec(token));
		case PLUS: return left + expression(C, prec(token));
		case MUL: return left * expression(C, prec(token));
		case DIV: return left / expression(C, prec(token));
		case EXP: return pow(left, expression(C, prec(token)-1));
		default: error(C->text, C->i, "operator??");
	}
}

static const operator_t operators[] = {
	[END] = {},
	[NUMBER] = { .nud = nud_number},
	[MINUS] = {.lbp = 10, .nud = nud_negate, .led = binop},
	[PLUS] = {.lbp = 10, .led = binop},
	[MUL] = {.lbp = 20, .led = binop},
	[DIV] = {.lbp = 20, .led = binop},
	[EXP] = {.lbp = 30, .led = binop},
	[OPEN_PAREN] = {.nud = nud_parens},
	[CLOSE_PAREN] = {},
};

int prec(token_t t) {
	return operators[t].lbp;
}

number_t expression(context_t *C, int rbp) {
	token_t token = Token;
	next_token(C);
	if(token == END)
		error(C->text, C->i, "unexpected end of expression");
	nud_t nud = operators[token].nud;
	if(!nud)
		error(C->text, C->i, "expression expected");
	number_t left = nud(C, token);
	while(rbp < operators[Token].lbp) {
		token = Token;
		next_token(C);
		assert(operators[token].led);
		left = operators[token].led(C, left, token);
	}
	return left;
}

number_t parse(const char *text) {
	context_t C = {.text = text};
	next_token(&C);
	return expression(&C, 0);
}

token_t tokenize(const char *text, unsigned int *ip, number_t *number) {
	while(isspace(text[*ip]))
		(*ip)++;
	if(!text[*ip]) return END;
	if(isdigit(text[*ip])) {
		char token[64];
		int n = 0;
		while(isdigit(text[*ip]) || text[*ip] == '.') {
			if(n == sizeof token - 1)
				error(text, *ip, "token too long");
			token[n++] = text[(*ip)++];
		}
		token[n] = '\0';
		if(number)
			*number = atof(token);
		return NUMBER;
	}
	switch(text[(*ip)++]) {
		case '+': return PLUS;
		case '-': return MINUS;
		case '*': return MUL;
		case '/': return DIV;
		case '(': return OPEN_PAREN;
		case ')': return CLOSE_PAREN;
		case '^': return EXP;
		default: error(text, *ip, "Unrecogniseed token");
	}

	return END;
}

int main(int argc, char *argv[]) {
	const char *in;
	if(argc > 1)
		in = argv[1];
	else
		in = "5+5";
	printf("in: %s\n", in);

#if 0
	int i = 0;
	token_t t;
	do {
		number_t n;
		t = tokenize(in, &i, &n);
		if(t == NUMBER)
			printf("%d   (%g)\n", t, n);
		else
			printf("%d\n", t);
	} while(t != END);

	printf("------------------\n");
#endif
	number_t result = parse(in);

	printf("result = %g\n", result);

	return 0;
}
