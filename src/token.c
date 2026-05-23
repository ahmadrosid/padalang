#include "token.h"

#include <stdlib.h>

const char *token_type_name(TokenType type) {
    switch (type) {
    case TOKEN_CALIAK: return "CALIAK";
    case TOKEN_SIMPAN: return "SIMPAN";
    case TOKEN_BILO: return "BILO";
    case TOKEN_BARU: return "BARU";
    case TOKEN_INDAK: return "INDAK";
    case TOKEN_FUNGSI: return "FUNGSI";
    case TOKEN_BULIAN: return "BULIAN";
    case TOKEN_UNTUK: return "UNTUK";
    case TOKEN_JO: return "JO";
    case TOKEN_ATAU: return "ATAU";
    case TOKEN_DARI: return "DARI";
    case TOKEN_SAMPAI: return "SAMPAI";
    case TOKEN_IDENT: return "IDENT";
    case TOKEN_INT: return "INT";
    case TOKEN_STRING: return "STRING";
    case TOKEN_ASSIGN: return "ASSIGN";
    case TOKEN_PLUS: return "PLUS";
    case TOKEN_STAR: return "STAR";
    case TOKEN_GT: return "GT";
    case TOKEN_GTE: return "GTE";
    case TOKEN_LPAREN: return "LPAREN";
    case TOKEN_RPAREN: return "RPAREN";
    case TOKEN_NEWLINE: return "NEWLINE";
    case TOKEN_INDENT: return "INDENT";
    case TOKEN_DEDENT: return "DEDENT";
    case TOKEN_EOF: return "EOF";
    case TOKEN_ERROR: return "ERROR";
    }
    return "UNKNOWN";
}

void token_free(Token *tok) {
    if (tok == NULL) {
        return;
    }
    free(tok->lexeme);
    tok->lexeme = NULL;
}
