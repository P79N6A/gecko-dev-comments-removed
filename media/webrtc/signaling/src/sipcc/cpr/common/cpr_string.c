







































#include "cpr_types.h"
#include "cpr_stdlib.h"
#include "cpr_string.h"
#include "cpr_strings.h"


















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
