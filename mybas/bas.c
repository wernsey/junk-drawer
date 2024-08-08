/*
https://github.com/norvig/pytudes/blob/master/ipynb/BASIC.ipynb
https://en.wikipedia.org/wiki/Recursive_descent_parser
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>
#include <assert.h>

#include "bas.h"
#include "ast.h"
#include "hash.h"

const char *Filename;
static int Sym = 0, LastSym = 0;
static char Word[64], LastWord[64];
static char *Text = NULL;
static char *Lex = NULL;
int Line = 1,      /* Actual line in the file */
    BasicLine = 0; /* BASIC line number */

Node *Funs[26];

static struct {
    const char *name;
    int value;
} Keywords[] = {
  {"LET", LET},
  {"READ", READ},
  {"DATA", DATA},
  {"PRINT", PRINT},
  {"GOTO", GOTO},
  {"IF", IF},
  {"FOR", FOR},
  {"NEXT", NEXT},
  {"END", END},
  {"DEF", DEF},
  {"GOSUB", GOSUB},
  {"RETURN", RETURN},
  {"DIM", DIM},
  {"REM", REM},
  {"TO", TO},
  {"THEN", THEN},
  {"STEP", STEP},
  {"STOP", STOP},
  {NULL,0}
},
Functions[] = {
  {"SIN",SIN},
  {"COS",COS},
  {"TAN",TAN},
  {"ATN",ATN},
  {"EXP",EXP},
  {"ABS",ABS},
  {"LOG",LOG},
  {"SQR",SQR},
  {"RND",RND},
  {"INT",INT},
  {"FN",FN},
  {NULL,0}
};
const char *nodename(int type);
static void nextsym();
void error(const char *msg, ...) {
    va_list ap;
    va_start(ap, msg);
    fprintf(stderr, "\nerror:%d (%d): ", BasicLine, Line);
    vfprintf(stderr, msg, ap);
    va_end(ap);
    exit(EXIT_FAILURE);
}

static int accept(int s) {
    if(Sym == s) {
        nextsym();
        return 1;
    }
    return 0;
}

static int expect(int s) {
    if(accept(s))
        return 1;
    error("'%s' expected; got '%s'", nodename(s), Word);
    return 0;
}

static void init(char *text) {
    Text = text;
    Lex = text;
    nextsym();
}
#define SYMBOL(s) do{Sym=s;return;}while(0)
static void nextsym() {
    LastSym = Sym;
    memcpy(LastWord, Word, sizeof LastWord);
    Word[0] = 0;
    int l = 0, i;
    while(isspace(Lex[0])) {
        if(Lex[0] == '\n') {
            Lex++;
            SYMBOL(NL);
        }
        Lex++;
    }
    if(!Lex[0])
        SYMBOL(FEND);
    else if(isalpha(Lex[0])) {
        do {
            Word[l++] = toupper(Lex[0]);
            Lex++;
            if(l >= sizeof Word - 1)
                error("identifier too long");
        } while(isalnum(Lex[0]));
        Word[l] = '\0';
        for(i = 0; Keywords[i].name; i++)
            if(!strcmp(Keywords[i].name, Word))
                SYMBOL(Keywords[i].value);
        for(i = 0; Functions[i].name; i++)
            if(!strcmp(Functions[i].name, Word))
                SYMBOL(FUN);
        if(Word[0] == 'F' && Word[1] == 'N' && isupper(Word[2]) && !Word[3])
            SYMBOL(FN);
        SYMBOL(ID);
    } else if(isdigit(Lex[0])) {
        do {
            Word[l++] = Lex[0];
            Lex++;
            if(l >= sizeof Word - 1)
                error("number too long");
        } while(isdigit(Lex[0]) || Lex[0] == '.');
        Word[l] = '\0';
        SYMBOL(NUM);
    } else if(Lex[0] == '"') {
        Lex++;
        do {
            if(!Lex[0] || Lex[0] =='\n')
                error("unterminated string literal");
            Word[l++] = Lex[0];
            Lex++;
            if(l >= sizeof Word - 1)
                error("string literal too long");
        } while(Lex[0] != '"');
        Lex++;
        Word[l] = '\0';
        SYMBOL(STR);
    } else if(!memcmp(Lex, "<=", 2)){
        Lex+=2;
        SYMBOL(LTE);
    } else if(!memcmp(Lex, ">=", 2)){
        Lex+=2;
        SYMBOL(GTE);
    } else if(!memcmp(Lex, "<>", 2)){
        Lex+=2;
        SYMBOL(NEQ);
    }
    Sym = *Lex++;
    Word[0] = Sym; Word[1] = '\0';
    return;
}

/* Parser... */

Node *stmts();
Node *stmt();
Node *exprn();
Node *atom();
Node *product();
Node *power();
Node *unary();

