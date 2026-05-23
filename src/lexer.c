#include "lexer.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#define INDENT_WIDTH 4
#define INITIAL_INDENT_CAP 8

typedef struct {
    const char *name;
    TokenType type;
} Keyword;

static const Keyword keywords[] = {
    {"atau", TOKEN_ATAU},
    {"baru", TOKEN_BARU},
    {"bilo", TOKEN_BILO},
    {"bulian", TOKEN_BULIAN},
    {"caliak", TOKEN_CALIAK},
    {"dari", TOKEN_DARI},
    {"fungsi", TOKEN_FUNGSI},
    {"indak", TOKEN_INDAK},
    {"jo", TOKEN_JO},
    {"sampai", TOKEN_SAMPAI},
    {"simpan", TOKEN_SIMPAN},
    {"untuak", TOKEN_UNTUK},
};

static int is_at_end(Lexer *lex) {
    return *lex->current == '\0';
}

static char peek(Lexer *lex) {
    return *lex->current;
}

static char advance(Lexer *lex) {
    char c = *lex->current++;
    if (c == '\n') {
        lex->line++;
        lex->column = 1;
    } else {
        lex->column++;
    }
    return c;
}

static int match(Lexer *lex, char expected) {
    if (is_at_end(lex) || peek(lex) != expected) {
        return 0;
    }
    advance(lex);
    return 1;
}

static Token make_token(Lexer *lex, TokenType type) {
    Token tok;
    tok.type = type;
    tok.lexeme = NULL;
    tok.line = lex->line;
    tok.column = lex->column;
    return tok;
}

static Token error_token(Lexer *lex, const char *message) {
    Token tok = make_token(lex, TOKEN_ERROR);
    tok.lexeme = strdup(message);
    if (tok.lexeme == NULL) {
        tok.lexeme = (char *)"Out of memory";
    }
    return tok;
}

static char *copy_lexeme(Lexer *lex) {
    size_t length = (size_t)(lex->current - lex->start);
    char *lexeme = (char *)malloc(length + 1);
    if (lexeme == NULL) {
        return NULL;
    }
    memcpy(lexeme, lex->start, length);
    lexeme[length] = '\0';
    return lexeme;
}

static void skip_whitespace(Lexer *lex) {
    for (;;) {
        char c = peek(lex);
        if (c == ' ') {
            advance(lex);
        } else if (c == '\r') {
            advance(lex);
        } else {
            break;
        }
    }
}

static void skip_comment(Lexer *lex) {
    while (!is_at_end(lex) && peek(lex) != '\n') {
        advance(lex);
    }
}

static int is_blank_or_comment_line(Lexer *lex) {
    const char *cursor = lex->current;
    int line = lex->line;
    int column = lex->column;

    while (*cursor == ' ') {
        cursor++;
        column++;
    }

    if (*cursor == '#') {
        while (*cursor != '\0' && *cursor != '\n') {
            cursor++;
        }
    }

    if (*cursor == '\0' || *cursor == '\n') {
        return 1;
    }

    (void)line;
    (void)column;
    return 0;
}

static int measure_indent(Lexer *lex, int *spaces_out) {
    int spaces = 0;

    while (peek(lex) == ' ') {
        spaces++;
        advance(lex);
    }

    if (peek(lex) == '\t') {
        return 0;
    }

    *spaces_out = spaces;
    return 1;
}

static int push_indent(Lexer *lex, int spaces) {
    if (lex->indent_depth >= lex->indent_cap) {
        int new_cap = lex->indent_cap * 2;
        int *stack = (int *)realloc(lex->indent_stack, (size_t)new_cap * sizeof(int));
        if (stack == NULL) {
            return 0;
        }
        lex->indent_stack = stack;
        lex->indent_cap = new_cap;
    }

    lex->indent_stack[lex->indent_depth++] = spaces;
    return 1;
}

static TokenType lookup_keyword(const char *lexeme) {
    size_t count = sizeof(keywords) / sizeof(keywords[0]);
    for (size_t i = 0; i < count; i++) {
        if (strcmp(lexeme, keywords[i].name) == 0) {
            return keywords[i].type;
        }
    }
    return TOKEN_IDENT;
}

static Token string_token(Lexer *lex) {
    int line = lex->line;
    int column = lex->column;

    while (!is_at_end(lex) && peek(lex) != '"') {
        if (peek(lex) == '\n') {
            return error_token(lex, "Unterminated string");
        }
        if (peek(lex) == '\\') {
            advance(lex);
            if (is_at_end(lex)) {
                return error_token(lex, "Unterminated string escape");
            }
            char esc = advance(lex);
            if (esc != '"' && esc != '\\' && esc != 'n' && esc != 't') {
                return error_token(lex, "Invalid string escape");
            }
            continue;
        }
        advance(lex);
    }

    if (is_at_end(lex)) {
        return error_token(lex, "Unterminated string");
    }

    advance(lex);

    size_t raw_len = (size_t)(lex->current - lex->start - 2);
    char *value = (char *)malloc(raw_len + 1);
    if (value == NULL) {
        return error_token(lex, "Out of memory");
    }

    size_t j = 0;
    for (size_t i = 0; i < raw_len; i++) {
        char c = lex->start[1 + i];
        if (c == '\\' && i + 1 < raw_len) {
            char next = lex->start[1 + i + 1];
            if (next == 'n') {
                value[j++] = '\n';
                i++;
                continue;
            }
            if (next == 't') {
                value[j++] = '\t';
                i++;
                continue;
            }
            if (next == '"' || next == '\\') {
                value[j++] = next;
                i++;
                continue;
            }
        }
        value[j++] = c;
    }
    value[j] = '\0';

    Token tok = make_token(lex, TOKEN_STRING);
    tok.lexeme = value;
    tok.line = line;
    tok.column = column;
    return tok;
}

