#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "bas.h"
#include "ast.h"
#include "hash.h"

static FILE *o;

static hash_table LoopVariables;
static hash_table DimmedVariables;

/* Tracks destinations of GOTOs and GOSUBs to
shut GCC up about labels being defined but not used: */
static hash_table Destinations;

static int NextDest = 1;
static int NodeLine = -1;

int DataCount = 0;

static Node *search_for(Node *node, int operator) {
    if(node && node->type == nt_opr) {
        int i;
        if(node->u.opr.op == operator)
            return node;
        for(i = 0; i < node->u.opr.nc; i++) {
            Node *c = node->u.opr.children[i];
            if(search_for(c, operator))
                return c;
        }
    }
    return NULL;
}

typedef void (*visit_fun)(Node *);
static void visit(Node *node, int operator, visit_fun visitor) {
    if(node->type == nt_opr) {
        int i;
        if(node->u.opr.op == operator)
            visitor(node);
        for(i = 0; i < node->u.opr.nc; i++)
            visit(node->u.opr.children[i], operator, visitor);
    }
}

void outline(Node *node) {
	if(node->line != NodeLine) {
		fprintf(o, "#line %d \"%s\"\n  ", node->line, Filename);
		NodeLine = node->line;
	}
}
static void walk(Node *node) {
    int i;
    Line = node->line;
    BasicLine = node->basic_line;
    switch(node->type) {
        case nt_num: fprintf(o, "%g", node->u.num); break;
        case nt_str: fprintf(o, "\"%s\"", node->u.str); break;
        case nt_id: fprintf(o, "%s", node->u.str); break;
        case nt_opr: {
            switch(node->u.opr.op) {
                case PROG: {
                    for(i = 0; i < node->u.opr.nc; i++)
                        walk(node->u.opr.children[i]);
                } break;
                case STMTS: {
					outline(node);
					if(node->u.opr.nc>1) {
						fputs("{", o);
						for(i = 0; i < node->u.opr.nc; i++) {
							walk(node->u.opr.children[i]);
						}
						fputs("}\n", o);
					} else {						
						for(i = 0; i < node->u.opr.nc; i++)
							walk(node->u.opr.children[i]);
						fputs("\n", o);
					}
                } break;
                case LINE: {
                    if(node->u.opr.nc == 2) {
                        Node *child = node->u.opr.children[0];
                        if(child->type == nt_num) {
                            char name[20];
                            snprintf(name, sizeof name, "lbl_%d", (int)child->u.num);
                            if(ht_get(Destinations, name))
                                fprintf(o, "%s:\n", name);
                        } else if(child->type == nt_id) {
                            if(ht_get(Destinations, child->u.str))
                                fprintf(o, "%s:\n", child->u.str);
                        }
                        walk(node->u.opr.children[1]);
                    } else {
                        assert(node->u.opr.nc == 1);
                        walk(node->u.opr.children[0]);
                    }
                } break;
                case VARIABLE: {
					assert(node->u.opr.nc == 1);
                    walk(node->u.opr.children[0]);
				} break;
                case LET: {
                    assert(node->u.opr.nc == 2);
                    walk(node->u.opr.children[0]);
                    fputs(" = ", o);
                    walk(node->u.opr.children[1]);
                    fputs(";", o);
                } break;
                case IF: {
                    assert(node->u.opr.nc == 2);
                    fputs("if(", o);
                    walk(node->u.opr.children[0]);
                    fputs(")", o);
                    walk(node->u.opr.children[1]);					
                } break;
                case FOR: {
					fputs("{", o);
					Node *loopVar = node->u.opr.children[0];
					assert(loopVar->type == nt_opr && loopVar->u.opr.op == VARIABLE && loopVar->u.opr.nc == 1);
					loopVar = loopVar->u.opr.children[0];					
                    assert(loopVar->type == nt_id);
                    char * looper = loopVar->u.str;
                    fprintf(o, "%s=", looper);
                    walk(node->u.opr.children[1]);
                    fputs(";", o);
                    fprintf(o, "loop_%s_to=", looper);
                    walk(node->u.opr.children[2]);
                    fputs(";", o);
                    if(node->u.opr.nc > 3) {
                        fprintf(o, "loop_%s_step=", looper);
                        walk(node->u.opr.children[3]);
                        fputs(";", o);
                    } else
                        fprintf(o, "loop_%s_step=1;", looper);
                    fprintf(o, "loop_%s=%d;}\n", looper, NextDest);
                    fprintf(o, " case %d:\n", NextDest++);
                } break;
                case NEXT: {
					Node *loopVar = node->u.opr.children[0];
					assert(loopVar->type == nt_opr && loopVar->u.opr.op == VARIABLE && loopVar->u.opr.nc == 1);
					loopVar = loopVar->u.opr.children[0];
                    char * looper = loopVar->u.str;
                    if(!ht_get(LoopVariables, looper))
                        error("NEXT %s does not have an associated FOR", looper);
                    fprintf(o, "if(%s<loop_%s_to){%s+=loop_%s_step;addr=loop_%s;goto jump;}", looper, looper, looper, looper, looper);
                } break;
                case GOTO: {
					assert(node->u.opr.nc == 1);
                    fputs("goto lbl_", o);
                    walk(node->u.opr.children[0]);
                    fputs(";", o);
                } break;
                case GOSUB: {
					fprintf(o, "{ret=%d;", NextDest);
                    assert(node->u.opr.nc == 1);
                    fputs("goto lbl_", o);
                    walk(node->u.opr.children[0]);
					fputs(";}\n", o);
                    fprintf(o, " case %d:\n", NextDest++);
                } break;
                case RETURN: {
					fputs("{addr=ret;goto jump;}",o);
                } break;
                case END: case STOP: {
					fputs("return;",o);
                } break;
                case PRINT: {
					fputs("{",o);
					int nl = 1;
                    for(i = 0; i < node->u.opr.nc; i++) {
                        nl = 1;
                        Node *c = node->u.opr.children[i];
                        if(c->type == nt_num) {
                            fprintf(o, "bprint(\"%%g\",(num_type)%g);", c->u.num);
                        } else if(c->type == nt_id){
                            fprintf(o, "bprint(\"%%g\",%s);", c->u.str);
                        } else if(c->type == nt_str){
                            fprintf(o, "fputs(\"%s\",stdout);Col+=%d;", c->u.str, strlen(c->u.str));
                        } else if(c->type == nt_opr) {
							if(c->u.opr.op == ',') {
								nl = 0;
								printf("pad(15);");
							} else if(c->u.opr.op == ';') {
								nl = 0;
								printf("pad(3);");
							} else {
								fputs("bprint(\"%g\", (num_type)", o);
								walk(c);
								fputs(");", o);
							}
                        }
                    }
                    if(nl) fputs("fputs(\"\\n\", stdout);Col=0;", o);
                    fputs("}", o);					
                } break;
                case READ: {
					if(DataCount == 0)
                        error("READ with no data");
					fputs("{", o);
                    for(i = 0; i < node->u.opr.nc; i++) {
                        fprintf(o, "READ(");
                        walk(node->u.opr.children[i]);
                        fputs(");",o);
                    }
					fputs("}",o);
                } break;
                case FUN: {
                    walk(node->u.opr.children[0]);
                    fputs("(",o);
                    walk(node->u.opr.children[1]);
                    fputs(")",o);
                } break;
                case ARR: {
                    int ndims = node->u.opr.nc - 1;
					Node *name = node->u.opr.children[0];
					char *nm = name->u.str;
					fprintf(o, "*arr(%s, %s_dims, %s_size, %d, ", nm, nm, nm, ndims);
                    for(i = 1; i < node->u.opr.nc; i++) {
						fputs("(int)", o);
						walk(node->u.opr.children[i]);
						if(i < node->u.opr.nc-1) fputs(",", o);
					}
					fputs(")", o);
                } break;
                case DIM: break;
                case DATA: break;
                case DEF: break;
                case '+':case '-':
                    if(node->u.opr.nc == 1) {
                        /* unary */
                        fprintf(o, "%c", node->u.opr.op);
                        walk(node->u.opr.children[0]);
                        break;
                    } /* else drop through... */
                case '*':case '/':case '%':case '>':case '<': {
                    assert(node->u.opr.nc == 2);
                    fputs("(", o);
                    walk(node->u.opr.children[0]);
                    fprintf(o, "%c", node->u.opr.op);
                    walk(node->u.opr.children[1]);
                    fputs(")", o);
                } break;
                case GTE:case LTE: case NEQ: case EQ: {
                    const char *ops[] = {">=", "<=", "!=", "=="};
                    assert(node->u.opr.nc == 2);
                    fputs("(", o);
                    walk(node->u.opr.children[0]);
                    fprintf(o, "%s", ops[node->u.opr.op - GTE]);
                    walk(node->u.opr.children[1]);
                    fputs(")", o);
                } break;
                case '^': {
                    assert(node->u.opr.nc == 2);
                    fputs("pow(", o);
                    walk(node->u.opr.children[0]);
                    fputs(",", o);
                    walk(node->u.opr.children[1]);
                    fputs(")", o);
                } break;
                default: error("unimplemented node %s [line %d]", nodename(node->u.opr.op), node->line);
            }
        } break;
    }
}