/*
program := lines*
lines :=  NUM stmts NL
      |   NL
      |   FEND
*/
Node *program() {
    Node *prog = node_operator(PROG, 0);
    while(Sym != FEND) {
        if(accept(NL)) {
            Line++;
            continue;
        }
        expect(NUM);
        BasicLine = atoi(LastWord);
        if(BasicLine != atof(LastWord)) error("bad line number %s", LastWord);
        Node *ln = node_number(BasicLine);
        Node *s = stmts();
        if(s){
			Node *line = node_operator(LINE, 2, ln, s);
			node_add_child(prog, line);
		}
        if(Sym == FEND) break;
        expect(NL);
        Line++;
    }
    return prog;
}

/*
stmts := stmt [':' stmt]+
*/
Node *stmts() {	
	Node *n = stmt();
	if(!n) return NULL;
	Node *s = node_operator(STMTS, 1, n);
	while(accept(':')) {
		node_add_child(s, stmt());
    }
    return s;
}

/*
stmt :=  REM <any tokens except NL and FEND>
	  |   DIM ID ['(' NUM (',' NUM ')')* ]
      |   [LET] ID '=' expr
      |   GOTO NUM
      |   GOSUB NUM
      |   IF expr ('='|'<>'|'>'|'>='|'<'|'<=') expr THEN (NUM | stmts)
      |   PRINT (expr (','|';'|e))*
      |   RETURN | END | STOP
      |   FOR ID '=' expr TO expr (STEP expr)
      |   NEXT ID
      |   DATA NUM (',' NUM)*
      |   READ ID (',' ID)*
      |   DEF FN[A-Z] '(' ID ')' '=' expr
*/
Node *stmt() {
    if(accept(REM)) {
        while(Sym != NL && Sym != FEND)
            nextsym();
        return NULL;
    } else if(accept(LET)) {
        expect(ID);
		Node *name = node_id(LastWord);
        Node *var;
		if(accept('(')) {
			var = node_operator(ARR, 1, name);
			do {
				Node *exp = exprn();
				node_add_child(var, exp);
			} while(accept(','));
			expect(')');
		} else 
			var = node_operator(VARIABLE, 1, name);
        expect('=');
        Node *expr = exprn();
        return node_operator(LET, 2, var, expr);
    } else if(accept(ID)) {
        Node *var = node_operator(VARIABLE, 1, node_id(LastWord));
        expect('=');
        Node *expr = exprn();
        return node_operator(LET, 2, var, expr);
    } else if(accept(DIM)) {
		Node *p = node_operator(DIM, 0);
        do {
            expect(ID);
			Node *name = node_id(LastWord);			
			if(accept('(')) {
				Node *arr = node_operator(ARR, 1, name); 
				do {
					expect(NUM);
					node_add_child(arr, node_number(atoi(LastWord)));
				} while(accept(','));
				expect(')');
				node_add_child(p, arr);
			} else {
				node_add_child(p, name);
			}
        } while(accept(','));
        return p;
    } else if(accept(GOTO) || accept(GOSUB)) {
        int op = LastSym;
        expect(NUM);
        int dest = atoi(LastWord);
        if(dest != atof(LastWord)) error("bad destination %s", LastWord);
        return node_operator(op, 1, node_number(dest));
    } else if(accept(IF)) {
        Node *l, *r;
        int rel;
        l = exprn();
        if(accept('=') || accept(NEQ) || accept(GTE) || accept(LTE) || accept('>') || accept('<')) {
            if(LastSym == '=')
                rel = EQ;
            else
                rel = LastSym;
        } else
            error("relational expected");
        r = exprn();
        expect(THEN);
        if(accept(NUM))
            return node_operator(IF, 2, node_operator(rel, 2, l, r), node_operator(GOTO, 1, node_number(atoi(LastWord))));
        else
            return node_operator(IF, 2, node_operator(rel, 2, l, r), stmts());
    } else if(accept(PRINT)) {
        Node *p = node_operator(PRINT, 0);
        while(Sym != NL && Sym != FEND && Sym != ':') {
            if(accept(',') || accept(';'))
				p = node_add_child(p, node_operator(LastSym, 0));
            else if(accept(STR))
                p = node_add_child(p, node_string(LastWord));
            else
                p = node_add_child(p, exprn());
        } 
        return p;
    } else if(accept(RETURN) || accept(END) || accept(STOP)) {
        return node_operator(LastSym, 0);
    } else if(accept(FOR)) {
        expect(ID);
        Node *name = node_operator(VARIABLE, 1, node_id(LastWord));
        expect('=');
        Node *from = exprn();
        expect(TO);
        Node *to = exprn();
        if(accept(STEP)) {
            Node *step = exprn();
            return node_operator(FOR, 4, name, from, to, step);
        }
        return node_operator(FOR, 3, name, from, to);
    } else if(accept(NEXT)) {
        expect(ID);
        return node_operator(NEXT, 1, node_operator(VARIABLE, 1, node_id(LastWord)));
    } else if(accept(DATA)) {
		Node *p = node_operator(DATA, 0);
        do {
            expect(NUM);
            node_add_child(p, node_number(atoi(LastWord)));
        } while(accept(','));
        return p;
    } else if(accept(READ)) {
        Node *p = node_operator(READ, 0);
        do {
            expect(ID);
            p = node_add_child(p, node_operator(VARIABLE, 1, node_id(LastWord)));
        } while(accept(','));
        return p;
    } else if(accept(DEF)) {
        expect(FN);
        assert(isupper(LastWord[2]));
        Node *name = node_id(LastWord);
        expect('(');
        expect(ID);
        Node *param = node_id(LastWord);
        expect(')');
        expect('=');
        Node *body = exprn();
        return node_operator(DEF, 3, name, param, body);
    } else
        error("statement expected");
    return NULL;
}
Node *exprn() {
    Node *n = product();
    while(accept('+') || accept('-')) {
        int op = LastSym;
        n = node_operator(op, 2, n, product());
    }
    return n;
}
Node *product() {
    Node *n = power();
    while(accept('*') || accept('/') || accept('%')) {
        int op = LastSym;
        n = node_operator(op, 2, n, power());
    }
    return n;
}
Node *power() {
    Node *n = unary();
    if(accept('^'))
        n = node_operator('^', 2, n, power());
    return n;
}
Node *unary() {
    if(accept('-') || accept('+')) {
        int op = LastSym;
        return node_operator(op, 1, atom());
    }
    return atom();
}
Node *atom() {
    if(accept('(')) {
        Node *n = exprn();
        expect(')');
        return n;
    } else if(accept(ID)) {
		Node *name = node_id(LastWord);
		if(accept('(')) {
			Node *arr = node_operator(ARR, 1, name); 
			do {
				Node *exp = exprn();
				node_add_child(arr, exp);
			} while(accept(','));
			expect(')');
			return arr;
		} else {
			return node_operator(VARIABLE, 1, name);
		}
    } else if(accept(FUN) || accept(FN)) {
        Node *name = node_id(LastWord);
        expect('(');
        Node *arg = exprn();
        expect(')');
        return node_operator(FUN, 2, name, arg);
    } else if(accept(NUM)) {
        return node_number(atof(LastWord));
    } else if(accept(STR))
        return node_string(LastWord);
    error("value expected");
    return NULL;
}

