#pragma once

#include <cstdio>
#include <cstdlib>
#include <cstdarg>
#include <cerrno>
#include <cstring>

[[noreturn]] static void error(char const* fmt, ...) {
    int saved_errno = errno;

    va_list ap;
    va_start(ap, fmt);
    fprintf(stderr, "fatal: ");
    vfprintf(stderr, fmt, ap);
    va_end(ap);

    int ecode = EXIT_FAILURE;
    if (saved_errno) {
        ecode = -saved_errno;
        fprintf(stderr, ": %s", strerror(saved_errno));
    }
    fprintf(stderr, "\n");

    exit(ecode);
}

#define STREQ(S1, S2) (strcmp(S1, S2) == 0)
