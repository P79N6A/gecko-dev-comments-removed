






























































#include <stdlib.h>
#include <assert.h>

#include "sphinxbase/case.h"
#include "sphinxbase/err.h"


void
lcase(register char *cp)
{
    if (cp) {
        while (*cp) {
            *cp = LOWER_CASE(*cp);
            cp++;
        }
    }
}

void
ucase(register char *cp)
{
    if (cp) {
        while (*cp) {
            *cp = UPPER_CASE(*cp);
            cp++;
        }
    }
}

int32
strcmp_nocase(const char *str1, const char *str2)
{
    char c1, c2;

    if (str1 == str2)
        return 0;
    if (str1 && str2) {
        for (;;) {
            c1 = *(str1++);
            c1 = UPPER_CASE(c1);
            c2 = *(str2++);
            c2 = UPPER_CASE(c2);
            if (c1 != c2)
                return (c1 - c2);
            if (c1 == '\0')
                return 0;
        }
    }
    else
        return (str1 == NULL) ? -1 : 1;

    return 0;
}

int32
strncmp_nocase(const char *str1, const char *str2, size_t len)
{
    char c1, c2;

    if (str1 && str2) {
        size_t n;

        for (n = 0; n < len; ++n) {
            c1 = *(str1++);
            c1 = UPPER_CASE(c1);
            c2 = *(str2++);
            c2 = UPPER_CASE(c2);
            if (c1 != c2)
                return (c1 - c2);
            if (c1 == '\0')
                return 0;
        }
    }
    else
        return (str1 == NULL) ? -1 : 1;

    return 0;
}
