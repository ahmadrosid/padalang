#include "env.h"

#include <stdlib.h>
#include <string.h>

#include "ast.h"

Env *env_new(Env *parent) {
    Env *env = (Env *)calloc(1, sizeof(Env));
    if (env == NULL) {
        return NULL;
    }
    env->parent = parent;
    return env;
}

static Binding *find_binding(Env *env, const char *name) {
    for (Binding *b = env->bindings; b != NULL; b = b->next) {
        if (strcmp(b->name, name) == 0) {
            return b;
        }
    }
    if (env->parent != NULL) {
        return find_binding(env->parent, name);
    }
    return NULL;
}

static Binding *find_local_binding(Env *env, const char *name) {
    for (Binding *b = env->bindings; b != NULL; b = b->next) {
        if (strcmp(b->name, name) == 0) {
            return b;
        }
    }
    return NULL;
}

void env_define_var(Env *env, const char *name, Value value) {
    Binding *b = (Binding *)calloc(1, sizeof(Binding));
    if (b == NULL) {
        val_free(&value);
        return;
    }
    b->name = strdup(name);
    b->is_func = 0;
    b->value = value;
    b->next = env->bindings;
    env->bindings = b;
}

void env_set_var(Env *env, const char *name, Value value) {
    Binding *b = find_local_binding(env, name);
    if (b != NULL && !b->is_func) {
        val_free(&b->value);
        b->value = value;
        return;
    }

    if (env->parent != NULL) {
        env_set_var(env->parent, name, value);
        val_free(&value);
        return;
    }

    env_define_var(env, name, value);
}

int env_get_var(Env *env, const char *name, Value *out) {
    Binding *b = find_binding(env, name);
    if (b == NULL || b->is_func) {
        return 0;
    }
    *out = val_copy(b->value);
    return 1;
}

FuncDef *func_def_new(char *name, char **params, int param_count, Ast *body, Env *closure) {
    FuncDef *func = (FuncDef *)calloc(1, sizeof(FuncDef));
    if (func == NULL) {
        return NULL;
    }
    func->name = name;
    func->params = params;
    func->param_count = param_count;
    func->body = body;
    func->closure = closure;
    return func;
}

void env_define_func(Env *env, FuncDef *func) {
    Binding *b = (Binding *)calloc(1, sizeof(Binding));
    if (b == NULL) {
        return;
    }
    b->name = strdup(func->name);
    b->is_func = 1;
    b->func = func;
    b->next = env->bindings;
    env->bindings = b;
}

FuncDef *env_get_func(Env *env, const char *name) {
    Binding *b = find_binding(env, name);
    if (b == NULL || !b->is_func) {
        return NULL;
    }
    return b->func;
}

static void binding_free(Binding *b) {
    if (b == NULL) {
        return;
    }
    free(b->name);
    if (!b->is_func) {
        val_free(&b->value);
    }
    free(b);
}

void func_def_free(FuncDef *func) {
    if (func == NULL) {
        return;
    }
    free(func->name);
    for (int i = 0; i < func->param_count; i++) {
        free(func->params[i]);
    }
    free(func->params);
    ast_free(func->body);
    free(func);
}

void env_free(Env *env) {
    if (env == NULL) {
        return;
    }

    Binding *b = env->bindings;
    while (b != NULL) {
        Binding *next = b->next;
        if (b->is_func) {
            func_def_free(b->func);
        }
        binding_free(b);
        b = next;
    }

    free(env);
}
