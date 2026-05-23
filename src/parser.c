#include "parser.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void free_stmts_fallback(Ast **stmts, int count) {
    if (stmts == NULL) {
        return;
    }
    for (int i = 0; i < count; i++) {
        ast_free(stmts[i]);
    }
    free(stmts);
}

static void free_params_fallback(char **params, int count) {
    if (params == NULL) {
        return;
    }
    for (int i = 0; i < count; i++) {
        free(params[i]);
    }
    free(params);
}

static void advance(Parser *parser) {
    token_free(&parser->current);
    parser->current = lexer_next(parser->lex);

    if (parser->current.type == TOKEN_ERROR && !parser->had_error) {
        parser->had_error = 1;
        snprintf(parser->error_message, sizeof(parser->error_message), "%s",
                 parser->current.lexeme != NULL ? parser->current.lexeme : "Lexer error");
    }
}

static int check(Parser *parser, TokenType type) {
    return parser->current.type == type;
}

static int match(Parser *parser, TokenType type) {
    if (!check(parser, type)) {
        return 0;
    }
    advance(parser);
    return 1;
}

static void error_at(Parser *parser, const char *message) {
    if (parser->had_error) {
        return;
    }
    parser->had_error = 1;
    snprintf(parser->error_message, sizeof(parser->error_message),
             "Parse error at %d:%d: %s", parser->current.line, parser->current.column, message);
}

static Token consume(Parser *parser, TokenType type, const char *message) {
    if (!check(parser, type)) {
        error_at(parser, message);
        return parser->current;
    }
    Token tok = parser->current;
    parser->current.lexeme = NULL;
    advance(parser);
    return tok;
}

static char *take_lexeme(Token *tok) {
    char *lexeme = tok->lexeme;
    tok->lexeme = NULL;
    return lexeme;
}

static void skip_newlines(Parser *parser) {
    while (check(parser, TOKEN_NEWLINE)) {
        advance(parser);
    }
}

static int append_stmt(Ast ***stmts_ref, int *count, int *cap, Ast *stmt) {
    Ast **stmts = *stmts_ref;
    if (*count >= *cap) {
        *cap = *cap == 0 ? 8 : *cap * 2;
        Ast **next = (Ast **)realloc(stmts, (size_t)(*cap) * sizeof(Ast *));
        if (next == NULL) {
            return 0;
        }
        *stmts_ref = next;
        stmts = next;
    }
    stmts[(*count)++] = stmt;
    return 1;
}

static Ast *parse_expression(Parser *parser);
static Ast *parse_statement(Parser *parser);
static Ast *parse_block(Parser *parser);

static Ast *parse_primary(Parser *parser) {
    if (check(parser, TOKEN_INT)) {
        Token tok = consume(parser, TOKEN_INT, "Expected expression");
        long value = strtol(take_lexeme(&tok), NULL, 10);
        return ast_int(value, tok.line, tok.column);
    }

    if (check(parser, TOKEN_STRING)) {
        Token tok = consume(parser, TOKEN_STRING, "Expected expression");
        return ast_string(take_lexeme(&tok), tok.line, tok.column);
    }

    if (check(parser, TOKEN_IDENT)) {
        Token tok = consume(parser, TOKEN_IDENT, "Expected expression");
        char *name = take_lexeme(&tok);

        if (match(parser, TOKEN_LPAREN)) {
            Ast **args = NULL;
            int arg_count = 0;
            int arg_cap = 0;

            if (!check(parser, TOKEN_RPAREN)) {
                for (;;) {
                    Ast *arg = parse_expression(parser);
                    if (arg == NULL) {
                        free(name);
                        free_stmts_fallback(args, arg_count);
                        return NULL;
                    }

                    if (arg_count >= arg_cap) {
                        arg_cap = arg_cap == 0 ? 4 : arg_cap * 2;
                        Ast **next = (Ast **)realloc(args, (size_t)arg_cap * sizeof(Ast *));
                        if (next == NULL) {
                            ast_free(arg);
                            free(name);
                            free_stmts_fallback(args, arg_count);
                            error_at(parser, "Out of memory");
                            return NULL;
                        }
                        args = next;
                    }
                    args[arg_count++] = arg;

                    if (!check(parser, TOKEN_RPAREN)) {
                        error_at(parser, "Expected ')' after arguments");
                        free(name);
                        free_stmts_fallback(args, arg_count);
                        return NULL;
                    }
                    break;
                }
            }

            consume(parser, TOKEN_RPAREN, "Expected ')' after arguments");
            return ast_call(name, args, arg_count, tok.line, tok.column);
        }

        return ast_ident(name, tok.line, tok.column);
    }

    error_at(parser, "Expected expression");
    return NULL;
}

