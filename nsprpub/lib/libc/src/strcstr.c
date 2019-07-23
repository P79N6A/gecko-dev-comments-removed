




































#include "plstr.h"

PR_IMPLEMENT(char *)
PL_strcasestr(const char *big, const char *little)
{
    PRUint32 ll;

    if( ((const char *)0 == big) || ((const char *)0 == little) ) return (char *)0;
    if( ((char)0 == *big) || ((char)0 == *little) ) return (char *)0;

    ll = PL_strlen(little);

    for( ; *big; big++ )
        
            if( 0 == PL_strncasecmp(big, little, ll) )
                return (char *)big;

    return (char *)0;
}

PR_IMPLEMENT(char *)
PL_strcaserstr(const char *big, const char *little)
{
    const char *p;
    PRUint32 bl, ll;

    if( ((const char *)0 == big) || ((const char *)0 == little) ) return (char *)0;
    if( ((char)0 == *big) || ((char)0 == *little) ) return (char *)0;

    bl = PL_strlen(big);
    ll = PL_strlen(little);
    if( bl < ll ) return (char *)0;
    p = &big[ bl - ll ];

    for( ; p >= big; p-- )
        
            if( 0 == PL_strncasecmp(p, little, ll) )
                return (char *)p;

    return (char *)0;
}

PR_IMPLEMENT(char *)
PL_strncasestr(const char *big, const char *little, PRUint32 max)
{
    PRUint32 ll;

    if( ((const char *)0 == big) || ((const char *)0 == little) ) return (char *)0;
    if( ((char)0 == *big) || ((char)0 == *little) ) return (char *)0;

    ll = PL_strlen(little);
    if( ll > max ) return (char *)0;
    max -= ll;
    max++;

    for( ; max && *big; big++, max-- )
        
            if( 0 == PL_strncasecmp(big, little, ll) )
                return (char *)big;

    return (char *)0;
}

PR_IMPLEMENT(char *)
PL_strncaserstr(const char *big, const char *little, PRUint32 max)
{
    const char *p;
    PRUint32 ll;

    if( ((const char *)0 == big) || ((const char *)0 == little) ) return (char *)0;
    if( ((char)0 == *big) || ((char)0 == *little) ) return (char *)0;

    ll = PL_strlen(little);

    for( p = big; max && *p; p++, max-- )
        ;

    p -= ll;
    if( p < big ) return (char *)0;

    for( ; p >= big; p-- )
        
            if( 0 == PL_strncasecmp(p, little, ll) )
                return (char *)p;

    return (char *)0;
}
