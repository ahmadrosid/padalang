#include "eval.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static EvalResult eval_ok(Value value) {
    EvalResult result;
    result.ok = 1;
    result.returned = 0;
    result.error_message[0] = '\0';
    result.value = value;
    return result;
}

static EvalResult eval_error(const char *message) {
    EvalResult result;
    result.ok = 0;
    result.returned = 0;
    result.error_message[0] = '\0';
    result.value = val_int(0);
    snprintf(result.error_message, sizeof(result.error_message), "%s", message);
    return result;
}

static EvalResult eval_error_at(Ast *node, const char *message) {
    EvalResult result = eval_error(message);
    if (node != NULL) {
        snprintf(result.error_message, sizeof(result.error_message),
                 "Runtime error at %d:%d: %s", node->line, node->column, message);
    }
    return result;
}

void eval_result_free(EvalResult *result) {
    if (result == NULL) {
        return;
    }
    val_free(&result->value);
}

static EvalResult eval_expr(Ast *node, Env *env);
static EvalResult eval_stmt(Ast *node, Env *env);

static char *int_to_string(long n) {
    char buf[32];
    snprintf(buf, sizeof(buf), "%ld", n);
    return strdup(buf);
}

static Value eval_concat(Value left, Value right) {
    const char *left_str = left.type == VAL_STRING ? left.as.string : NULL;
    const char *right_str = right.type == VAL_STRING ? right.as.string : NULL;
    char *owned_left = NULL;
    char *owned_right = NULL;

    if (left.type == VAL_INT) {
        owned_left = int_to_string(left.as.integer);
        left_str = owned_left;
    }
    if (right.type == VAL_INT) {
        owned_right = int_to_string(right.as.integer);
        right_str = owned_right;
    }

    size_t len = strlen(left_str) + strlen(right_str) + 1;
    char *out = (char *)malloc(len);
    if (out != NULL) {
        snprintf(out, len, "%s%s", left_str, right_str);
    }

    free(owned_left);
    free(owned_right);
    return val_string(out);
}

static EvalResult eval_binary(Ast *node, Env *env) {
    EvalResult left_result = eval_expr(node->as.binary.left, env);
    if (!left_result.ok) {
        return left_result;
    }

    EvalResult right_result = eval_expr(node->as.binary.right, env);
    if (!right_result.ok) {
        eval_result_free(&left_result);
        return right_result;
    }

    Value left = left_result.value;
    Value right = right_result.value;
    EvalResult result;

    switch (node->as.binary.op) {
    case BINOP_PLUS:
        if (left.type == VAL_STRING || right.type == VAL_STRING) {
            result = eval_ok(eval_concat(left, right));
        } else {
            result = eval_ok(val_int(val_as_int(left) + val_as_int(right)));
        }
        break;
    case BINOP_STAR:
        if (left.type != VAL_INT || right.type != VAL_INT) {
            result = eval_error_at(node, "Multiplication requires numbers");
        } else {
            result = eval_ok(val_int(val_as_int(left) * val_as_int(right)));
        }
        break;
    case BINOP_GT:
        if (left.type != VAL_INT || right.type != VAL_INT) {
            result = eval_error_at(node, "Comparison requires numbers");
        } else {
            result = eval_ok(val_int(val_as_int(left) > val_as_int(right)));
        }
        break;
    case BINOP_GTE:
        if (left.type != VAL_INT || right.type != VAL_INT) {
            result = eval_error_at(node, "Comparison requires numbers");
        } else {
            result = eval_ok(val_int(val_as_int(left) >= val_as_int(right)));
        }
        break;
    default:
        result = eval_error_at(node, "Unknown operator");
        break;
    }

    val_free(&left);
    val_free(&right);
    return result;
}

static EvalResult eval_call(Ast *node, Env *env) {
    FuncDef *func = env_get_func(env, node->as.call.name);
    if (func == NULL) {
        return eval_error_at(node, "Undefined function");
    }

    if (node->as.call.arg_count != func->param_count) {
        return eval_error_at(node, "Wrong number of arguments");
    }

    Env *call_env = env_new(func->closure);
    if (call_env == NULL) {
        return eval_error("Out of memory");
    }

    for (int i = 0; i < func->param_count; i++) {
        EvalResult arg_result = eval_expr(node->as.call.args[i], env);
        if (!arg_result.ok) {
            env_free(call_env);
            return arg_result;
        }
        env_define_var(call_env, func->params[i], arg_result.value);
    }

    EvalResult result = eval_stmt(func->body, call_env);
    env_free(call_env);

    if (!result.ok) {
        return result;
    }

    if (!result.returned) {
        eval_result_free(&result);
        return eval_ok(val_int(0));
    }

    result.returned = 0;
    return result;
}

