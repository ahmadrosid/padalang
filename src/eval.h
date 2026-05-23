#ifndef PADALANG_EVAL_H
#define PADALANG_EVAL_H

#include "ast.h"
#include "env.h"
#include "value.h"

typedef struct {
    int ok;
    int returned;
    char error_message[256];
    Value value;
} EvalResult;

EvalResult eval_program(Ast *program, Env *env);
void eval_result_free(EvalResult *result);

#endif