static Ast *parse_multiplication(Parser *parser) {
    Ast *left = parse_primary(parser);
    if (left == NULL) {
        return NULL;
    }

    while (check(parser, TOKEN_STAR)) {
        Token op = parser->current;
        advance(parser);
        Ast *right = parse_primary(parser);
        if (right == NULL) {
            ast_free(left);
            return NULL;
        }
        Ast *node = ast_binary(left, BINOP_STAR, right, op.line, op.column);
        if (node == NULL) {
            ast_free(left);
            ast_free(right);
            error_at(parser, "Out of memory");
            return NULL;
        }
        left = node;
    }

    return left;
}

static Ast *parse_addition(Parser *parser) {
    Ast *left = parse_multiplication(parser);
    if (left == NULL) {
        return NULL;
    }

    while (check(parser, TOKEN_PLUS)) {
        Token op = parser->current;
        advance(parser);
        Ast *right = parse_multiplication(parser);
        if (right == NULL) {
            ast_free(left);
            return NULL;
        }
        Ast *node = ast_binary(left, BINOP_PLUS, right, op.line, op.column);
        if (node == NULL) {
            ast_free(left);
            ast_free(right);
            error_at(parser, "Out of memory");
            return NULL;
        }
        left = node;
    }

    return left;
}

static Ast *parse_comparison(Parser *parser) {
    Ast *left = parse_addition(parser);
    if (left == NULL) {
        return NULL;
    }

    if (check(parser, TOKEN_GTE) || check(parser, TOKEN_GT)) {
        Token op = parser->current;
        BinOp binop = op.type == TOKEN_GTE ? BINOP_GTE : BINOP_GT;
        advance(parser);
        Ast *right = parse_addition(parser);
        if (right == NULL) {
            ast_free(left);
            return NULL;
        }
        Ast *node = ast_binary(left, binop, right, op.line, op.column);
        if (node == NULL) {
            ast_free(left);
            ast_free(right);
            error_at(parser, "Out of memory");
            return NULL;
        }
        return node;
    }

    return left;
}

static Ast *parse_expression(Parser *parser) {
    return parse_comparison(parser);
}

static Ast *parse_block(Parser *parser) {
    Token indent = consume(parser, TOKEN_INDENT, "Expected indented block");
    Ast **stmts = NULL;
    int count = 0;
    int cap = 0;

    skip_newlines(parser);
    while (!check(parser, TOKEN_DEDENT) && !check(parser, TOKEN_EOF)) {
        Ast *stmt = parse_statement(parser);
        if (stmt == NULL) {
            free_stmts_fallback(stmts, count);
            return NULL;
        }
        if (!append_stmt(&stmts, &count, &cap, stmt)) {
            ast_free(stmt);
            free_stmts_fallback(stmts, count);
            error_at(parser, "Out of memory");
            return NULL;
        }
        skip_newlines(parser);
    }

    consume(parser, TOKEN_DEDENT, "Expected end of block");
    return ast_block(stmts, count, indent.line, indent.column);
}

static Ast *parse_if(Parser *parser) {
    Token tok = consume(parser, TOKEN_BILO, "Expected 'bilo'");
    Ast *cond = parse_expression(parser);
    if (cond == NULL) {
        return NULL;
    }
    consume(parser, TOKEN_BARU, "Expected 'baru' after condition");
    skip_newlines(parser);
    Ast *then_branch = parse_block(parser);
    if (then_branch == NULL) {
        ast_free(cond);
        return NULL;
    }

    Ast *else_branch = NULL;
    skip_newlines(parser);
    if (match(parser, TOKEN_INDAK)) {
        skip_newlines(parser);
        else_branch = parse_block(parser);
        if (else_branch == NULL) {
            ast_free(cond);
            ast_free(then_branch);
            return NULL;
        }
    }

    return ast_if(cond, then_branch, else_branch, tok.line, tok.column);
}

static Ast *parse_func(Parser *parser) {
    Token tok = consume(parser, TOKEN_FUNGSI, "Expected 'fungsi'");
    Token name_tok = consume(parser, TOKEN_IDENT, "Expected function name");
    char *name = take_lexeme(&name_tok);

    consume(parser, TOKEN_LPAREN, "Expected '(' after function name");

    char **params = NULL;
    int param_count = 0;
    int param_cap = 0;

    if (!check(parser, TOKEN_RPAREN)) {
        for (;;) {
            Token param_tok = consume(parser, TOKEN_IDENT, "Expected parameter name");
            char *param = take_lexeme(&param_tok);

            if (param_count >= param_cap) {
                param_cap = param_cap == 0 ? 4 : param_cap * 2;
                char **next = (char **)realloc(params, (size_t)param_cap * sizeof(char *));
                if (next == NULL) {
                    free(param);
                    free(name);
                    free_params_fallback(params, param_count);
                    error_at(parser, "Out of memory");
                    return NULL;
                }
                params = next;
            }
            params[param_count++] = param;

            if (check(parser, TOKEN_RPAREN)) {
                break;
            }
            error_at(parser, "Expected ')' after parameters");
            free(name);
            free_params_fallback(params, param_count);
            return NULL;
        }
    }

    consume(parser, TOKEN_RPAREN, "Expected ')' after parameters");
    skip_newlines(parser);

    Ast *body = parse_block(parser);
    if (body == NULL) {
        free(name);
        free_params_fallback(params, param_count);
        return NULL;
    }

    return ast_func(name, params, param_count, body, tok.line, tok.column);
}

