


















































































static char *RCSSTRING __UNUSED__ ="$Id: r_errors.c,v 1.5 2008/11/26 03:22:02 adamcain Exp $";

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "r_common.h"
#include "r_errors.h"

static struct {
    int    errnum;
    char  *str;
} errors[] = NR_ERROR_MAPPING;

int nr_verr_exit(char *fmt,...)
  {
    va_list ap;

    va_start(ap,fmt);
    vfprintf(stderr,fmt,ap);

    if (fmt[0] != '\0' && fmt[strlen(fmt)-1] != '\n')
        fprintf(stderr,"\n");

    exit(1);
  }

char *
nr_strerror(int errnum)
{
    static char unknown_error[256];
    int i;
    char *error = 0;

    for (i = 0; i < sizeof(errors)/sizeof(*errors); ++i) {
        if (errnum == errors[i].errnum) {
            error = errors[i].str;
            break;
        }
    }

    if (! error) {
        snprintf(unknown_error, sizeof(unknown_error), "Unknown error: %d", errnum);
        error = unknown_error;
    }

    return error;
}

int
nr_strerror_r(int errnum, char *strerrbuf, size_t buflen)
{
    char *error = nr_strerror(errnum);
    snprintf(strerrbuf, buflen, "%s", error);
    return 0;
}

