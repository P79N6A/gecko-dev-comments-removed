






#ifndef _SHA256_H_
#define _SHA256_H_

#define SHA256_LENGTH 32 

#include "prtypes.h"	
#include "prlong.h"

#ifdef __cplusplus 
extern "C" {
#endif

struct SHA256Context {
  union {
    PRUint32 w[64];	    
    PRUint8  b[256];
  } u;
  PRUint32 h[8];		
  PRUint32 sizeHi,sizeLo;	
};

typedef struct SHA256Context SHA256Context;

extern void
SHA256_Begin(SHA256Context *ctx);

extern void 
SHA256_Update(SHA256Context *ctx, const unsigned char *input,
		    unsigned int inputLen);

extern void
SHA256_End(SHA256Context *ctx, unsigned char *digest,
           unsigned int *digestLen, unsigned int maxDigestLen);

#ifdef __cplusplus 
} 
#endif

#endif
