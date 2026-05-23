#include "value.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Value val_int(long n) {
    Value v;
    v.type = VAL_INT;
    v.as.integer = n;
    return v;
}

Value val_string(char *s) {
    Value v;
    v.type = VAL_STRING;
    v.as.string = s;
    return v;
}

Value val_copy(Value v) {
    if (v.type == VAL_INT) {
        return val_int(v.as.integer);
    }
    return val_string(strdup(v.as.string));
}

void val_free(Value *v) {
    if (v == NULL) {
        return;
    }
    if (v->type == VAL_STRING) {
        free(v->as.string);
        v->as.string = NULL;
    }
}

int val_is_truthy(Value v) {
    if (v.type == VAL_INT) {
        return v.as.integer != 0;
    }
    return v.as.string != NULL && v.as.string[0] != '\0';
}

long val_as_int(Value v) {
    return v.as.integer;
}

const char *val_as_string(Value v) {
    return v.as.string;
}

void val_print(Value v) {
    if (v.type == VAL_INT) {
        printf("%ld", v.as.integer);
        return;
    }
    printf("%s", v.as.string);
}
