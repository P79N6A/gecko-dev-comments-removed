




































#include "plstr.h"
#include <string.h>

PR_IMPLEMENT(PRIntn)
PL_strcmp(const char *a, const char *b)
{
    if( ((const char *)0 == a) || (const char *)0 == b ) 
        return (PRIntn)(a-b);

    return (PRIntn)strcmp(a, b);
}

PR_IMPLEMENT(PRIntn)
PL_strncmp(const char *a, const char *b, PRUint32 max)
{
    if( ((const char *)0 == a) || (const char *)0 == b ) 
        return (PRIntn)(a-b);

    return (PRIntn)strncmp(a, b, (size_t)max);
}
