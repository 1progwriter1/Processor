#include <stdio.h>
#include "assert.h"
#include "hash.h"

static size_t sizehash = 8;
static unsigned long long *hash_data = NULL;

unsigned long long HashCount(const Stack *stk) {

    assert(stk);

    unsigned long long bytesum = 5381;
    size_t NUM_OF_BITES = stk->capacity * sizeof (Elem_t) + 2 * sizeof (canary_t);
    char *ptr = (char *) stk->data - sizeof (canary_t);

    for (size_t i = 0; i < NUM_OF_BITES; i++) {
        bytesum = 33 * bytesum + (size_t) *ptr++;
    }
    ptr = (char *) const_cast<Stack *>(stk) + sizeof (canary_t);
    NUM_OF_BITES = sizeof (Elem_t *) + sizeof (int);

    for (size_t i = 0; i < NUM_OF_BITES; i++) {
        bytesum = 33 * bytesum + (size_t) *ptr++;
    }

    return bytesum;
}

int HashCreate(Stack *stk) {

    if (!hash_data)
        hash_data = (unsigned long long *) calloc (sizehash, sizeof (unsigned long long));

    if (stk->id >= sizehash) {
         sizehash *= 2;
        hash_data = (unsigned long long *) realloc (hash_data, sizehash * sizeof (unsigned long long));
        if (!hash_data)
            return NO_MEMORY;
    }
    hash_data[stk->id] = HashCount(stk);
    return SUCCESS;
}

void clean() {
    free(hash_data);
}

void HashClean(size_t id) {
    hash_data[id] = 0;
}

int HashCheck(const Stack *stk) {

    assert(stk);

    return hash_data[stk->id] == HashCount(stk);
}
