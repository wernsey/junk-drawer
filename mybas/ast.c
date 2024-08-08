/* http://epaperpress.com/lexandyacc/ */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <assert.h>

#include "ast.h"
#include "bas.h"

Node *node_string(char *s) {
    Node *node = malloc(sizeof * node);
    if(!node) return NULL;
    node->type = nt_str;
    node->u.str = strdup(s);
    node->line = Line;
    node->basic_line = BasicLine;
    return node;
}

Node *node_id(char *i) {
    Node *node = malloc(sizeof * node);
    if(!node) return NULL;
    node->type = nt_id;
    node->u.str = strdup(i);
    node->line = Line;
    node->basic_line = BasicLine;
    return node;
}

Node *node_number(double num) {
    Node *node = malloc(sizeof * node);
    if(!node) return NULL;
    node->type = nt_num;
    node->u.num = num;
    node->line = Line;
    node->basic_line = BasicLine;
    return node;
}

Node *node_operator(int op, int nargs, ...) {
    int i;
    va_list ap;

    Node *node = malloc(sizeof * node);
    if(!node) return NULL;
    node->type = nt_opr;
    node->u.opr.op = op;
    node->u.opr.nc = nargs;
    node->u.opr.ac = nargs;
    if(node->u.opr.ac == 0)
        node->u.opr.ac = 1;
    node->u.opr.children = calloc(node->u.opr.ac, sizeof *node->u.opr.children);
    if(!node->u.opr.children) {
        free(node);
        return NULL;
    }
    va_start(ap, nargs);
    for(i = 0; i < nargs; i++) {
        node->u.opr.children[i] = va_arg(ap, Node *);
    }
    va_end(ap);
    node->line = Line;
    node->basic_line = BasicLine;
    return node;
}

Node *node_add_child(Node *parent, Node *child) {
    int n;
    Node **re;
    assert(parent->type == nt_opr);

    n = parent->u.opr.nc;
    if(n == parent->u.opr.ac) {
        parent->u.opr.ac <<= 1;
        re = realloc(parent->u.opr.children, parent->u.opr.ac * sizeof *parent->u.opr.children);
        if(!re)
            return NULL;
        parent->u.opr.children = re;
    }
    assert(n < parent->u.opr.ac);
    parent->u.opr.children[n] = child;
    parent->u.opr.nc = n + 1;

    return parent;
}

void free_node(Node *node) {
    if(!node) return;
    switch(node->type) {
        case nt_id :
        case nt_str : free(node->u.str); break;
        case nt_opr : {
            int i;
            for(i = 0; i < node->u.opr.nc; i++)
                free_node(node->u.opr.children[i]);
            free(node->u.opr.children);
        } break;
        default : break;
    }
    free(node);
}

static void print_node(Node *node, int level) {
    int i;

    if(level) {
        printf("%*s-- ", level*2, "+");
    }
    if(!node) {
        printf("NULL\n");
        return;
    }

    switch(node->type) {
        case nt_num : printf("%g\n", node->u.num); break;
        case nt_id :  printf("%s\n", node->u.str); break;
        case nt_str : printf("\"%s\"\n", node->u.str); break;
        case nt_opr : {
            if(node->u.opr.op < 0x20 || node->u.opr.op >= 0xFF)
                printf("<%s>\n", nodename(node->u.opr.op));
            else if(node->u.opr.op < 128)
                printf("'%c'\n", node->u.opr.op);
            else
                printf("<%d>\n", (node->u.opr.op));
            for(i = 0; i < node->u.opr.nc; i++)
                print_node(node->u.opr.children[i], level+1);
        } break;
        default : break;
    }
}

void print_tree(Node *node) {
    print_node(node, 0);
}
