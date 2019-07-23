



































#ifndef _SHA_256_H_
#define _SHA_256_H_

#include "base/third_party/nspr/prtypes.h"

struct SHA256ContextStr {
    union {
	PRUint32 w[64];	    
	PRUint8  b[256];
    } u;
    PRUint32 h[8];		
    PRUint32 sizeHi,sizeLo;	
};

#endif 