void evaluate_dims(Node *node) {
	int i, j;
	for(i = 0; i < node->u.opr.nc; i++) {
		Node *var = node->u.opr.children[i];
		fprintf(o, "#line %d \"%s\"\n", var->line, Filename);
		if(var->type == nt_id) {
			fprintf(o, "static num_type %s;\n", var->u.str);
			ht_put(DimmedVariables, var->u.str, node);
		} else {
			assert(var->type == nt_opr && var->u.opr.op == ARR);
			Node *name = var->u.opr.children[0];
			ht_put(DimmedVariables, name->u.str, node);
			
			fprintf(o, "static num_type %s[", name->u.str);
			for(j = 1; j < var->u.opr.nc; j++) {
				Node *child = var->u.opr.children[j];				
				int size = (int)child->u.num;
				fprintf(o, "%d*", size + 1);
			}
			fputs("1];\n", o);
			
			fprintf(o, "static int %s_dims=%d, %s_size[]={", name->u.str, var->u.opr.nc - 1, name->u.str);
			for(j = 1; j < var->u.opr.nc; j++) {
				Node *child = var->u.opr.children[j];				
				int size = (int)child->u.num;
				fprintf(o, "%d,", size + 1);
			}
			fputs("};\n", o);
		}
	}
}

void find_undimmed(Node *node) {
	Node *var = node->u.opr.children[0];
	assert(var->type == nt_id);
	if(node->u.opr.op == VARIABLE && var->type == nt_id) {
		if(!ht_get(DimmedVariables, var->u.str)) {
			fprintf(o, "#line %d \"%s\"\n", var->line, Filename);
			fprintf(o, "static num_type %s;/*Undimmed*/\n", var->u.str);
			ht_put(DimmedVariables, var->u.str, var);
		}
	} else if(node->u.opr.op == ARR && var->type == nt_id) {
		if(!ht_get(DimmedVariables, var->u.str)) {
			char *name = var->u.str;
			fprintf(o, "#line %d \"%s\"\n", var->line, Filename);
			fprintf(o, "static num_type %s[11];/*Undimmed*/\n", name);
			fprintf(o, "static int %s_dims=1, %s_size[]={11,};\n", name, name);
			ht_put(DimmedVariables, var->u.str, var);
		}
	} 
}