static Ast *parse_for(Parser *parser) {
    Token tok = consume(parser, TOKEN_UNTUK, "Expected 'untuak'");
    Token var_tok = consume(parser, TOKEN_IDENT, "Expected loop variable");
    char *var = take_lexeme(&var_tok);

    consume(parser, TOKEN_DARI, "Expected 'dari' after loop variable");
    Ast *from = parse_expression(parser);
    if (from == NULL) {
        free(var);
        return NULL;
    }

    consume(parser, TOKEN_SAMPAI, "Expected 'sampai' after range start");
    Ast *to = parse_expression(parser);
    if (to == NULL) {
        free(var);
        ast_free(from);
        return NULL;
    }

    skip_newlines(parser);
    Ast *body = parse_block(parser);
    if (body == NULL) {
        free(var);
        ast_free(from);
        ast_free(to);
        return NULL;
    }

    return ast_for(var, from, to, body, tok.line, tok.column);
}

static Ast *parse_expression_statement(Parser *parser) {
    Ast *expr = parse_expression(parser);
    if (expr == NULL) {
        return NULL;
    }

    if (expr->type == AST_IDENT && check(parser, TOKEN_ASSIGN)) {
        char *name = expr->as.ident.name;
        expr->as.ident.name = NULL;
        ast_free(expr);
        Token assign = parser->current;
        advance(parser);
        Ast *value = parse_expression(parser);
        if (value == NULL) {
            free(name);
            return NULL;
        }
        return ast_assign(name, value, assign.line, assign.column);
    }

    return ast_print_stmt(expr, expr->line, expr->column);
}

static Ast *parse_statement(Parser *parser) {
    if (check(parser, TOKEN_CALIAK)) {
        Token tok = parser->current;
        advance(parser);
        Ast *expr = parse_expression(parser);
        if (expr == NULL) {
            return NULL;
        }
        return ast_print_stmt(expr, tok.line, tok.column);
    }

    if (check(parser, TOKEN_SIMPAN)) {
        Token tok = consume(parser, TOKEN_SIMPAN, "Expected 'simpan'");
        Token name_tok = consume(parser, TOKEN_IDENT, "Expected variable name");
        char *name = take_lexeme(&name_tok);
        consume(parser, TOKEN_ASSIGN, "Expected '=' after variable name");
        Ast *value = parse_expression(parser);
        if (value == NULL) {
            free(name);
            return NULL;
        }
        return ast_assign(name, value, tok.line, tok.column);
    }

    if (check(parser, TOKEN_BILO)) {
        return parse_if(parser);
    }

    if (check(parser, TOKEN_FUNGSI)) {
        return parse_func(parser);
    }

    if (check(parser, TOKEN_BULIAN)) {
        Token tok = parser->current;
        advance(parser);
        Ast *expr = parse_expression(parser);
        if (expr == NULL) {
            return NULL;
        }
        return ast_return(expr, tok.line, tok.column);
    }

    if (check(parser, TOKEN_UNTUK)) {
        return parse_for(parser);
    }

    return parse_expression_statement(parser);
}

void parser_init(Parser *parser, Lexer *lex) {
    parser->lex = lex;
    parser->had_error = 0;
    parser->error_message[0] = '\0';
    parser->current.type = TOKEN_EOF;
    parser->current.lexeme = NULL;
    advance(parser);
}

Ast *parse_program(Parser *parser) {
    Ast **stmts = NULL;
    int count = 0;
    int cap = 0;

    skip_newlines(parser);
    while (!check(parser, TOKEN_EOF)) {
        Ast *stmt = parse_statement(parser);
        if (stmt == NULL) {
            free_stmts_fallback(stmts, count);
            return NULL;
        }
        if (!append_stmt(&stmts, &count, &cap, stmt)) {
            ast_free(stmt);
            free_stmts_fallback(stmts, count);
            error_at(parser, "Out of memory");
            return NULL;
        }
        skip_newlines(parser);
    }

    return ast_program(stmts, count);
}

const char *parser_get_error(const Parser *parser) {
    return parser->error_message;
}
