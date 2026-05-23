#ifndef PADALANG_PARSER_H
#define PADALANG_PARSER_H

#include "ast.h"
#include "lexer.h"

typedef struct {
    Lexer *lex;
    Token current;
    int had_error;
    char error_message[256];
} Parser;

void parser_init(Parser *parser, Lexer *lex);
Ast *parse_program(Parser *parser);
const char *parser_get_error(const Parser *parser);

#endif
