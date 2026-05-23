#include "ast.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static Ast *alloc_node(AstType type, int line, int column) {
    Ast *node = (Ast *)calloc(1, sizeof(Ast));
    if (node == NULL) {
        return NULL;
    }
    node->type = type;
    node->line = line;
    node->column = column;
    return node;
}

const char *binop_name(BinOp op) {
    switch (op) {
    case BINOP_PLUS: return "+";
    case BINOP_STAR: return "*";
    case BINOP_GT: return ">";
    case BINOP_GTE: return ">=";
    }
    return "?";
}

Ast *ast_program(Ast **stmts, int count) {
    Ast *node = alloc_node(AST_PROGRAM, 1, 1);
    if (node == NULL) {
        return NULL;
    }
    node->as.program.stmts = stmts;
    node->as.program.count = count;
    return node;
}

Ast *ast_print_stmt(Ast *expr, int line, int column) {
    Ast *node = alloc_node(AST_PRINT, line, column);
    if (node == NULL) {
        return NULL;
    }
    node->as.print.expr = expr;
    return node;
}

Ast *ast_assign(char *name, Ast *value, int line, int column) {
    Ast *node = alloc_node(AST_ASSIGN, line, column);
    if (node == NULL) {
        return NULL;
    }
    node->as.assign.name = name;
    node->as.assign.value = value;
    return node;
}

Ast *ast_if(Ast *cond, Ast *then_branch, Ast *else_branch, int line, int column) {
    Ast *node = alloc_node(AST_IF, line, column);
    if (node == NULL) {
        return NULL;
    }
    node->as.if_stmt.cond = cond;
    node->as.if_stmt.then_branch = then_branch;
    node->as.if_stmt.else_branch = else_branch;
    return node;
}

Ast *ast_func(char *name, char **params, int param_count, Ast *body, int line, int column) {
    Ast *node = alloc_node(AST_FUNC, line, column);
    if (node == NULL) {
        return NULL;
    }
    node->as.func.name = name;
    node->as.func.params = params;
    node->as.func.param_count = param_count;
    node->as.func.body = body;
    return node;
}

Ast *ast_return(Ast *expr, int line, int column) {
    Ast *node = alloc_node(AST_RETURN, line, column);
    if (node == NULL) {
        return NULL;
    }
    node->as.ret.expr = expr;
    return node;
}

Ast *ast_for(char *var, Ast *from, Ast *to, Ast *body, int line, int column) {
    Ast *node = alloc_node(AST_FOR, line, column);
    if (node == NULL) {
        return NULL;
    }
    node->as.for_stmt.var = var;
    node->as.for_stmt.from = from;
    node->as.for_stmt.to = to;
    node->as.for_stmt.body = body;
    return node;
}

Ast *ast_block(Ast **stmts, int count, int line, int column) {
    Ast *node = alloc_node(AST_BLOCK, line, column);
    if (node == NULL) {
        return NULL;
    }
    node->as.block.stmts = stmts;
    node->as.block.count = count;
    return node;
}

Ast *ast_binary(Ast *left, BinOp op, Ast *right, int line, int column) {
    Ast *node = alloc_node(AST_BINARY, line, column);
    if (node == NULL) {
        return NULL;
    }
    node->as.binary.left = left;
    node->as.binary.op = op;
    node->as.binary.right = right;
    return node;
}

Ast *ast_int(long value, int line, int column) {
    Ast *node = alloc_node(AST_INT, line, column);
    if (node == NULL) {
        return NULL;
    }
    node->as.integer.value = value;
    return node;
}

Ast *ast_string(char *value, int line, int column) {
    Ast *node = alloc_node(AST_STRING, line, column);
    if (node == NULL) {
        return NULL;
    }
    node->as.string.value = value;
    return node;
}

Ast *ast_ident(char *name, int line, int column) {
    Ast *node = alloc_node(AST_IDENT, line, column);
    if (node == NULL) {
        return NULL;
    }
    node->as.ident.name = name;
    return node;
}

Ast *ast_call(char *name, Ast **args, int arg_count, int line, int column) {
    Ast *node = alloc_node(AST_CALL, line, column);
    if (node == NULL) {
        return NULL;
    }
    node->as.call.name = name;
    node->as.call.args = args;
    node->as.call.arg_count = arg_count;
    return node;
}

static void print_indent(int depth) {
    for (int i = 0; i < depth; i++) {
        printf("  ");
    }
}

static void ast_tree_print_stmts(const Ast **stmts, int count, int depth);

