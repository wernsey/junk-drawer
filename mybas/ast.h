enum NodeType {nt_opr, nt_num, nt_str, nt_id};

typedef struct Node {
    int line, basic_line;
    enum NodeType type;
    union {
        char *str;
        double num;
        struct {
            int op; /* operator */
            int nc, ac; /* num children, allocated children */
            struct Node **children;
        } opr;
    } u;
} Node;

Node *node_string(char *s);

Node *node_id(char *i);

Node *node_number(double num);

Node *node_operator(int op, int nargs, ...);

Node *node_add_child(Node *parent, Node *child);

void free_node(Node *node);

void print_tree(Node *node);
