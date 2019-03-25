#pragma once

#include <stdlib.h> /* malloc(3) */
#include <string.h> /* strcmp(3) */

#define STREQ(S1, S2) ((_Bool)(strcmp(S1, S2) == 0))

static void* emalloc(size_t siz) {
    if (siz < 1) {
        fprintf(stderr, "refusing to create nullptr\n");
        exit(1);
    }
    void* p = malloc(siz);
    if (!p) {
        fprintf(stderr, "out of memory!\n");
        exit(1);
    }
    return p;
}
