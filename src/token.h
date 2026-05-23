#ifndef PADALANG_TOKEN_H
#define PADALANG_TOKEN_H

typedef enum {
    TOKEN_CALIAK,
    TOKEN_SIMPAN,
    TOKEN_BILO,
    TOKEN_BARU,
    TOKEN_INDAK,
    TOKEN_FUNGSI,
    TOKEN_BULIAN,
    TOKEN_UNTUK,
    TOKEN_JO,
    TOKEN_ATAU,
    TOKEN_DARI,
    TOKEN_SAMPAI,

    TOKEN_IDENT,
    TOKEN_INT,
    TOKEN_STRING,

    TOKEN_ASSIGN,
    TOKEN_PLUS,
    TOKEN_STAR,
    TOKEN_GT,
    TOKEN_GTE,
    TOKEN_LPAREN,
    TOKEN_RPAREN,

    TOKEN_NEWLINE,
    TOKEN_INDENT,
    TOKEN_DEDENT,

    TOKEN_EOF,
    TOKEN_ERROR
} TokenType;

typedef struct {
    TokenType type;
    char *lexeme;
    int line;
    int column;
} Token;

const char *token_type_name(TokenType type);
void token_free(Token *tok);

#endif