void evaluate_data(Node *node) {
	int i;
	for(i = 0; i < node->u.opr.nc; i++) {
		DataCount++;
		walk(node->u.opr.children[i]);
		fputc(',', o);
	}
}

void evaluate_def(Node *node) {
	assert(node->type == nt_opr && node->u.opr.op == DEF);
	fprintf(o, "#line %d \"%s\"\n", node->line, Filename);
	fprintf(o, "num_type ");
	walk(node->u.opr.children[0]);
	fprintf(o, "(num_type ");
	walk(node->u.opr.children[1]);
	fputs("){return ", o);
	walk(node->u.opr.children[2]);
	fputs(";}\n", o);
}
	
void mark_loop(Node *node) {
    Node *loopVar = node->u.opr.children[0];
    assert(loopVar->type == nt_opr && loopVar->u.opr.op == VARIABLE && loopVar->u.opr.nc == 1);
    loopVar = loopVar->u.opr.children[0];
    assert(loopVar->type == nt_id);
    ht_put(LoopVariables, loopVar->u.str, node);
}

void mark_dest(Node *node) {
    assert(node->type == nt_opr && node->u.opr.nc == 1);
    Node *child = node->u.opr.children[0];
    if(child->type == nt_num) {
        char name[20];
        snprintf(name, sizeof name, "lbl_%d", (int)child->u.num);
        ht_put(Destinations, name, node);
    } else if(child->type == nt_id) {
        ht_put(Destinations, child->u.str, node);
    } else
        error("unexpected destination type %d on %s node", child->type, nodename(node->u.opr.op));
}