static EvalResult eval_expr(Ast *node, Env *env) {
    if (node == NULL) {
        return eval_error("Missing expression");
    }

    switch (node->type) {
    case AST_INT:
        return eval_ok(val_int(node->as.integer.value));
    case AST_STRING:
        return eval_ok(val_string(strdup(node->as.string.value)));
    case AST_IDENT: {
        Value value;
        if (!env_get_var(env, node->as.ident.name, &value)) {
            return eval_error_at(node, "Undefined variable");
        }
        return eval_ok(value);
    }
    case AST_BINARY:
        return eval_binary(node, env);
    case AST_CALL:
        return eval_call(node, env);
    default:
        return eval_error_at(node, "Invalid expression");
    }
}

static EvalResult eval_block(Ast *node, Env *env) {
    for (int i = 0; i < node->as.block.count; i++) {
        EvalResult result = eval_stmt(node->as.block.stmts[i], env);
        if (!result.ok || result.returned) {
            return result;
        }
        eval_result_free(&result);
    }
    return eval_ok(val_int(0));
}

static EvalResult eval_stmt(Ast *node, Env *env) {
    if (node == NULL) {
        return eval_error("Missing statement");
    }

    switch (node->type) {
    case AST_PRINT: {
        EvalResult result = eval_expr(node->as.print.expr, env);
        if (!result.ok) {
            return result;
        }
        val_print(result.value);
        printf("\n");
        eval_result_free(&result);
        return eval_ok(val_int(0));
    }
    case AST_ASSIGN: {
        EvalResult result = eval_expr(node->as.assign.value, env);
        if (!result.ok) {
            return result;
        }
        env_set_var(env, node->as.assign.name, result.value);
        return eval_ok(val_int(0));
    }
    case AST_IF: {
        EvalResult cond = eval_expr(node->as.if_stmt.cond, env);
        if (!cond.ok) {
            return cond;
        }
        int truthy = val_is_truthy(cond.value);
        val_free(&cond.value);
        if (truthy) {
            return eval_stmt(node->as.if_stmt.then_branch, env);
        }
        if (node->as.if_stmt.else_branch != NULL) {
            return eval_stmt(node->as.if_stmt.else_branch, env);
        }
        return eval_ok(val_int(0));
    }
    case AST_FUNC: {
        FuncDef *func = func_def_new(
            node->as.func.name,
            node->as.func.params,
            node->as.func.param_count,
            node->as.func.body,
            env);
        if (func == NULL) {
            return eval_error("Out of memory");
        }
        node->as.func.name = NULL;
        node->as.func.params = NULL;
        node->as.func.body = NULL;
        env_define_func(env, func);
        return eval_ok(val_int(0));
    }
    case AST_RETURN: {
        EvalResult result = eval_expr(node->as.ret.expr, env);
        if (!result.ok) {
            return result;
        }
        result.returned = 1;
        return result;
    }
    case AST_FOR: {
        EvalResult from_result = eval_expr(node->as.for_stmt.from, env);
        if (!from_result.ok) {
            return from_result;
        }
        EvalResult to_result = eval_expr(node->as.for_stmt.to, env);
        if (!to_result.ok) {
            eval_result_free(&from_result);
            return to_result;
        }

        if (from_result.value.type != VAL_INT || to_result.value.type != VAL_INT) {
            eval_result_free(&from_result);
            eval_result_free(&to_result);
            return eval_error_at(node, "Loop range requires numbers");
        }

        long from = val_as_int(from_result.value);
        long to = val_as_int(to_result.value);
        val_free(&from_result.value);
        val_free(&to_result.value);

        for (long i = from; i <= to; i++) {
            env_set_var(env, node->as.for_stmt.var, val_int(i));
            EvalResult result = eval_stmt(node->as.for_stmt.body, env);
            if (!result.ok || result.returned) {
                return result;
            }
            eval_result_free(&result);
        }

        return eval_ok(val_int(0));
    }
    case AST_BLOCK:
        return eval_block(node, env);
    default:
        return eval_error_at(node, "Invalid statement");
    }
}

EvalResult eval_program(Ast *program, Env *env) {
    if (program == NULL || program->type != AST_PROGRAM) {
        return eval_error("Expected program");
    }

    for (int i = 0; i < program->as.program.count; i++) {
        EvalResult result = eval_stmt(program->as.program.stmts[i], env);
        if (!result.ok) {
            return result;
        }
        if (result.returned) {
            return eval_error_at(program->as.program.stmts[i], "Cannot return outside function");
        }
        eval_result_free(&result);
    }

    return eval_ok(val_int(0));
}
