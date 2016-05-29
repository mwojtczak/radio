#include "err.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include "err.h"

#define EXIT_FAILURE 1

void syserr(const char *fmt, ...) {
    va_list fmt_args;

    fprintf(stderr, "ERROR: ");
    va_start(fmt_args, fmt);
    vfprintf(stderr, fmt, fmt_args);
    va_end(fmt_args);
    fprintf(stderr, " (%d; %s)\n", errno, strerror(errno));
    exit(EXIT_FAILURE);
}

void fatal(const char *fmt, ...) {
    va_list fmt_args;

    fprintf(stderr, "ERROR: ");
    va_start(fmt_args, fmt);
    vfprintf(stderr, fmt, fmt_args);
    va_end(fmt_args);
    fprintf(stderr, "\n");
    exit(EXIT_FAILURE);
}