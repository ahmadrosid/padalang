#ifndef PADALANG_AST_H
#define PADALANG_AST_H

typedef enum {
    AST_PROGRAM,
    AST_PRINT,
    AST_ASSIGN,
    AST_IF,
    AST_FUNC,
    AST_RETURN,
    AST_FOR,
    AST_BLOCK,
    AST_BINARY,
    AST_INT,
    AST_STRING,
    AST_IDENT,
    AST_CALL
} AstType;

typedef enum {
    BINOP_PLUS,
    BINOP_STAR,
    BINOP_GT,
    BINOP_GTE
} BinOp;

typedef struct Ast Ast;

struct Ast {
    AstType type;
    int line;
    int column;
    union {
        struct {
            Ast **stmts;
            int count;
        } program;
        struct {
            Ast *expr;
        } print;
        struct {
            char *name;
            Ast *value;
        } assign;
        struct {
            Ast *cond;
            Ast *then_branch;
            Ast *else_branch;
        } if_stmt;
        struct {
            char *name;
            char **params;
            int param_count;
            Ast *body;
        } func;
        struct {
            Ast *expr;
        } ret;
        struct {
            char *var;
            Ast *from;
            Ast *to;
            Ast *body;
        } for_stmt;
        struct {
            Ast **stmts;
            int count;
        } block;
        struct {
            Ast *left;
            BinOp op;
            Ast *right;
        } binary;
        struct {
            long value;
        } integer;
        struct {
            char *value;
        } string;
        struct {
            char *name;
        } ident;
        struct {
            char *name;
            Ast **args;
            int arg_count;
        } call;
    } as;
};

Ast *ast_program(Ast **stmts, int count);
Ast *ast_print_stmt(Ast *expr, int line, int column);
Ast *ast_assign(char *name, Ast *value, int line, int column);
Ast *ast_if(Ast *cond, Ast *then_branch, Ast *else_branch, int line, int column);
Ast *ast_func(char *name, char **params, int param_count, Ast *body, int line, int column);
Ast *ast_return(Ast *expr, int line, int column);
Ast *ast_for(char *var, Ast *from, Ast *to, Ast *body, int line, int column);
Ast *ast_block(Ast **stmts, int count, int line, int column);
Ast *ast_binary(Ast *left, BinOp op, Ast *right, int line, int column);
Ast *ast_int(long value, int line, int column);
Ast *ast_string(char *value, int line, int column);
Ast *ast_ident(char *name, int line, int column);
Ast *ast_call(char *name, Ast **args, int arg_count, int line, int column);

void ast_tree_print(const Ast *node, int depth);
void ast_free(Ast *node);

const char *binop_name(BinOp op);

#endif