char *readfile(const char *fname) {
    FILE *f;
    long len,r;
    char *str;

    if(!(f = fopen(fname, "rb")))
        return NULL;

    Filename = fname;

    fseek(f, 0, SEEK_END);
    len = ftell(f);
    rewind(f);

    if(!(str = malloc(len+2)))
        return NULL;
    r = fread(str, 1, len, f);

    if(r != len) {
        free(str);
        return NULL;
    }

    fclose(f);
    str[len] = '\0';
    return str;
}

const char *nodename(int type) {
		
    switch(type) {
     case FEND: return "EOF";
     case NL: return "newline";
     case PROG: return "<program>";
     case LINE: return "<line>";
     case ID: return "identifier";
     case NUM: return "number";
     case STR: return "string";
     case ARR: return "array access";
     case GTE: return "'>='";
     case LTE: return "'<='";
     case NEQ: return "'<>'";
     case EQ: return "'=='";
     case FUN: return "call";
     case VARIABLE: return "variable";
     case STMTS: return "stmts";
    }
    static char buffer[20];
    int i;
    if(type > 0x20 && type < 0xFF) {
        snprintf(buffer, sizeof buffer, "'%c'", type);
    } else {
        for(i = 0; Keywords[i].name; i++) {
            if(Keywords[i].value == type) {
                return Keywords[i].name;
            }
        }
        for(i = 0; Functions[i].name; i++) {
            if(Functions[i].value == type) {
                return Functions[i].name;
            }
        }
        snprintf(buffer, sizeof buffer, "{%d}", type);
    }
    return buffer;
}

int main(int argc, char *argv[]) {
    if(argc < 2) {
        fprintf(stderr, "no input file\n");
        return 1;
    }

    char *text = readfile(argv[1]);
    if(!text) {
        fprintf(stderr, "error reading %s\n", argv[1]);
        return 1;
    }

    init(text);

    Node * prog = program();

    fputs("/*==========\n", stdout);

    print_tree(prog);

    fputs("==========*/\n\n", stdout);

    compile(prog);

    free_node(prog);

    free(text);
    return 0;
}