void compile(Node *node) {
    const char *k;
	int has_print = 0;

    o = stdout;

    fputs("#include <stdio.h>\n", o);
    fputs("#include <stdlib.h>\n", o);
    fputs("#include <math.h>\n", o);
    fputs("#include <string.h>\n", o);
    fputs("#include <stdarg.h>\n", o);
    fputs("#include <time.h>\n", o);
    fputs("#include <assert.h>\n", o);
	
    fputs("typedef double num_type;\n", o);

    fputs("#define SIN(a) sin(a)\n", o);
    fputs("#define COS(a) cos(a)\n", o);
    fputs("#define TAN(a) tan(a)\n", o);
    fputs("#define ATN(a) atan(a)\n", o);
    fputs("#define EXP(a) exp(a)\n", o);
    fputs("#define ABS(a) fabs(a)\n", o);
    fputs("#define LOG(a) log(a)\n", o);
    fputs("#define SQR(a) sqrt(a)\n", o);
    fputs("#define RND(a) (rand()%((int)(a)))\n", o);
    fputs("#define INT(a) ((int)(a))\n", o);
    fputs("#define READ(a) assert(read < DATA_SIZE); a = Data[read++]\n", o);

	if(search_for(node, PRINT)) {
	  has_print=1;
	  fputs("static int Col = 0;",o);
	  fputs("static void bprint(const char *msg, ...) {",o);
	  fputs(" char buffer[20];",o);
	  fputs(" va_list ap;",o);
	  fputs(" va_start(ap, msg);",o);
	  fputs(" vsnprintf(buffer, sizeof buffer, msg, ap);",o);
	  fputs(" fputs(buffer,stdout);",o);
	  fputs(" va_end(ap);",o);
	  fputs(" Col+=strlen(buffer);",o);
	  fputs("}\n",o);	
	  fputs("static void pad(int p) {",o);
	  fputs(" fputc(' ', stdout);",o);
	  fputs(" Col++;",o);
	  fputs(" while(Col % p) {",o);
	  fputs("  fputc(' ', stdout);",o);
	  fputs("  Col++;",o);
	  fputs(" }",o);
	  fputs("}\n",o);
	}
	
	fputs("static num_type *arr(num_type a[], int dims, int sizes[], int n, ...) {", o);
	fputs(" int idx=0, i;", o);
	fputs(" va_list ap;", o);
	fputs(" va_start(ap, n);", o);
	fputs(" assert(n==dims);", o);
	fputs(" for(i=0;i<n;i++){", o);
	fputs("  int s = va_arg(ap, int);", o);
	//fputs(" printf(\"-- arr(%p, size[%d]=%d) => %d\\n\", a, i, sizes[i], s);", o);
	fputs("  assert(s < sizes[i]);", o);
	fputs("  idx*=sizes[i];", o);
	fputs("  idx += s;", o);
	fputs(" }", o);
	fputs(" va_end(ap);", o);
	//fputs(" printf(\"arr(%p, %d, %d): %p\\n\", a, n, idx, &a[idx]);", o);
	fputs(" return a+idx;", o);
	fputs("}\n", o);		
	
    ht_init(DimmedVariables);
	visit(node, DIM, evaluate_dims);
	
	/* Find variables that aren't DIMmed */
	visit(node, VARIABLE, find_undimmed);
	visit(node, ARR, find_undimmed);

    ht_init(LoopVariables);
    visit(node, FOR, mark_loop);

    ht_init(Destinations);
    visit(node, GOTO, mark_dest);
    visit(node, GOSUB, mark_dest);
	
    for(k = ht_next(LoopVariables, NULL); k; k = ht_next(LoopVariables, k)) {
        fprintf(o, "int loop_%s; num_type loop_%s_to, loop_%s_step;\n", k, k, k);
    }
	
	DataCount = 0;
	fputs("static num_type Data[] = {", o);
	visit(node, DATA, evaluate_data);
    fputs("};\n", o);
    fprintf(o, "#define DATA_SIZE %d\n", DataCount);
	
	visit(node, DEF, evaluate_def);
    
    fputs("\n", o);
    fputs("void run(){\n", o);
    fputs(" int addr=0,ret=0,read=0;\n", o);
    fputs(" (void)addr;(void)ret;(void)read;(void)Data;\n", o);
    if(has_print) fputs(" (void)pad;(void)bprint;\n", o);
	
    if(search_for(node, RETURN) || search_for(node, FOR)) {
        /* Don't generate the `jump:switch()` if the program does
        not contain any `FOR` or `RETURN` statements else `gcc -Wall` will
        complain about label jump being defined but not used  */
        fputs(" jump:switch(addr){\n", o);
        fputs(" case 0:\n", o);
        walk(node);
        fputs(" }\n", o);
    } else {
        walk(node);
    }
    fputs("}\n", o);
    fputs("int main(int argc, char *argv[]){srand(time(NULL));run();return 0;}\n", o);
		
	ht_destroy(DimmedVariables, NULL);
	ht_destroy(LoopVariables, NULL);
	ht_destroy(Destinations, NULL);
}
