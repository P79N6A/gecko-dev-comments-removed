




































#include "plstr.h"
#include <string.h>

PR_IMPLEMENT(char *)
PL_strcat(char *dest, const char *src)
{
    if( ((char *)0 == dest) || ((const char *)0 == src) )
        return dest;

    return strcat(dest, src);
}

PR_IMPLEMENT(char *)
PL_strncat(char *dest, const char *src, PRUint32 max)
{
    char *rv;

    if( ((char *)0 == dest) || ((const char *)0 == src) || (0 == max) )
        return dest;

    for( rv = dest; *dest; dest++ )
        ;

    (void)PL_strncpy(dest, src, max);
    return rv;
}

PR_IMPLEMENT(char *)
PL_strcatn(char *dest, PRUint32 max, const char *src)
{
    char *rv;
    PRUint32 dl;

    if( ((char *)0 == dest) || ((const char *)0 == src) )
        return dest;

    for( rv = dest, dl = 0; *dest; dest++, dl++ )
        ;

    if( max <= dl ) return rv;
    (void)PL_strncpyz(dest, src, max-dl);

    return rv;
}
