




































#include "primpl.h"








#if !(defined(AIX) && !defined(DEBUG))
#include <string.h>
#endif

PRSize _pr_CopyLowBits( 
    void *dst, 
    PRSize dstlen, 
    void *src, 
    PRSize srclen )
{
    if (srclen <= dstlen) {
    	memcpy(dst, src, srclen);
	    return srclen;
    }
#if defined IS_BIG_ENDIAN
    memcpy(dst, (char*)src + (srclen - dstlen), dstlen);
#else
    memcpy(dst, src, dstlen);
#endif
    return dstlen;
}    

PR_IMPLEMENT(PRSize) PR_GetRandomNoise( 
    void    *buf,
    PRSize  size
)
{
    return( _PR_MD_GET_RANDOM_NOISE( buf, size ));
} 

