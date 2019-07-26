






































#include "cpr_types.h"
#include "cpr_string.h"
#include <stdio.h>



























unsigned long
sstrncpy (char *dst, const char *src, unsigned long max)
{
    unsigned long cnt = 0;

    if (dst == NULL) {
        return 0;
    }

    if (src) {
        while ((max-- > 1) && (*src)) {
            *dst = *src;
            dst++;
            src++;
            cnt++;
        }
    }

#if defined(CPR_SSTRNCPY_PAD)
    



    while (max-- > 1) {
        *dst = '\0';
        dst++;
    }
#endif
    *dst = '\0';

    return cnt;
}

















char *
sstrncat (char *s1, const char *s2, unsigned long max)
{
    if (s1 == NULL)
        return (char *) NULL;

    while (*s1)
        s1++;

    if (s2) {
        while ((max-- > 1) && (*s2)) {
            *s1 = *s2;
            s1++;
            s2++;
        }
    }
    *s1 = '\0';

    return s1;
}

void
SafeStrCpy (char *dest, const char *src, int maxlen)
{
    int len;

    if (dest == NULL || maxlen < 0) {
        
        
        
        
        
        return;
    }

    if (src == NULL && dest != NULL) {
        dest[0] = 0;
        return;
    }

    len = strlen(src);

    if (len >= maxlen) {
        len = maxlen - 1;
    }
    memcpy(dest, src, len);
    dest[len] = 0;
}

int
strncasecmp (const char *s1, const char *s2, size_t length)
{
    return (_strnicmp(s1, s2, length));
}

int
strcasecmp (const char *s1, const char *s2)
{

    if ((!s1 && s2) || (s1 && !s2)) {
        return (int) (s1 - s2);
    }

    return (_stricmp(s1, s2));
}



char *
strdup (const char *input_str)
{
    return (_strdup(input_str));
}











char *
strcasestr (const char *s1, const char *s2)
{
    const char *cmp;
    const char *wpos;

    for (wpos = (char *) s1; *s1; wpos = ++s1) {
        cmp = s2;

        do {
            if (!*cmp) {
                return (char *) s1;
            }

            if (!*wpos) {
                return NULL;
            }

        } while (toupper(*wpos++) == toupper(*cmp++));
    }
    return NULL;
}





void
upper_string (char *str)
{
    while (*str) {
        *(str++) = toupper(*str);
    }
}
