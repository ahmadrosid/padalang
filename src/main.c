#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ast.h"
#include "env.h"
#include "eval.h"
#include "lexer.h"
#include "parser.h"
#include "repl.h"
#include "token.h"

static char *read_file(const char *path, size_t *size_out) {
    FILE *file = fopen(path, "rb");
    if (file == NULL) {
        return NULL;
    }

    if (fseek(file, 0, SEEK_END) != 0) {
        fclose(file);
        return NULL;
    }

    long size = ftell(file);
    if (size < 0) {
        fclose(file);
        return NULL;
    }

    if (fseek(file, 0, SEEK_SET) != 0) {
        fclose(file);
        return NULL;
    }

    char *buffer = (char *)malloc((size_t)size + 1);
    if (buffer == NULL) {
        fclose(file);
        return NULL;
    }

    size_t read = fread(buffer, 1, (size_t)size, file);
    fclose(file);

    if (read != (size_t)size) {
        free(buffer);
        return NULL;
    }

    buffer[size] = '\0';
    *size_out = (size_t)size;
    return buffer;
}

static void print_token(Token tok) {
    printf("%s", token_type_name(tok.type));

    if (tok.type == TOKEN_IDENT || tok.type == TOKEN_INT) {
        printf("(%s)", tok.lexeme);
    } else if (tok.type == TOKEN_STRING) {
        printf("(\"%s\")", tok.lexeme);
    } else if (tok.type == TOKEN_ERROR) {
        printf("(%s)", tok.lexeme != NULL ? tok.lexeme : "unknown error");
    }

    printf(" @%d:%d\n", tok.line, tok.column);
}

static int run_lexer(const char *path) {
    size_t size = 0;
    char *source = read_file(path, &size);
    if (source == NULL) {
        fprintf(stderr, "Could not read file: %s\n", path);
        return 1;
    }

    Lexer lex;
    lexer_init(&lex, source);

    for (;;) {
        Token tok = lexer_next(&lex);
        print_token(tok);

        int done = tok.type == TOKEN_EOF || tok.type == TOKEN_ERROR;
        token_free(&tok);

        if (done) {
            break;
        }
    }

    lexer_free(&lex);
    free(source);
    return 0;
}

static int run_program(const char *path) {
    size_t size = 0;
    char *source = read_file(path, &size);
    if (source == NULL) {
        fprintf(stderr, "Could not read file: %s\n", path);
        return 1;
    }

    Lexer lex;
    lexer_init(&lex, source);

    Parser parser;
    parser_init(&parser, &lex);
    Ast *program = parse_program(&parser);

    if (program == NULL || parser.had_error) {
        fprintf(stderr, "%s\n", parser_get_error(&parser));
        token_free(&parser.current);
        lexer_free(&lex);
        free(source);
        return 1;
    }

    Env *env = env_new(NULL);
    if (env == NULL) {
        fprintf(stderr, "Out of memory\n");
        ast_free(program);
        token_free(&parser.current);
        lexer_free(&lex);
        free(source);
        return 1;
    }

    EvalResult result = eval_program(program, env);
    ast_free(program);
    token_free(&parser.current);
    lexer_free(&lex);
    free(source);

    if (!result.ok) {
        fprintf(stderr, "%s\n", result.error_message);
        eval_result_free(&result);
        env_free(env);
        return 1;
    }

    eval_result_free(&result);
    env_free(env);
    return 0;
}

static int run_parser(const char *path) {
    size_t size = 0;
    char *source = read_file(path, &size);
    if (source == NULL) {
        fprintf(stderr, "Could not read file: %s\n", path);
        return 1;
    }

    Lexer lex;
    lexer_init(&lex, source);

    Parser parser;
    parser_init(&parser, &lex);
    Ast *program = parse_program(&parser);

    if (program == NULL || parser.had_error) {
        fprintf(stderr, "%s\n", parser_get_error(&parser));
        token_free(&parser.current);
        lexer_free(&lex);
        free(source);
        return 1;
    }

    ast_tree_print(program, 0);
    ast_free(program);
    token_free(&parser.current);
    lexer_free(&lex);
    free(source);
    return 0;
}

static void print_usage(const char *prog) {
    fprintf(stderr, "Usage: %s [--repl | --parse | --tokens] [file.pad]\n", prog);
    fprintf(stderr, "  (no args)  Start interactive REPL\n");
    fprintf(stderr, "  --repl     Start interactive REPL\n");
    fprintf(stderr, "  file.pad   Run program\n");
    fprintf(stderr, "  --parse    Print AST for file\n");
    fprintf(stderr, "  --tokens   Print tokens for file\n");
}

int main(int argc, char **argv) {
    int parse_mode = 0;
    int tokens_mode = 0;
    int repl_mode = 0;
    const char *path = NULL;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--parse") == 0) {
            parse_mode = 1;
        } else if (strcmp(argv[i], "--tokens") == 0) {
            tokens_mode = 1;
        } else if (strcmp(argv[i], "--repl") == 0) {
            repl_mode = 1;
        } else if (path == NULL) {
            path = argv[i];
        } else {
            print_usage(argv[0]);
            return 1;
        }
    }

    if (repl_mode || (path == NULL && !parse_mode && !tokens_mode)) {
        return repl_run();
    }

    if (path == NULL) {
        print_usage(argv[0]);
        return 1;
    }

    if (parse_mode) {
        return run_parser(path);
    }

    if (tokens_mode) {
        return run_lexer(path);
    }

    return run_program(path);
}
