#include <stdio.h>
#include "stack.h"
#include "assert.h"
#include "hash.h"
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include "../lib/func.h"

#ifdef logs_file
FILE *output_file = fileopen(logs_file, "w");
#else
FILE *output_file = stdout;
#endif
const int SIZESTK = 8;
const int INCREASE = 2;
const int EMPTY_POSITIONS = 3;
const int NUM_OF_ERRORS = 12;
static size_t Current_id = 0;
const Elem_t *POISON_PTR = &POISON;
const canary_t CANARY_VALUE_STACK_LEFT = 0xBADBA5EBA11;
const canary_t CANARY_VALUE_STACK_RIGHT = 0xAB0BA;
const canary_t CANARY_VALUE_DATA_LEFT = 0xC01055A1;
const canary_t CANARY_VALUE_DATA_RIGHT = 0xBADFACE;

static void PrintInfo(const Stack *stk, const char *file, const char *func, const int line);
static const char* StackStrError (int error);
static void InfoDetor(StackInfo *info);
static void StackResize(Stack *stk, const int is_increase);
static void Poison_fill(Stack *stk);
static void SetCanary(Stack *stk);

int StackCtor(Stack *stk, const char *name, const char *file, const int line, const char *func) {

    assert(stk);
    assert(name);
    assert(file);
    assert(func);

    if (!stk)
        return NULL_POINTER;

    #ifdef PROTECT
    SetCanary(stk);
    stk->canary_left = CANARY_VALUE_STACK_LEFT;
    stk->canary_right = CANARY_VALUE_STACK_RIGHT;
    #else
    stk->data = (Elem_t *) calloc (SIZESTK, sizeof (Elem_t));
    #endif

    if (!stk->data)
        return NO_MEMORY;

    stk->capacity = SIZESTK;
    stk->size = 0;
    stk->info.name = name;
    stk->info.file = file;
    stk->info.line = line;
    stk->info.func = func;
    stk->id = Current_id++;
    Poison_fill(stk);
    #ifdef PROTECT
    HashCreate(stk);
    #endif
    return SUCCESS;
}

int StackPush(Stack *stk, Elem_t n) {

    assert(stk);

    unsigned int error = StackVerify(stk);
    if (error != 0) {
        dump(stk, error)
        return ERROR;
    }
    if (stk->size >= stk->capacity) {
        StackResize(stk, RAISE);
        if (!stk->data)
            return NO_MEMORY;
    }
    stk->data[stk->size++] = n;
    #ifdef PROTECT
    HashCreate(stk);
    #endif
    return SUCCESS;
}

int StackPop(Stack *stk, Elem_t *n) {

    assert(stk);
    assert(n);

    unsigned int error = StackVerify(stk);
    if (error != 0) {
        dump(stk, error)
        return ERROR;
    }
    if (stk->size >= 1) {
        *n = stk->data[--stk->size];
        stk->data[stk->size + 1] = POISON;
        if (stk->capacity > SIZESTK && stk->capacity - stk->size > stk->capacity - stk->capacity / 2) {
            StackResize(stk, CUT);
            if (!stk->data)
                return NO_MEMORY;
            }
        #ifdef PROTECT
        HashCreate(stk);
        #endif
        return SUCCESS;
    }
    return EMPTY;
}

int StackDtor(Stack *stk) {

    assert(stk);

    #ifdef PROTECT
    free((canary_t *)stk->data - 1);
    stk->canary_left = 0;
    stk->canary_right = 0;
    HashClean(stk->id);
    #else
    free(stk->data);
    #endif
    stk->capacity = 0;
    stk->size = 0;
    InfoDetor(&stk->info);
    stk->id = 0;
    return SUCCESS;
};

void PrintStack(const Stack *stk) {

    assert(stk);

    unsigned int error = StackVerify(stk);
    if (error != 0) {
        dump(stk ,error)
    }
    else {
        if (stk->data) {
            int col = 0;
            fprintf(output_file, "capacity = %lu\n", stk->capacity);
            for (size_t i = 0; i < stk->capacity && col < 3; i++) {
                if (memcmp((char *)(stk->data + i), (const char *) POISON_PTR, sizeof (Elem_t)) == 0) {
                    col++;
                    fprintf(output_file, "[%lu] = POISON\n", i);
                }
                else {
                    fprintf(output_file, "[%lu] = " output_id "\n", i, stk->data[i]);
                }
            }
        }
        else {
            fprintf(output_file, "Unable to print Stack %s", stk->info.name);
        }
    }

}

unsigned int StackVerify(const Stack *stk) {

    assert(stk);

    unsigned int error = 0;

    unsigned int numerror = 2;

    if (stk == NULL) {
        error |= numerror;
        return error;
    }
    numerror *= 2;
    if (stk->data == NULL)
        error |= numerror;

    numerror *= 2;
    if (stk->capacity < 0)
        error |= numerror;

    numerror *= 2;
    if (stk->size < 0)
        error |= numerror;

    numerror *= 2;
    if (stk->size > stk->capacity)
        error |= numerror;

    #ifdef PROTECT
    numerror *= 2;
    if (stk->canary_left != CANARY_VALUE_STACK_LEFT) {
        error |= numerror;
    }

    numerror *= 2;
    if (stk->canary_right != CANARY_VALUE_STACK_RIGHT)
        error |= numerror;


    numerror *= 2;
    if (stk->data != NULL) {
        if (((canary_t *)(stk->data))[-1] != CANARY_VALUE_DATA_LEFT)
            error |= numerror;
    }

    numerror *= 2;
    if (stk->data != NULL) {
        if (*((canary_t *)(stk->data + stk->capacity)) != CANARY_VALUE_DATA_RIGHT)
            error |= numerror;
    }

    numerror *= 2;
    if (!HashCheck(stk))
        error |= numerror;
    #endif

    numerror *= 2;
    if (stk->id < 0)
        error |= numerror;

    return error;
}

