enum Symbol {
    FEND = 0, NL, ID, NUM, STR, ARR,
    LET,READ,DATA,PRINT,GOTO,IF,FOR,NEXT,END,
    DEF,GOSUB,RETURN,DIM,REM,TO,THEN,STEP,STOP,

    SIN=256,COS,TAN,ATN,EXP,ABS,LOG,SQR,RND,INT,FN,FUN,
    PROG, LINE, GTE, LTE, NEQ, EQ,
	VARIABLE, STMTS,
};

extern const char *Filename;

extern void error(const char *msg, ...);
extern const char *nodename(int type);
extern int Line, BasicLine;

struct Node;

extern void compile(struct Node *node);