static Token number_token(Lexer *lex) {
    while (isdigit((unsigned char)peek(lex))) {
        advance(lex);
    }

    Token tok = make_token(lex, TOKEN_INT);
    tok.lexeme = copy_lexeme(lex);
    if (tok.lexeme == NULL) {
        return error_token(lex, "Out of memory");
    }
    return tok;
}

static Token identifier_token(Lexer *lex) {
    while (isalnum((unsigned char)peek(lex)) || peek(lex) == '_') {
        advance(lex);
    }

    Token tok = make_token(lex, TOKEN_IDENT);
    tok.lexeme = copy_lexeme(lex);
    if (tok.lexeme == NULL) {
        return error_token(lex, "Out of memory");
    }
    tok.type = lookup_keyword(tok.lexeme);
    return tok;
}

static Token scan_token(Lexer *lex) {
    lex->start = lex->current;
    skip_whitespace(lex);
    lex->start = lex->current;

    if (is_at_end(lex)) {
        return make_token(lex, TOKEN_EOF);
    }

    char c = advance(lex);

    if (c == '#') {
        skip_comment(lex);
        return scan_token(lex);
    }

    if (c == '\n') {
        lex->at_line_start = 1;
        return make_token(lex, TOKEN_NEWLINE);
    }

    lex->at_line_start = 0;

    if (c == '"') {
        return string_token(lex);
    }

    if (isdigit((unsigned char)c)) {
        return number_token(lex);
    }

    if (isalpha((unsigned char)c) || c == '_') {
        return identifier_token(lex);
    }

    switch (c) {
    case '(':
        return make_token(lex, TOKEN_LPAREN);
    case ')':
        return make_token(lex, TOKEN_RPAREN);
    case '+':
        return make_token(lex, TOKEN_PLUS);
    case '*':
        return make_token(lex, TOKEN_STAR);
    case '=':
        return make_token(lex, TOKEN_ASSIGN);
    case '>':
        if (match(lex, '=')) {
            return make_token(lex, TOKEN_GTE);
        }
        return make_token(lex, TOKEN_GT);
    }

    return error_token(lex, "Unexpected character");
}

static Token emit_dedent(Lexer *lex) {
    lex->pending_dedent--;
    return make_token(lex, TOKEN_DEDENT);
}

static Token handle_indentation(Lexer *lex) {
    if (lex->pending_dedent > 0) {
        return emit_dedent(lex);
    }

    if (!lex->at_line_start) {
        return scan_token(lex);
    }

    if (is_at_end(lex)) {
        if (lex->indent_depth > 1) {
            lex->indent_depth--;
            lex->pending_dedent++;
            return emit_dedent(lex);
        }
        return make_token(lex, TOKEN_EOF);
    }

    if (peek(lex) == '\n') {
        lex->at_line_start = 1;
        advance(lex);
        Token tok = make_token(lex, TOKEN_NEWLINE);
        return tok;
    }

    if (is_blank_or_comment_line(lex)) {
        if (peek(lex) == '#') {
            skip_comment(lex);
        }
        while (peek(lex) == ' ') {
            advance(lex);
        }
        if (peek(lex) == '\n') {
            lex->at_line_start = 1;
            advance(lex);
            return make_token(lex, TOKEN_NEWLINE);
        }
        if (is_at_end(lex)) {
            if (lex->indent_depth > 1) {
                lex->indent_depth--;
                lex->pending_dedent++;
                return emit_dedent(lex);
            }
            return make_token(lex, TOKEN_EOF);
        }
    }

    int spaces = 0;
    if (!measure_indent(lex, &spaces)) {
        return error_token(lex, "Tabs are not allowed for indentation");
    }

    int current = lex->indent_stack[lex->indent_depth - 1];

    if (spaces > current) {
        if (spaces - current != INDENT_WIDTH) {
            return error_token(lex, "Indentation must increase by 4 spaces");
        }
        if (!push_indent(lex, spaces)) {
            return error_token(lex, "Out of memory");
        }
        lex->at_line_start = 0;
        Token tok = make_token(lex, TOKEN_INDENT);
        return tok;
    }

    if (spaces < current) {
        while (lex->indent_depth > 1 && spaces < lex->indent_stack[lex->indent_depth - 1]) {
            lex->indent_depth--;
            lex->pending_dedent++;
        }

        if (spaces != lex->indent_stack[lex->indent_depth - 1]) {
            return error_token(lex, "Inconsistent indentation");
        }

        lex->at_line_start = 0;
        return emit_dedent(lex);
    }

    lex->at_line_start = 0;
    return scan_token(lex);
}

void lexer_init(Lexer *lex, const char *source) {
    lex->source = source;
    lex->start = source;
    lex->current = source;
    lex->line = 1;
    lex->column = 1;
    lex->indent_depth = 1;
    lex->indent_cap = INITIAL_INDENT_CAP;
    lex->indent_stack = (int *)malloc((size_t)lex->indent_cap * sizeof(int));
    lex->indent_stack[0] = 0;
    lex->at_line_start = 1;
    lex->pending_dedent = 0;
}

Token lexer_next(Lexer *lex) {
    if (lex->pending_dedent > 0) {
        return emit_dedent(lex);
    }

    if (is_at_end(lex)) {
        if (lex->indent_depth > 1) {
            lex->indent_depth--;
            lex->pending_dedent++;
            return emit_dedent(lex);
        }
        return make_token(lex, TOKEN_EOF);
    }

    return handle_indentation(lex);
}

void lexer_free(Lexer *lex) {
    free(lex->indent_stack);
    lex->indent_stack = NULL;
    lex->indent_depth = 0;
    lex->indent_cap = 0;
}
