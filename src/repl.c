#include "repl.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "eval.h"
#include "lexer.h"
#include "parser.h"

typedef struct {
    char *data;
    size_t len;
    size_t cap;
} InputBuffer;

static void input_buffer_init(InputBuffer *buf) {
    buf->data = NULL;
    buf->len = 0;
    buf->cap = 0;
}

static void input_buffer_free(InputBuffer *buf) {
    free(buf->data);
    buf->data = NULL;
    buf->len = 0;
    buf->cap = 0;
}

static int input_buffer_append(InputBuffer *buf, const char *line) {
    size_t line_len = strlen(line);
    size_t needed = buf->len + line_len + 2;

    if (needed > buf->cap) {
        size_t new_cap = buf->cap == 0 ? 256 : buf->cap * 2;
        while (new_cap < needed) {
            new_cap *= 2;
        }
        char *next = (char *)realloc(buf->data, new_cap);
        if (next == NULL) {
            return 0;
        }
        buf->data = next;
        buf->cap = new_cap;
    }

    if (buf->len > 0) {
        buf->data[buf->len++] = '\n';
    }
    memcpy(buf->data + buf->len, line, line_len);
    buf->len += line_len;
    buf->data[buf->len] = '\0';
    return 1;
}

static int parse_is_incomplete(const Parser *parser) {
    const char *err = parser_get_error(parser);
    return strstr(err, "Expected indented block") != NULL;
}

static int should_quit(const char *line) {
    return strcmp(line, "exit") == 0 || strcmp(line, "quit") == 0;
}

static int repl_execute(InputBuffer *input, Env *env) {
    Lexer lex;
    lexer_init(&lex, input->data);

    Parser parser;
    parser_init(&parser, &lex);
    Ast *program = parse_program(&parser);

    if (program == NULL || parser.had_error) {
        int incomplete = parser.had_error && parse_is_incomplete(&parser);
        if (!incomplete) {
            fprintf(stderr, "%s\n", parser_get_error(&parser));
        }
        token_free(&parser.current);
        lexer_free(&lex);
        return incomplete ? 1 : -1;
    }

    EvalResult result = eval_program(program, env);
    ast_free(program);
    token_free(&parser.current);
    lexer_free(&lex);

    if (!result.ok) {
        fprintf(stderr, "%s\n", result.error_message);
        eval_result_free(&result);
        return -1;
    }

    eval_result_free(&result);
    return 0;
}

int repl_run(void) {
    InputBuffer input;
    input_buffer_init(&input);

    Env *env = env_new(NULL);
    if (env == NULL) {
        fprintf(stderr, "Out of memory\n");
        return 1;
    }

    printf("Padalang REPL (exit or Ctrl-D to quit)\n");

    char line[4096];
    int collecting = 0;

    for (;;) {
        const char *prompt = collecting ? "....> " : "padalang> ";
        if (fputs(prompt, stdout) == EOF) {
            break;
        }
        fflush(stdout);

        if (fgets(line, sizeof(line), stdin) == NULL) {
            putchar('\n');
            break;
        }

        size_t len = strlen(line);
        while (len > 0 && (line[len - 1] == '\n' || line[len - 1] == '\r')) {
            line[--len] = '\0';
        }

        if (!collecting && len == 0) {
            continue;
        }

        if (!collecting && should_quit(line)) {
            break;
        }

        if (!input_buffer_append(&input, line)) {
            fprintf(stderr, "Out of memory\n");
            break;
        }

        collecting = 1;
        int status = repl_execute(&input, env);
        if (status == 1) {
            continue;
        }

        input_buffer_free(&input);
        input_buffer_init(&input);
        collecting = 0;

        if (status < 0) {
            continue;
        }
    }

    env_free(env);
    input_buffer_free(&input);
    return 0;
}
