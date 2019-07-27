












#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "misc.h"
#include "os.h" 

extern const char *progname;




void *
memalloc(size_t size)
{
    void *ptr = malloc(size);
    if (!ptr)
        errorexit("out of memory");
    
    memset(ptr, 0, size);
    return ptr;
}

void *
memrealloc(void *ptr, size_t size)
{
    void *newptr = realloc(ptr, size);
    if (!newptr)
        errorexit("out of memory");
    return newptr;
}

void
memfree(void *ptr)
{
    *(int *)ptr = 0xfefefefe;
    free(ptr);
}




char *
memprintf(const char *format, ...)
{
    va_list ap;
    char *buf;
    va_start(ap, format);
    buf = vmemprintf(format, ap);
    va_end(ap);
    return buf;
}

char *
vmemprintf(const char *format, va_list ap)
{
    char *buf;
    unsigned int max, len;
    va_list ap2;
    max = 16;
    for (;;) {
        va_copy(ap2, ap);
        buf = memalloc(max);
        len = vsnprintf(buf, max, format, ap2);
        va_end(ap2);
        if (len < max)
            break;
        memfree(buf);
        max *= 2;
    }
    return buf;
}




void
vlocerrorexit(const char *filename, unsigned int linenum,
        const char *format, va_list ap)
{
    if (filename)
        fprintf(stderr, linenum ? "%s: %i: " : "%s: ", filename, linenum);
    vfprintf(stderr, format, ap);
    fputc('\n', stderr);
    exit(1);
}

void
locerrorexit(const char *filename, unsigned int linenum,
        const char *format, ...)
{
    va_list ap;
    va_start(ap, format);
    vlocerrorexit(filename, linenum, format, ap);
    va_end(ap);
}

void
errorexit(const char *format, ...)
{
    va_list ap;
    va_start(ap, format);
    vlocerrorexit(0, 0, format, ap);
    va_end(ap);
}

