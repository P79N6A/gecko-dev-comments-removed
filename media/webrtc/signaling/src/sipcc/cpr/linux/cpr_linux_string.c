






































#include "cpr_types.h"
#include "cpr_stdlib.h"
#include "cpr_string.h"
#include "cpr_strings.h"
















char *
cpr_strdup (const char *str)
{
    char *dup;
    size_t len;

    if (!str) {
        return (char *) NULL;
    }

    len = strlen(str);
    if (len == 0) {
        return (char *) NULL;
    }
    len++;

    dup = cpr_malloc(len * sizeof(char));
    if (!dup) {
        return (char *) NULL;
    }
    (void) memcpy(dup, str, len);
    return dup;
}















#ifndef CPR_USE_OS_STRCASECMP
int
cpr_strcasecmp (const char *s1, const char *s2)
{
    const unsigned char *us1 = (const unsigned char *) s1;
    const unsigned char *us2 = (const unsigned char *) s2;

    
    if ((!s1 && s2) || (s1 && !s2)) {
        



        return (int) (s1 - s2);
    }

    
    if (s1 == s2)
        return 0;

    while (*us1 != '\0' && *us2 != '\0' && toupper(*us1) == toupper(*us2)) {
        us1++;
        us2++;
    }

    return (toupper(*us1) - toupper(*us2));
}















int
cpr_strncasecmp (const char *s1, const char *s2, size_t len)
{
    const unsigned char *us1 = (const unsigned char *) s1;
    const unsigned char *us2 = (const unsigned char *) s2;

    
    if ((!s1 && s2) || (s1 && !s2))
        return ((int) (s1 - s2));

    if ((len == 0) || (s1 == s2))
        return 0;

    while (len-- > 0 && toupper(*us1) == toupper(*us2)) {
        if (len == 0 || *us1 == '\0' || *us2 == '\0')
            break;
        us1++;
        us2++;
    }

    return (toupper(*us1) - toupper(*us2));
}
#endif


















char *
strcasestr (const char *s1, const char *s2)
{
    unsigned int i;

    if (!s1)
        return (char *) NULL;

    if (!s2 || (s1 == s2) || (*s2 == '\0'))
        return (char *) s1;

    while (*s1) {
        i = 0;
        do {
            if (s2[i] == '\0')
                return (char *) s1;
            if (s1[i] == '\0')
                return (char *) NULL;
            if (toupper(s1[i]) != toupper(s2[i]))
                break;
            i++;
        } while (1);
        s1++;
    }

    return (char *) NULL;
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




