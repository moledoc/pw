#pragma once

#ifndef MMALLOC_IMPLEMENTATIONH_
#define MMALLOC_IMPLEMENTATIONH_
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

typedef struct Alloc {
    void *ptr;
    struct Alloc *next; 
} Alloc;
Alloc *alloced_ptrs = NULL;
void mfree();
void *mmalloc(size_t size);
#endif // MMALLOC_IMPLEMENTATIONH_

#ifdef MMALLOC_IMPLEMENTATION

// MAYBE: TODO: use calloc instead of malloc

void mfree() {
    while (alloced_ptrs != NULL) {
        Alloc *me = alloced_ptrs;
        alloced_ptrs = alloced_ptrs->next;
        if (me != NULL) {
            if (me->ptr != NULL) {
                free(me->ptr);
            }
            free(me);
        }
    }
}

void *mmalloc(size_t size) {
    void *ptr = malloc(size);
    if (ptr == NULL) {
        fprintf(stderr, "[ERROR]: malloc returned NULL\n");
        mfree();
    }
    assert(ptr != NULL && "unexpected NULL from malloc");
    memset(ptr, 0, size);

    Alloc *alloced = malloc(sizeof(Alloc)*1);
    if (alloced == NULL) {
        fprintf(stderr, "[ERROR]: malloc returned NULL on Alloc\n");
        mfree();
    }
    assert(alloced != NULL && "unexpected NULL from malloc");
    alloced->ptr = ptr;
    alloced->next = alloced_ptrs;
    alloced_ptrs = alloced;
    return ptr;
}
#endif // MMALLOC_IMPLEMENTATION