void ast_tree_print(const Ast *node, int depth) {
    if (node == NULL) {
        print_indent(depth);
        printf("(null)\n");
        return;
    }

    print_indent(depth);

    switch (node->type) {
    case AST_PROGRAM:
        printf("Program (%d stmts)\n", node->as.program.count);
        ast_tree_print_stmts((const Ast **)node->as.program.stmts, node->as.program.count, depth + 1);
        break;
    case AST_PRINT:
        printf("Print\n");
        ast_tree_print(node->as.print.expr, depth + 1);
        break;
    case AST_ASSIGN:
        printf("Assign(%s)\n", node->as.assign.name);
        ast_tree_print(node->as.assign.value, depth + 1);
        break;
    case AST_IF:
        printf("If\n");
        print_indent(depth + 1);
        printf("Cond:\n");
        ast_tree_print(node->as.if_stmt.cond, depth + 2);
        print_indent(depth + 1);
        printf("Then:\n");
        ast_tree_print(node->as.if_stmt.then_branch, depth + 2);
        if (node->as.if_stmt.else_branch != NULL) {
            print_indent(depth + 1);
            printf("Else:\n");
            ast_tree_print(node->as.if_stmt.else_branch, depth + 2);
        }
        break;
    case AST_FUNC:
        printf("Func(%s, %d params)\n", node->as.func.name, node->as.func.param_count);
        ast_tree_print(node->as.func.body, depth + 1);
        break;
    case AST_RETURN:
        printf("Return\n");
        ast_tree_print(node->as.ret.expr, depth + 1);
        break;
    case AST_FOR:
        printf("For(%s)\n", node->as.for_stmt.var);
        print_indent(depth + 1);
        printf("From:\n");
        ast_tree_print(node->as.for_stmt.from, depth + 2);
        print_indent(depth + 1);
        printf("To:\n");
        ast_tree_print(node->as.for_stmt.to, depth + 2);
        print_indent(depth + 1);
        printf("Body:\n");
        ast_tree_print(node->as.for_stmt.body, depth + 2);
        break;
    case AST_BLOCK:
        printf("Block (%d stmts)\n", node->as.block.count);
        ast_tree_print_stmts((const Ast **)node->as.block.stmts, node->as.block.count, depth + 1);
        break;
    case AST_BINARY:
        printf("Binary(%s)\n", binop_name(node->as.binary.op));
        ast_tree_print(node->as.binary.left, depth + 1);
        ast_tree_print(node->as.binary.right, depth + 1);
        break;
    case AST_INT:
        printf("Int(%ld)\n", node->as.integer.value);
        break;
    case AST_STRING:
        printf("String(\"%s\")\n", node->as.string.value);
        break;
    case AST_IDENT:
        printf("Ident(%s)\n", node->as.ident.name);
        break;
    case AST_CALL:
        printf("Call(%s, %d args)\n", node->as.call.name, node->as.call.arg_count);
        for (int i = 0; i < node->as.call.arg_count; i++) {
            ast_tree_print(node->as.call.args[i], depth + 1);
        }
        break;
    }
}

static void ast_tree_print_stmts(const Ast **stmts, int count, int depth) {
    for (int i = 0; i < count; i++) {
        ast_tree_print(stmts[i], depth);
    }
}

static void free_stmts(Ast **stmts, int count) {
    if (stmts == NULL) {
        return;
    }
    for (int i = 0; i < count; i++) {
        ast_free(stmts[i]);
    }
    free(stmts);
}

void ast_free(Ast *node) {
    if (node == NULL) {
        return;
    }

    switch (node->type) {
    case AST_PROGRAM:
        free_stmts(node->as.program.stmts, node->as.program.count);
        break;
    case AST_PRINT:
        ast_free(node->as.print.expr);
        break;
    case AST_ASSIGN:
        free(node->as.assign.name);
        ast_free(node->as.assign.value);
        break;
    case AST_IF:
        ast_free(node->as.if_stmt.cond);
        ast_free(node->as.if_stmt.then_branch);
        ast_free(node->as.if_stmt.else_branch);
        break;
    case AST_FUNC:
        free(node->as.func.name);
        if (node->as.func.params != NULL) {
            for (int i = 0; i < node->as.func.param_count; i++) {
                free(node->as.func.params[i]);
            }
            free(node->as.func.params);
        }
        ast_free(node->as.func.body);
        break;
    case AST_RETURN:
        ast_free(node->as.ret.expr);
        break;
    case AST_FOR:
        free(node->as.for_stmt.var);
        ast_free(node->as.for_stmt.from);
        ast_free(node->as.for_stmt.to);
        ast_free(node->as.for_stmt.body);
        break;
    case AST_BLOCK:
        free_stmts(node->as.block.stmts, node->as.block.count);
        break;
    case AST_BINARY:
        ast_free(node->as.binary.left);
        ast_free(node->as.binary.right);
        break;
    case AST_STRING:
        free(node->as.string.value);
        break;
    case AST_IDENT:
        free(node->as.ident.name);
        break;
    case AST_CALL:
        free(node->as.call.name);
        for (int i = 0; i < node->as.call.arg_count; i++) {
            ast_free(node->as.call.args[i]);
        }
        free(node->as.call.args);
        break;
    case AST_INT:
        break;
    }

    free(node);
}
