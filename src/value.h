#ifndef PADALANG_VALUE_H
#define PADALANG_VALUE_H

typedef enum {
    VAL_INT,
    VAL_STRING
} ValueType;

typedef struct {
    ValueType type;
    union {
        long integer;
        char *string;
    } as;
} Value;

Value val_int(long n);
Value val_string(char *s);
Value val_copy(Value v);
void val_free(Value *v);

int val_is_truthy(Value v);
long val_as_int(Value v);
const char *val_as_string(Value v);

void val_print(Value v);

#endif
