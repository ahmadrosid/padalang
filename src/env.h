#ifndef PADALANG_ENV_H
#define PADALANG_ENV_H

#include "ast.h"
#include "value.h"

typedef struct FuncDef FuncDef;

typedef struct Env {
    struct Binding *bindings;
    struct Env *parent;
} Env;

struct FuncDef {
    char *name;
    char **params;
    int param_count;
    Ast *body;
    Env *closure;
};

typedef struct Binding {
    char *name;
    int is_func;
    Value value;
    FuncDef *func;
    struct Binding *next;
} Binding;

Env *env_new(Env *parent);
void env_free(Env *env);

void env_define_var(Env *env, const char *name, Value value);
void env_set_var(Env *env, const char *name, Value value);
int env_get_var(Env *env, const char *name, Value *out);

void env_define_func(Env *env, FuncDef *func);
FuncDef *env_get_func(Env *env, const char *name);

FuncDef *func_def_new(char *name, char **params, int param_count, Ast *body, Env *closure);
void func_def_free(FuncDef *func);

#endif
