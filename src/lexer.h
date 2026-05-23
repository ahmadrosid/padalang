#ifndef PADALANG_LEXER_H
#define PADALANG_LEXER_H

#include "token.h"

typedef struct {
    const char *source;
    const char *start;
    const char *current;
    int line;
    int column;
    int *indent_stack;
    int indent_depth;
    int indent_cap;
    int at_line_start;
    int pending_dedent;
} Lexer;

void lexer_init(Lexer *lex, const char *source);
Token lexer_next(Lexer *lex);
void lexer_free(Lexer *lex);

#endif