static const char* StackStrError (int error) {

    #define ERR_(code)  case code: return #code;

    switch (error)
        {
        ERR_ (SUCCESS)
        ERR_ (NULL_POINTER)
        ERR_ (NO_MEMORY)
        ERR_ (INCORRECT_CAPACITY)
        ERR_ (INCORRECT_SIZE)
        ERR_ (STACK_OVERFLOW)
        ERR_ (CANARY_FAULT_STACK_LEFT)
        ERR_ (CANARY_FAULT_STACK_RIGHT)
        ERR_ (CANARY_FAULT_DATA_LEFT)
        ERR_ (CANARY_FAULT_DATA_RIGHT)
        ERR_ (HASH_ERROR)
        ERR_ (ID_ERROR)
        ERR_ (ERROR)
        ERR_ (EMPTY)

        default: return "???";
        }

    #undef ERR_
    }

void StackDump(unsigned int error, const char *file, const int line, const char *func, const Stack *stk) {

    if (!file)
        fprintf(output_file, "NULL");
    if (!func)
        fprintf(output_file, "NULL");

    if (stk) {
        if (error != 0) {
            fprintf(output_file, "Error codes: ");

            const int NUMBER = 1;

            int err = 0;
            for (int i = 2; i < NUM_OF_ERRORS + 1; i++) {
                if ((error | NUMBER) == error) {
                    fprintf(output_file, "%s%d (%s)", (err? ", " : ""), i, StackStrError ((enum Result) i));
                    err = 1;
                }
            error >>= 1;
            }
            fprintf(output_file, "\n");

            PrintInfo(stk, file, func, line);
        }
    }
    else {
        fprintf(output_file, "Unable to dump stack");
    }
}

static void PrintInfo(const Stack *stk, const char *file, const char *func, const int line) {

    assert(stk);
    assert(file);
    assert(func);

    int col = 0;
    fprintf(output_file, "Stack \"%s\" [%p] from \"%s\" (%d)\ncalled from file: \"%s\", function: \"%s\", line: (%d)\n", stk->info.name, stk, stk->info.file, stk->info.line, file, func, line);
        fprintf(output_file, "{size = %lu\n capacity = %lu\n data [%p]\n", stk->size, stk->capacity, stk->data);

    if (stk->capacity > 0 && stk->data) {
        fprintf(output_file, "\t{\n");
        for (size_t i = 0; i < stk->capacity && col < EMPTY_POSITIONS; i++) {
            if (memcmp((char *)(stk->data + i), (const char *) POISON_PTR, sizeof (Elem_t)) == 0) {
                col++;
                fprintf(output_file, "\t [%lu] = POISON\n", i);
                continue;
            }
            fprintf(output_file, "\t*[%lu] = " output_id "\n", i, stk->data[i]);
        }
        fprintf(output_file, "\t}\n");
    }
    fprintf(output_file, "}\n");
}

static void StackResize(Stack *stk, const int is_increase) {

    assert(stk);

    if (is_increase)
        stk->capacity *= INCREASE;
    else
        stk->capacity /= INCREASE;

    #ifdef PROTECT
    stk->data = (Elem_t *) realloc ((canary_t *)stk->data - 1, sizeof (Elem_t) * stk->capacity + 2 * sizeof (canary_t));
    stk->data = (Elem_t *) ((canary_t *) stk->data + 1);
    *((canary_t *)(stk->data + stk->capacity)) = CANARY_VALUE_DATA_RIGHT;
    #else
    stk->data = (Elem_t *) realloc (stk->data, sizeof (Elem_t) * stk->capacity);
    #endif
    Poison_fill(stk);
}

void Detor() {
    clean();
}

static void Poison_fill(Stack *stk) {

    assert(stk);

    for (size_t i = stk->size; i < stk->capacity; i++)
        stk->data[i] = POISON;
}

static void SetCanary(Stack *stk) {

    assert(stk);
    int length = sizeof (Elem_t) * SIZESTK + sizeof (canary_t) * 2;
    stk->data = (Elem_t *) calloc ((size_t) length, sizeof (char));

    if (stk->data) {
        ((canary_t *) stk->data)[0] = CANARY_VALUE_DATA_LEFT;
        stk->data = (Elem_t *) ((canary_t *) stk->data + 1);
        *((canary_t *)(stk->data + SIZESTK)) = CANARY_VALUE_DATA_RIGHT;
    }
}

static void InfoDetor(StackInfo *info) {

    assert(info);

    info->file = NULL;
    info->func = NULL;
    info->line = -1;
    info->name = NULL;
}
