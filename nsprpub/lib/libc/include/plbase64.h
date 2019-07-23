




































#ifndef _plbase64_h
#define _plbase64_h

#include "prtypes.h"

PR_BEGIN_EXTERN_C






















PR_EXTERN(char *)
PL_Base64Encode
(
    const char *src,
    PRUint32    srclen,
    char       *dest
);

























PR_EXTERN(char *)
PL_Base64Decode
(
    const char *src,
    PRUint32    srclen,
    char       *dest
);

PR_END_EXTERN_C

#endif 
