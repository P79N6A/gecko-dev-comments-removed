








































#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdarg.h>

#include "sphinxbase/ckd_alloc.h"
#include "sphinxbase/strfuncs.h"


double sb_strtod(const char *s00, char **se);

double
atof_c(char const *str)
{
    return sb_strtod(str, NULL);
}


static int
isspace_c(char ch)
{
    if (ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r')
	return 1;
    return 0;
}

char *
string_join(const char *base, ...)
{
    va_list args;
    size_t len;
    const char *c;
    char *out;

    va_start(args, base);
    len = strlen(base);
    while ((c = va_arg(args, const char *)) != NULL) {
        len += strlen(c);
    }
    len++;
    va_end(args);

    out = ckd_calloc(len, 1);
    va_start(args, base);
    strcpy(out, base);
    while ((c = va_arg(args, const char *)) != NULL) {
        strcat(out, c);
    }
    va_end(args);

    return out;
}

char *
string_trim(char *string, enum string_edge_e which)
{
    size_t len;

    len = strlen(string);
    if (which == STRING_START || which == STRING_BOTH) {
        size_t sub = strspn(string, " \t\n\r\f");
        if (sub > 0) {
            memmove(string, string + sub, len + 1 - sub);
            len -= sub;
        }
    }
    if (which == STRING_END || which == STRING_BOTH) {
        long sub = len;
        while (--sub >= 0)
            if (strchr(" \t\n\r\f", string[sub]) == NULL)
                break;
        if (sub == -1)
            string[0] = '\0';
        else
            string[sub+1] = '\0';
    }
    return string;
}

int32
str2words(char *line, char **ptr, int32 max_ptr)
{
    int32 i, n;

    n = 0;                      
    i = 0;                      
    while (1) {
        
        while (line[i] && isspace_c(line[i]))
            ++i;
        if (!line[i])
            break;

        if (ptr != NULL && n >= max_ptr) {
            



            for (; i >= 0; --i)
                if (line[i] == '\0')
                    line[i] = ' ';

            return -1;
        }

        
        if (ptr != NULL)
            ptr[n] = line + i;
        ++n;
        while (line[i] && !isspace_c(line[i]))
            ++i;
        if (!line[i])
            break;
        if (ptr != NULL)
            line[i] = '\0';
        ++i;
    }

    return n;
}


int32
nextword(char *line, const char *delim, char **word, char *delimfound)
{
    const char *d;
    char *w;

    
    for (w = line; *w; w++) {
        for (d = delim; *d && (*d != *w); d++);
        if (!*d)
            break;
    }
    if (!*w)
        return -1;

    *word = w;                  

    
    for (w++; *w; w++) {
        for (d = delim; *d && (*d != *w); d++);
        if (*d)
            break;
    }

    
    *delimfound = *w;
    *w = '\0';

    return (w - *word);
}
